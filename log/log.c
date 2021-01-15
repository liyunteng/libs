/*
 * log.c - log
 *
 * Date   : 2021/01/14
 */

#ifdef __cplusplus
#ifndef __STDC_FORMAT_MARCOS
#define __STDC_FORMAT_MARCOS
#endif
#endif

#include "log.h"
#include "list.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
/* #include <fcntl.h> */

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <syslog.h>

#ifdef ANDROID
#include <android/log.h>
#endif

#ifndef BOOL
#define BOOL int8_t
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define COLOR_NORMAL "\033[0;00m"
#define COLOR_EMERG "\033[5;7;31m"
#define COLOR_ALERT "\033[5;7;35m"
#define COLOR_FATAL "\033[5;7;33m"
#define COLOR_ERROR "\033[1;31m"
#define COLOR_WARNING "\033[1;35m"
#define COLOR_NOTICE "\033[1;34m"
#define COLOR_INFO "\033[1;37m"
#define COLOR_DEBUG "\033[0;00m"
#define COLOR_VERBOSE "\033[0;32m"

#define DEFAULT_SOCKADDR "127.0.0.1"
#define DEFAULT_SOCKPORT 12345
#define DEFAULT_FILEPATH "."
#define DEFAULT_FILENAME "test"
#define DEFAULT_BAKUP 0
#define DEFAULT_FILESIZE 4 * 1024 * 1024
#define DEFAULT_TIME_FORMAT "%F %T"
#define DEFAULT_FORMAT "%d.%ms %c:%p [%V] %F:%U(%L) %m%n"

#define BUFFER_MIN 1024 * 4
#define BUFFER_MAX 1024 * 1024 * 4

#define DEBUG_LOG(fmt, ...)                                                    \
    fprintf(stdout, "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define ERROR_LOG(fmt, ...)                                                    \
    fprintf(stderr, "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

struct log_format {
    char format[128];
    BOOL color;
    struct list_head format_entry;
};

typedef int (*log_output_config_init_fn)(log_output_t *output, va_list ap);
typedef void (*log_output_config_uninit_fn)(log_output_t *output);
typedef int (*log_output_emit_fn)(log_output_t *output, LOG_LEVEL_E level,
                                  char *buf, size_t len);
typedef void (*log_output_dump_fn)(log_output_t *output);
struct log_output {
    enum LOG_OUTTYPE type;
    char *type_name;
    struct list_head output_entry;
    struct {
        struct {
            uint32_t count;
            uint64_t bytes;
        } stats[LOG_VERBOSE + 1];
        uint64_t count_total;
        uint64_t bytes_total;
    } stat;
    void *conf;
    log_output_config_init_fn config_init;
    log_output_config_uninit_fn config_uninit;
    log_output_emit_fn emit;
    log_output_dump_fn dump;
};

typedef struct {
    char *file_path;
    char *log_name;
    uint16_t num_files;
    uint32_t file_size;
    uint16_t file_idx;
    uint32_t data_offset;
    FILE *fp;
} file_output_config;

typedef struct {
    char addr[256];
    uint16_t port;
    int sockfd;
} sock_output_config;

typedef struct {
    LOG_LEVEL_E level_begin;
    LOG_LEVEL_E level_end;
    log_output_t *output;
    log_format_t *format;
    struct list_head rule_entry;
    struct list_head rule;
} log_rule_t;

struct log_handler {
    pthread_mutex_t mutex;
    char ident[128];

    char *bufferp;
    size_t buffer_max;
    size_t buffer_min;
    size_t buffer_real;

    struct list_head rules;  // rules
    struct list_head handler_entry;
};

/* pointer to environment */
extern char **environ;

static const char *const LOGLEVELSTR[] = {
    "EMERG",  "ALERT", "FATAL", "ERROR",   "WARN",
    "NOTICE", "INFO",  "DEBUG", "VERBOSE",
};

static struct list_head output_header = {
    &output_header,
    &output_header,
};
static struct list_head format_header = {
    &format_header,
    &format_header,
};
static struct list_head rule_header = {
    &rule_header,
    &rule_header,
};
static struct list_head handler_header = {
    &handler_header,
    &handler_header,
};

/* dump the environment */
static void
log_dump_environment(void)
{
    static char buf[BUFSIZ];
    int cnt = 0;
    while (1) {
        char *e = environ[cnt++];

        if (!e || !*e) {
            break;
        }

        snprintf(buf, sizeof(buf), "%s", e);
        e = strchr(buf, '=');
        if (!e) {
            ERROR_LOG("Can't parse environment variable %s\n", buf);
            continue;
        }

        *e = 0;
        ++e;
        printf("Environment: [%s] = [%s]\n", buf, e);
    }
}

inline static void
statstic_dump(log_output_t *output)
{
    int i;
    if (output) {
        for (i = LOG_VERBOSE; i >= LOG_EMERG; i--) {
            printf("%-8s  count: %-8d  bytes: %-10llu\n", LOGLEVELSTR[i],
                   output->stat.stats[i].count,
                   (unsigned long long)output->stat.stats[i].bytes);
        }
        printf("%-8s  count: %-8llu  bytes: %-10llu\n", "TOTAL",
               (unsigned long long)output->stat.count_total,
               (unsigned long long)output->stat.bytes_total);
    }
}

static void
logout_dump(log_output_t *output)
{
    if (output) {
        printf("type: %s\n", output->type_name);

        statstic_dump(output);
    }
}

static int
file_getname(log_output_t *output, char *file_name, uint16_t len)
{
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }
    if (output->type != LOG_OUTTYPE_FILE) {
        ERROR_LOG("type invalid\n");
        return -1;
    }
    file_output_config *conf = (file_output_config *)output->conf;
    if (!conf) {
        ERROR_LOG("conf is NULL\n");
        return -1;
    }

    snprintf(file_name, len, "%s/%s.log", conf->file_path, conf->log_name);
    return 0;
}

static int
file_open_logfile(log_output_t *output)
{
    char *file_name;
    uint32_t len;

    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }
    if (output->type != LOG_OUTTYPE_FILE) {
        ERROR_LOG("type invalid\n");
        return -1;
    }

    file_output_config *conf = (file_output_config *)output->conf;
    if (!conf) {
        ERROR_LOG("conf is NULL\n");
        return -1;
    }

    if (conf->fp != NULL) {
        fclose(conf->fp);
        conf->fp = NULL;
    }

    len = strlen(conf->file_path);
    len += 1; /* "/" */
    len += strlen(conf->log_name);
    len += 4; /* ".log" */
    len += 1; /* NULL char */

    file_name = malloc(len * sizeof(char));
    if (!file_name) {
        ERROR_LOG("malloc failed(%s)\n", strerror(errno));
        return -1;
    }

    file_getname(output, file_name, len);

    if ((conf->fp = fopen(file_name, "w")) == NULL) {
        ERROR_LOG("fopen %s failed:(%s)\n", file_name, strerror(errno));
        free(file_name);
        return -1;
    }

    conf->data_offset = 0;
    // ftruncate(fileno(output->u.file.fp), output->u.file.file_size);

    free(file_name);
    return 0;
}

static int
file_rename_logfile(log_output_t *output)
{
    uint32_t num, num_files, len;
    char *old_file_name, *new_file_name;

    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (output->type != LOG_OUTTYPE_FILE) {
        ERROR_LOG("type invalid\n");
        return -1;
    }

    file_output_config *conf = (file_output_config *)output->conf;
    if (!conf) {
        ERROR_LOG("conf is NULL\n");
        return -1;
    }

    if (conf->num_files > 0) {
        for (num = 0, num_files = conf->num_files; num_files; num_files /= 10) {
            ++num;
        }

        len = strlen(conf->file_path);
        len += 1; /* "/" */

        len += strlen(conf->log_name);
        len += 4;         /* ".log" */
        len += (num + 1); /* ".<digit>" */
        len += 1;         /* NULL char */

        old_file_name = malloc(len * sizeof(char));
        new_file_name = malloc(len * sizeof(char));
        if (!old_file_name || !new_file_name) {
            ERROR_LOG("malloc failed(%s)\n", strerror(errno));
            free(old_file_name);
            free(new_file_name);
            return -1;
        }

        file_getname(output, old_file_name, len);
        snprintf(new_file_name, len, "%s.%u", old_file_name,
                 (conf->file_idx % conf->num_files));
        rename(old_file_name, new_file_name);

        ++conf->file_idx;
        DEBUG_LOG("rename %s ==> %s\n", old_file_name, new_file_name);
        free(old_file_name);
        free(new_file_name);
    }
    return 0;
}

static int
stdout_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
    if (fwrite(buf, len, 1, stdout) != 1) {
        ERROR_LOG("fwrite failed(%s)\n", strerror(errno));
        return -1;
    }
    return len;
}

static int
stderr_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
    if (fwrite(buf, len, 1, stderr) != 1) {
        ERROR_LOG("fwrite failed(%s)\n", strerror(errno));
        return -1;
    }
    return len;
}

static int
logcat_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
#ifdef ANDROID
    android_LogPriority pri;
    switch (level) {
    case LOG_VERBOSE:
        pri = ANDROID_LOG_VERBOSE;
        break;
    case LOG_DEBUG:
        pri = ANDROID_LOG_DEBUG;
        break;
    case LOG_INFO:
        pri = ANDROID_LOG_INFO;
        break;
    case LOG_WARNING:
        pri = ANDROID_LOG_WARN;
        break;
    case LOG_ERROR:
        pri = ANDROID_LOG_ERROR;
        break;
    case LOG_FATAL:
        pri = ANDROID_LOG_FATAL;
        break;
    case LOG_ALERT:
        pri = ANDROID_LOG_FATAL;
        break;
    case LOG_EMERG:
        pri = ANDROID_LOG_SILENT;
        break;
    default:
        pri = ANDROID_LOG_DEFAULT;
        break;
    }
    return __android_log_vprint(l, r->indent, format, args);
#else
    return -1;
#endif
}

static int
syslog_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
    syslog(level, "%s", buf);
    return len;
}

static int
file_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
    int ret;
    file_output_config *conf = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    conf = (file_output_config *)output->conf;
    if (!conf) {
        ERROR_LOG("conf is NULL\n");
        return -1;
    }

    if (conf->fp == NULL) {
        ret = file_open_logfile(output);
        if (ret < 0) {
            ERROR_LOG("open logfile failed\n");
            return -1;
        }
    }

    int left = conf->file_size - conf->data_offset;
    if (left <= len) {
        if (fwrite(buf, left, 1, conf->fp) != 1) {
            ERROR_LOG("fwrite failed(%s)\n", strerror(errno));
            return -1;
        }
        conf->data_offset += left;

        file_rename_logfile(output);
        ret = file_open_logfile(output);
        if (ret != 0) {
            ERROR_LOG("open logfile failed\n");
            return left;
        }

        if (fwrite(buf + left, len - left, 1, conf->fp) != 1) {
            ERROR_LOG("fwrite failed(%s)\n", strerror(errno));
            return left;
        }

        conf->data_offset += (len - left);
        return len;
    } else {

        if (fwrite(buf, len, 1, conf->fp) != 1) {
            ERROR_LOG("fwrite failed(%s) len:%lu\n", strerror(errno), len);
            return -1;
        }
        conf->data_offset += len;
        return len;
    }
}

static int
sock_emit(log_output_t *output, LOG_LEVEL_E level, char *buf, size_t len)
{
    sock_output_config *conf = NULL;

    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }
    conf = (sock_output_config *)output->conf;
    if (!conf) {
        ERROR_LOG("conf is NULL\n");
        return -1;
    }

    if (conf->sockfd == -1) {
        return -1;
    }

    int total = 0;
    int nsend = 0;
    while (total < len) {
        nsend = send(conf->sockfd, buf + total, len - total, MSG_NOSIGNAL);
        if (nsend < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            } else {
                ERROR_LOG("send failed(%s)\n", strerror(errno));
                close(conf->sockfd);
                conf->sockfd = -1;
                break;
            }
        } else if (nsend == 0) {
            ERROR_LOG("sock closed\n");
            close(conf->sockfd);
            conf->sockfd = -1;
            break;
        } else {
            total += nsend;
        }
    }
    return total;
}

static void
file_config_dump(log_output_t *output)
{
    if (output) {
        printf("type: %s\n", output->type_name);
        file_output_config *conf = (file_output_config *)output->conf;
        if (conf) {
            printf("filepath: %s\n", conf->file_path);
            printf("logname:  %s\n", conf->log_name);
            printf("filesize: %d\n", conf->file_size);
            printf("bakup:    %d\n", conf->num_files);
            printf("idx:      %d\n", conf->file_idx);
            printf("offset:   %u\n", conf->data_offset);
        }
        statstic_dump(output);
    }
}

static int
file_config_init(log_output_t *output, va_list ap)
{
    file_output_config *conf = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (output->conf == NULL) {
        output->conf =
            (file_output_config *)calloc(1, sizeof(file_output_config));
        if (output->conf == NULL) {
            ERROR_LOG("calloc failed(%s)\n", strerror(errno));
            return -1;
        }
    }
    conf = (file_output_config *)output->conf;

    char *file_path = va_arg(ap, char *);
    if (file_path && strlen(file_path) > 0) {
        conf->file_path = strdup(file_path);
    } else {
        conf->file_path = strdup(DEFAULT_FILEPATH);
    }

    char *file_name = va_arg(ap, char *);
    if (file_name && strlen(file_name) > 0) {
        conf->log_name = strdup(file_name);
    } else {
        conf->log_name = strdup(DEFAULT_FILENAME);
    }

    unsigned long file_size = va_arg(ap, unsigned long);
    if (file_size > 0) {
        conf->file_size = file_size;
    } else {
        conf->file_size = DEFAULT_FILESIZE;
    }

    int num_files = va_arg(ap, int);
    if (num_files >= 0) {
        conf->num_files = num_files;
    } else {
        conf->num_files = DEFAULT_BAKUP;
    }

    conf->fp       = NULL;
    conf->file_idx = 0;

    return 0;
}

static void
file_config_uninit(log_output_t *output)
{
    file_output_config *conf = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return;
    }
    conf = (file_output_config *)output->conf;
    if (conf == NULL) {
        return;
    }

    if (conf->file_path) {
        free(conf->file_path);
        conf->file_path = NULL;
    }
    if (conf->log_name) {
        free(conf->log_name);
        conf->log_name = NULL;
    }
    if (conf->fp) {
        fclose(conf->fp);
        conf->fp = NULL;
    }
    free(conf);
    conf         = NULL;
    output->conf = NULL;
}

static void
sock_config_dump(log_output_t *output)
{
    if (output) {
        printf("type: %s\n", output->type_name);
        sock_output_config *conf = (sock_output_config *)output->conf;
        if (conf) {
            printf("addr: %s:%d\n", conf->addr, conf->port);
        }
        statstic_dump(output);
    }
}

static int
sock_config_init(log_output_t *output, va_list ap)
{
    sock_output_config *conf = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (output->conf == NULL) {
        output->conf =
            (sock_output_config *)calloc(1, sizeof(sock_output_config));
        if (!output->conf) {
            ERROR_LOG("calloc failed(%s)\n", strerror(errno));
            goto failed;
        }
        ((sock_output_config *)(output->conf))->sockfd = -1;
    }
    conf = (sock_output_config *)output->conf;

    char *addr_str = va_arg(ap, char *);
    if (addr_str && strlen(addr_str) > 0) {
        strncpy(conf->addr, addr_str, strlen(addr_str) + 1);
    } else {
        strncpy(conf->addr, DEFAULT_SOCKADDR, strlen(DEFAULT_SOCKADDR) + 1);
    }

    unsigned port = va_arg(ap, unsigned);
    if (port > 0 && port < 65535) {
        conf->port = port;
    } else {
        conf->port = DEFAULT_SOCKPORT;
    }

    if (conf->sockfd != -1) {
        close(conf->sockfd);
        conf->sockfd = -1;
    }

    struct hostent *host = gethostbyname(conf->addr);
    if (host == NULL) {
        ERROR_LOG("gethostbyname failed(%s)\n", strerror(errno));
        goto failed;
    }

    if (output->type == LOG_OUTTYPE_TCP) {
        conf->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        conf->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    }
    if (conf->sockfd < 0) {
        ERROR_LOG("socket failed(%s)\n", strerror(errno));
        goto failed;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(conf->port);
    addr.sin_addr   = *(struct in_addr *)(host->h_addr_list[0]);
    if (connect(conf->sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ERROR_LOG("connect(%s)\n", strerror(errno));
        goto failed;
    }

    return 0;

failed:
    if (conf != NULL) {
        if (conf->sockfd != -1) {
            close(conf->sockfd);
            conf->sockfd = -1;
        }
        free(conf);
        conf = NULL;
    }
    return -1;
}

static void
sock_config_uninit(log_output_t *output)
{
    sock_output_config *conf = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return;
    }
    conf = (sock_output_config *)output->conf;
    if (conf == NULL) {
        return;
    }

    if (conf->sockfd != -1) {
        close(conf->sockfd);
        conf->sockfd = -1;
    }
    free(conf);
    conf         = NULL;
    output->conf = NULL;
}

// ################################################################################

static size_t
log_parse_format(log_handler_t *handler, log_rule_t *r, const LOG_LEVEL_E level,
                 const char *file, const char *func, const long line,
                 const char *fmt, va_list ap)
{

    if (r == NULL || r->format == NULL || handler == NULL
        || handler->bufferp == NULL) {
        ERROR_LOG("invalid argument\n");
        return 0;
    }

    va_list local_ap;
    size_t idx;
    struct tm now;
    size_t len;
    char *buf;
    int nscan;
    int nread;
    time_t t;
    char hostname[128];

begin:
    va_copy(local_ap, ap);
    buf = handler->bufferp;
    len = handler->buffer_real;
    memset(buf, 0, handler->buffer_real);

    nscan = 0;
    nread = 0;
    idx   = 0;

    if (r->format->color) {
        switch (level) {
        case LOG_EMERG:
            idx += snprintf(buf + idx, len - idx, "%s", COLOR_EMERG);
            break;
        case LOG_ALERT:
            idx += snprintf(buf + idx, len - idx, "%s", COLOR_ALERT);
            break;
        case LOG_FATAL:
            idx += snprintf(buf + idx, len - idx, "%s", COLOR_FATAL);
            break;
        case LOG_ERROR:
            idx += snprintf(buf + idx, len - idx, "%s", COLOR_ERROR);
            break;
        case LOG_WARNING:
            idx += snprintf(buf + idx, len - idx, "%s", COLOR_WARNING);
            break;
        case LOG_NOTICE:
            idx += snprintf(buf + idx, len - idx, "%s", COLOR_NOTICE);
            break;
        case LOG_INFO:
            idx += snprintf(buf + idx, len - idx, "%s", COLOR_INFO);
            break;
        case LOG_DEBUG:
            idx += snprintf(buf + idx, len - idx, "%s", COLOR_DEBUG);
            break;
        case LOG_VERBOSE:
            idx += snprintf(buf + idx, len - idx, "%s", COLOR_VERBOSE);
            break;
        default:
            idx += snprintf(buf + idx, len - idx, "%s", COLOR_NORMAL);
            break;
        }
    }

    char *p = r->format->format;
    char format_buf[128];
    while (*p) {
        if (idx >= len) {
            if (handler->buffer_real < handler->buffer_max) {
                if (handler->buffer_real * 2 <= handler->buffer_max) {
                    handler->bufferp = (char *)realloc(
                        handler->bufferp, handler->buffer_real * 2);
                    if (handler->bufferp) {
                        handler->buffer_real *= 2;
                        DEBUG_LOG("realloc buffer to: %lu\n",
                                  handler->buffer_real);
                        goto begin;
                    }

                    handler->buffer_real = 0;
                    ERROR_LOG("realloc failed(%s)\n", strerror(errno));
                    goto err;
                } else {
                    handler->bufferp =
                        (char *)realloc(handler->bufferp, handler->buffer_max);
                    if (handler->bufferp) {
                        handler->buffer_real = handler->buffer_max;
                        DEBUG_LOG("realloc buffer to: %lu\n",
                                  handler->buffer_real);
                        goto begin;
                    }

                    handler->buffer_real = 0;
                    ERROR_LOG("realloc failed(%s)\n", strerror(errno));
                    goto err;
                }
            } else {
                snprintf(buf + len - 13, 13, "%s", "(truncated)\n");
                DEBUG_LOG("msg too long, truncated to %u.\n", (unsigned)idx);
                goto end;
            }
        }

        if (*p == '%') {
            nread = 0;
            nscan = sscanf(p, "%%%[.0-9]%n", format_buf, &nread);
            if (nscan == 1) {
                ERROR_LOG("parse format [%s] failed.\n", p);
                goto err;
            } else {
                nread = 1;
            }
            p += nread;

            if (*p == 'E') {
                char env[128];
                nread = 0;
                nscan = sscanf(p, "E(%[^)])%n", env, &nread);
                if (nscan != 1) {
                    nread = 0;
                }
                p += nread;
                if (*(p - 1) != ')') {
                    ERROR_LOG(
                        "parse foramt [%s] failed, can't find "
                        "\')\'.\n",
                        p);
                    goto err;
                }
                idx += snprintf(buf + idx, len - idx, "%s", getenv(env));
                continue;
            }

            if (*p == 'd') {
                char time_format[32] = {0};
                if (*(p + 1) != '(') {
                    strcpy(time_format, DEFAULT_TIME_FORMAT);
                    p++;
                } else if (strncmp(p, "d()", 3) == 0) {
                    strcpy(time_format, DEFAULT_TIME_FORMAT);
                    p += 3;
                } else {
                    nread = 0;
                    nscan = sscanf(p, "d(%[^)])%n", time_format, &nread);
                    if (nscan != 1) {
                        nread = 0;
                    }
                    p += nread;
                    if (*(p - 1) != ')') {
                        ERROR_LOG(
                            "parse format [%s] failed, can't find "
                            "\')\''.\n",
                            p);
                        goto err;
                    }
                }

                t = time(NULL);
                localtime_r(&t, &now);
                idx += strftime(buf + idx, len - idx, time_format, &now);
                continue;
            }

            if (strncmp(p, "ms", 2) == 0) {
                p += 2;
                struct timeval tv;
                gettimeofday(&tv, NULL);
                idx += snprintf(buf + idx, len - idx, "%03d",
                                (int)(tv.tv_usec / 1000));
                continue;
            }

            if (strncmp(p, "us", 2) == 0) {
                p += 2;
                struct timeval tv;
                gettimeofday(&tv, NULL);
                idx += snprintf(buf + idx, len - idx, "%6d", (int)(tv.tv_usec));
                continue;
            }

            switch (*p) {
            case 'D':
                t = time(NULL);
                localtime_r(&t, &now);
                idx += strftime(buf + idx, len - idx, "%F", &now);
                break;
            case 'T':
                t = time(NULL);
                localtime_r(&t, &now);
                idx += strftime(buf + idx, len - idx, "%T", &now);
                break;
            case 'F':
                idx += snprintf(buf + idx, len - idx, "%s", file);
                break;
            case 'U':
                idx += snprintf(buf + idx, len - idx, "%s", func);
                break;
            case 'L':
                idx += snprintf(buf + idx, len - idx, "%ld", line);
                break;
            case 'n':
                idx += snprintf(buf + idx, len - idx, "%c", '\n');
                break;
            case 'p':
                idx += snprintf(buf + idx, len - idx, "%u", getpid());
                break;
            case 'm':
                idx += vsnprintf(buf + idx, len - idx, fmt, local_ap);
                break;
            case 'c':
                idx += snprintf(buf + idx, len - idx, "%s", handler->ident);
                break;
            case 'V':
                idx +=
                    snprintf(buf + idx, len - idx, "%5.5s", LOGLEVELSTR[level]);
                break;
            case 'H':
                gethostname(hostname, 128);
                idx += snprintf(buf + idx, len - idx, "%s", hostname);
                break;
            case 't':
                idx += snprintf(buf + idx, len - idx, "%lu",
                                (unsigned long)pthread_self());
                break;
            case '%':
                idx += snprintf(buf + idx, len - idx, "%c", *p);
                break;
            default:
                idx += snprintf(buf + idx, len - idx, "%c", *p);
                break;
            }
        } else {
            idx += snprintf(buf + idx, len - idx, "%c", *p);
        }

        p++;
    }

end:
    if (r->format->color) {
        idx += snprintf(buf + idx, len - idx, "%s", COLOR_NORMAL);
    }
    return idx;

err:
    return 0;
}

static void
log_update_stat(log_output_t *output, const LOG_LEVEL_E level, size_t len)
{
    if (output) {
        output->stat.stats[level].count++;
        output->stat.stats[level].bytes += len;

        output->stat.count_total++;
        output->stat.bytes_total += len;
    }
}

static void
vlog(log_handler_t *handler, const LOG_LEVEL_E lvl, const char *file,
     const char *function, const long line, const char *fmt, va_list ap)
{
    uint16_t i;
    int ret;
    LOG_LEVEL_E level;
    if (handler == NULL) {
        ERROR_LOG("handler is NULL\n");
        return;
    }

    level = lvl;
    if (level > LOG_VERBOSE)
        level = LOG_VERBOSE;
    if (lvl < LOG_EMERG)
        level = LOG_EMERG;

    log_rule_t *r = NULL;
    pthread_mutex_lock(&handler->mutex);
    list_for_each_entry(r, &(handler->rules), rule)
    {
        if (r->level_begin < level || r->level_end > level) {
            continue;
        }

        va_list ap2;
        va_copy(ap2, ap);
        size_t len =
            log_parse_format(handler, r, level, file, function, line, fmt, ap2);
        va_end(ap2);
        if (len <= 0) {
            continue;
        }

        ret = r->output->emit(r->output, level, handler->bufferp, len);
        if (ret >= 0) {
            log_update_stat(r->output, level, ret);
        }
    }
    pthread_mutex_unlock(&handler->mutex);
}

static int
log_ctl_(enum LOG_OPTS opt, va_list ap)
{
    log_handler_t *handler = va_arg(ap, log_handler_t *);
    if (handler == NULL) {
        ERROR_LOG("invalid indent\n");
        return -1;
    }
    switch (opt) {
    case LOG_OPT_SET_HANDLER_BUFFERSIZEMIN: {
        size_t buffer_min   = va_arg(ap, size_t);
        handler->buffer_min = buffer_min;
        if (handler->buffer_real < buffer_min)
            handler->bufferp = (char *)realloc(handler->bufferp, buffer_min);
        if (handler->bufferp == NULL) {
            handler->buffer_real = 0;
            ERROR_LOG("realloc failed(%s)\n", strerror(errno));
            return -1;
        }
        if (handler->buffer_real < buffer_min)
            handler->buffer_real = buffer_min;
        break;
    }

    case LOG_OPT_SET_HANDLER_BUFFERSIZEMAX: {
        size_t buffer_max   = va_arg(ap, size_t);
        handler->buffer_max = buffer_max;
        if (handler->buffer_real > buffer_max)
            handler->bufferp = (char *)realloc(handler->bufferp, buffer_max);
        if (handler->bufferp == NULL) {
            handler->buffer_real = 0;
            ERROR_LOG("realloc failed(%s)\n", strerror(errno));
            return -1;
        }
        if (handler->buffer_real > buffer_max)
            handler->buffer_real = buffer_max;
        break;
    }
    case LOG_OPT_GET_HANDLER_BUFFERSIZEMIN: {
        size_t *buffer_min = va_arg(ap, size_t *);
        if (!buffer_min) {
            ERROR_LOG("buffer_min pointer is NULL\n");
            return -1;
        }
        *buffer_min = handler->buffer_min;
        break;
    }
    case LOG_OPT_GET_HANDLER_BUFFERSIZEMAX: {
        size_t *buffer_max = va_arg(ap, size_t *);
        if (!buffer_max) {
            ERROR_LOG("buffer_max pointer is NULL\n");
            return -1;
        }
        *buffer_max = handler->buffer_max;
        break;
    }
    case LOG_OPT_GET_HANDLER_BUFFERSIZEREAL: {
        size_t *buffer_real = va_arg(ap, size_t *);
        if (!buffer_real) {
            ERROR_LOG("buffer_real pointer is NULL\n");
            return -1;
        }
        *buffer_real = handler->buffer_real;
        break;
    }
    case LOG_OPT_SET_HANDLER_IDENT: {
        char *ident = va_arg(ap, char *);
        if (!ident) {
            ERROR_LOG("ident pointer is NULL\n");
            return -1;
        }
        strncpy(handler->ident, ident, strlen(ident) + 1);
        break;
    }
    case LOG_OPT_GET_HANDLER_IDENT: {
        char *ident = va_arg(ap, char *);
        if (!ident) {
            ERROR_LOG("ident is NULL\n");
            return -1;
        }
        strncpy(ident, handler->ident, strlen(handler->ident) + 1);

        break;
    }
    default:
        ERROR_LOG("invalid LOG_OPT: %d\n", opt);
        return -1;
    }

    return 0;
}

int
log_ctl(enum LOG_OPTS opt, ...)
{
    va_list ap;
    va_start(ap, opt);
    int ret = log_ctl_(opt, ap);
    va_end(ap);
    return ret;
}

log_handler_t *
log_handler_create(const char *ident)
{
    log_handler_t *handler = log_handler_get(ident);
    if (handler == NULL) {
        handler = (log_handler_t *)malloc(sizeof(log_handler_t));
        if (handler != NULL) {
            pthread_mutex_init(&handler->mutex, NULL);
            strncpy(handler->ident, ident, strlen(ident) + 1);
            handler->buffer_max  = BUFFER_MAX;
            handler->buffer_min  = BUFFER_MIN;
            handler->buffer_real = BUFFER_MIN;
            handler->bufferp     = (char *)calloc(1, BUFFER_MIN);
            if (handler->bufferp == NULL) {
                ERROR_LOG("calloc failed(%s)\n", strerror(errno));
                pthread_mutex_destroy(&handler->mutex);
                free(handler);
                return NULL;
            }

            INIT_LIST_HEAD(&handler->rules);
            list_add_tail(&handler->handler_entry, &handler_header);
        }
    }
    return handler;
}

void
log_handler_destory(log_handler_t *handler)
{
    if (!handler) {
        ERROR_LOG("handler is NULL\n");
        return;
    }
    list_del(&handler->handler_entry);

    pthread_mutex_destroy(&handler->mutex);
    if (handler->bufferp) {
        free(handler->bufferp);
        handler->bufferp = NULL;
    }

    log_rule_t *r, *tmp;
    list_for_each_entry_safe(r, tmp, &(handler->rules), rule)
    {
        if (r) {
            list_del(&r->rule_entry);
            list_del(&r->rule);
            free(r);
            r = NULL;
        }
    }
}

log_handler_t *
log_handler_get(const char *ident)
{
    log_handler_t *handler = NULL;
    list_for_each_entry(handler, &handler_header, handler_entry)
    {
        if (handler && strcmp(handler->ident, ident) == 0) {
            return handler;
        }
    }
    return NULL;
}

log_format_t *
log_format_create(const char *format, int color)
{
    log_format_t *fp = NULL;
    if (format) {
        fp = (log_format_t *)calloc(1, sizeof(log_format_t));
        if (fp) {
            strncpy(fp->format, format, strlen(format) + 1);
            if (color) {
                fp->color = TRUE;
            } else {
                fp->color = FALSE;
            }
            list_add_tail(&fp->format_entry, &format_header);
            return fp;
        } else {
            ERROR_LOG("alloc failed(%s)\n", strerror(errno));
            return NULL;
        }
    }
    return NULL;
}
void
log_format_destory(log_format_t *format)
{
    if (!format) {
        ERROR_LOG("format is NULL\n");
        return;
    }
    /* TODO: delete from rule */
    list_del(&format->format_entry);
    free(format);
    format = NULL;
}

log_output_t *
log_output_create_(enum LOG_OUTTYPE type, va_list ap)
{
    log_output_t *output = NULL;
    output               = (log_output_t *)calloc(1, sizeof(log_output_t));
    if (!output) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }

    output->type = type;
    output->dump = logout_dump;

    switch (type) {
    case LOG_OUTTYPE_STDOUT:
        output->type_name = "stdout";
        output->emit      = stdout_emit;
        break;
    case LOG_OUTTYPE_STDERR:
        output->type_name = "stderr";
        output->emit      = stderr_emit;
        break;
    case LOG_OUTTYPE_LOGCAT:
        output->type_name = "logcat";
        output->emit      = logcat_emit;
        break;
    case LOG_OUTTYPE_SYSLOG:
        output->type_name = "syslog";
        output->emit      = syslog_emit;
        break;
    case LOG_OUTTYPE_FILE:
        output->type_name     = "file";
        output->emit          = file_emit;
        output->config_init   = file_config_init;
        output->config_uninit = file_config_uninit;
        output->dump          = file_config_dump;
        break;
    case LOG_OUTTYPE_TCP:
    case LOG_OUTTYPE_UDP:
        if (type == LOG_OUTTYPE_TCP) {
            output->type_name = "tcp";
        } else {
            output->type_name = "udp";
        }
        output->emit          = sock_emit;
        output->config_init   = sock_config_init;
        output->config_uninit = sock_config_uninit;
        output->dump          = sock_config_dump;
        break;
    default:
        output->type_name = "unknown";
        ERROR_LOG("invalid type: %d\n", type);
        goto failed;
    }

    if (output->config_init) {
        if (output->config_init(output, ap) != 0) {
            ERROR_LOG("%s config_init failed\n", output->type_name);
            goto failed;
        }
    }

    list_add_tail(&output->output_entry, &output_header);
    return output;

failed:
    free(output);
    return NULL;
    /* list_add_tail(&output->output_entry, &output_header); */
    /* return output; */
}

log_output_t *
log_output_create(enum LOG_OUTTYPE type, ...)
{
    log_output_t *output = NULL;
    va_list ap;
    va_start(ap, type);
    output = log_output_create_(type, ap);
    va_end(ap);
    return output;
}

void
log_output_destroy(log_output_t *output)
{
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return;
    }
    /* TODO: delete from rule's list */
    list_del(&output->output_entry);

    if (output->type == LOG_OUTTYPE_FILE) {
        file_config_uninit(output);
    } else if (output->type == LOG_OUTTYPE_TCP
               || output->type == LOG_OUTTYPE_UDP) {
        sock_config_uninit(output);
    }
    free(output);
    output = NULL;
}


int
log_bind(log_handler_t *handler, LOG_LEVEL_E level_begin, LOG_LEVEL_E level_end,
         log_format_t *format, log_output_t *output)
{
    if (handler == NULL || format == NULL || output == NULL) {
        ERROR_LOG("invalid argument\n");
        return -1;
    }

    log_rule_t *r = (log_rule_t *)calloc(1, sizeof(log_rule_t));
    if (!r) {
        ERROR_LOG("malloc failed(%s)\n", strerror(errno));
        return -1;
    }

    if (level_begin >= LOG_EMERG && level_begin <= LOG_VERBOSE) {
        r->level_begin = level_begin;
    } else {
        r->level_begin = LOG_VERBOSE;
    }

    if (level_end >= LOG_EMERG && level_end <= LOG_VERBOSE) {
        r->level_end = level_end;
    } else {
        r->level_end = LOG_EMERG;
    }

    r->format = format;
    r->output = output;
    list_add_tail(&r->rule_entry, &rule_header);
    list_add_tail(&r->rule, &handler->rules);

    return 0;
}

int
log_unbind(log_handler_t *handler, log_output_t *output)
{
    if (handler == NULL || output == NULL) {
        ERROR_LOG("invalid argument\n");
        return -1;
    }

    log_rule_t *r;
    list_for_each_entry(r, &(handler->rules), rule)
    {
        if (r->output == output) {
            list_del(&r->rule);
            return 0;
        }
    }

    ERROR_LOG("output not found\n");
    return -1;
}

void
mlog(log_handler_t *handle, LOG_LEVEL_E level, const char *file,
     const char *function, long line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(handle, level, file, function, line, format, args);
    va_end(args);
    return;
}

void
slog(LOG_LEVEL_E level, const char *file, const char *function, long line,
     const char *fmt, ...)
{
    log_handler_t *handler = log_handler_get(DEFAULT_IDENT);
    if (handler) {
        va_list ap;
        va_start(ap, fmt);
        vlog(handler, level, file, function, line, fmt, ap);
        va_end(ap);
    } else {
        DEBUG_LOG("can't find handler: %s\n", DEFAULT_IDENT);
    }
    return;
}

void
log_dump(void)
{
    int i = 0;
    int j = 0;
    printf("=====================log profile==============================\n");
    log_rule_t *ru;
    list_for_each_entry(ru, &rule_header, rule_entry) { i++; }
    printf("ctx: rule: %d", i);
    i = 0;
    log_format_t *format;
    list_for_each_entry(format, &format_header, format_entry) { i++; }
    printf(" format: %d", i);
    i = 0;
    log_output_t *output;
    list_for_each_entry(output, &output_header, output_entry) { i++; }
    printf(" output: %d\n", i);
    i = 0;
    log_handler_t *handler;
    list_for_each_entry(handler, &handler_header, handler_entry)
    {
        i++;
        j = 0;
        printf("------------------------\n");
        printf("handler: %d\n", i);
        printf("ident: %s\n", handler->ident);
        printf("buffer_min: %u\n", (unsigned)handler->buffer_min);
        printf("buffer_real: %u\n", (unsigned)handler->buffer_real);
        printf("buffer_max: %u\n", (unsigned)handler->buffer_max);
        printf("\n");
        log_rule_t *r = NULL;
        list_for_each_entry(r, &handler->rules, rule)
        {
            j++;
            if (r) {
                printf("rule: %d\n", j);
                printf("format: %s\n", r->format->format);
                printf("level: %s -- %s\n", LOGLEVELSTR[r->level_begin],
                       LOGLEVELSTR[r->level_end]);

                if (r->output->dump) {
                    r->output->dump(r->output);
                }

                printf("\n");
            }
        }
    }
}
