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

#define DEBUG_LOG

struct log_format {
    char format[128];
    BOOL color;
    struct list_head l;
};

typedef struct {
    uint32_t count;
    uint64_t bytes;
} log_statstic_t;

typedef int (*log_output_init_fn)();
typedef int (*log_output_uninit_fn)();
typedef int (*log_output_emit_fn)();
struct log_output {
    enum LOG_OUTTYPE type;
    char *type_name;
    pthread_mutex_t mutex;
    struct list_head l;
    struct {
        log_statstic_t stats[LOG_VERBOSE + 1];
        uint64_t count_total;
        uint64_t bytes_total;
    } stat;
    void *config;
    log_output_init_fn init;
    log_output_uninit_fn uninit;
    log_output_emit_fn emit;
};

typedef struct {
    char *file_path;
    char *log_name;
    uint16_t num_files;
    uint32_t file_size;
    uint16_t file_idx;
    uint32_t data_offset;
    FILE *fp;
    int fd;
} file_output_config;

typedef struct {
    char addr[256];
    unsigned short port;
    int sockfd;
} sock_output_config;

typedef struct {
    LOG_LEVEL_E level_begin;
    LOG_LEVEL_E level_end;
    log_output_t *output;
    log_format_t *format;
    struct list_head l;
} log_rule_t;

typedef struct {
    log_rule_t *rule;
    struct list_head l;
} log_rules;

struct log_handler {
    pthread_mutex_t mutex;
    char ident[128];

    char *bufferp;
    size_t buffer_max;
    size_t buffer_min;
    size_t buffer_real;

    struct list_head rules;  // prule
    struct list_head l;
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

static int
log_file_name(char *file_name, uint16_t len, log_output_t *output)
{
    if (!output) {
        return -1;
    }
    if (output->type != LOG_OUTTYPE_FILE) {
        return -1;
    }
    file_output_config *config = (file_output_config *)output->config;
    if (!config) {
        return -1;
    }

    snprintf(file_name, len, "%s/%s.log", config->file_path, config->log_name);
    return 0;
}

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
            printf("Can't parse environment variable %s\n", buf);
            continue;
        }

        *e = 0;
        ++e;
        printf("Environment: [%s] = [%s]\n", buf, e);
    }
}

static int
log_open_logfile(log_output_t *output)
{
    char *file_name;
    uint32_t len;

    if (!output) {
        return -1;
    }
    if (output->type != LOG_OUTTYPE_FILE) {
        return -1;
    }
    file_output_config *config = (file_output_config *)output->config;
    if (!config) {
        return -1;
    }

    if (config->fp != NULL) {
        fclose(config->fp);
        config->fp = NULL;
    }

    len = strlen(config->file_path);
    len += 1; /* "/" */
    len += strlen(config->log_name);
    len += 4; /* ".log" */
    len += 1; /* NULL char */

    file_name = malloc(len * sizeof(char));
    if (!file_name) {
        return -1;
    }

    log_file_name(file_name, len, output);

    if ((config->fp = fopen(file_name, "w")) == NULL) {
        free(file_name);
        return -1;
    }

    config->data_offset = 0;
    // ftruncate(fileno(output->u.file.fp), output->u.file.file_size);

    free(file_name);
    return 0;
}

static int
log_rename_logfile(log_output_t *output)
{
    uint32_t num, num_files, len;
    char *old_file_name, *new_file_name;

    if (!output) {
        return -1;
    }

    if (output->type != LOG_OUTTYPE_FILE) {
        return -1;
    }

    file_output_config *config = (file_output_config *)output->config;
    if (!config) {
        return -1;
    }

    if (config->num_files > 0) {
        for (num = 0, num_files = config->num_files; num_files;
             num_files /= 10) {
            ++num;
        }

        len = strlen(config->file_path);
        len += 1; /* "/" */

        len += strlen(config->log_name);
        len += 4;         /* ".log" */
        len += (num + 1); /* ".<digit>" */
        len += 1;         /* NULL char */

        old_file_name = malloc(len * sizeof(char));
        new_file_name = malloc(len * sizeof(char));
        if (!old_file_name || !new_file_name) {
            free(old_file_name);
            free(new_file_name);
            return -1;
        }

        log_file_name(old_file_name, len, output);
        snprintf(new_file_name, len, "%s.%u", old_file_name,
                 (config->file_idx % config->num_files));
        rename(old_file_name, new_file_name);

        ++config->file_idx;
        free(old_file_name);
        free(new_file_name);
    }
    return 0;
}

static int
log_ctl_(enum LOG_OPTS opt, va_list ap)
{
    switch (opt) {
    case LOG_OPT_SET_HANDLER_BUFFERSIZEMIN: {
        log_handler_t *lp = va_arg(ap, log_handler_t *);
        if (lp == NULL) {
#ifdef DEBUG_LOG
            fprintf(stderr, "logdebug: invalid ident\n");
#endif
            return -1;
        }
        size_t buffer_min = va_arg(ap, size_t);
        lp->buffer_min    = buffer_min;
        if (lp->buffer_real < buffer_min)
            lp->bufferp = (char *)realloc(lp->bufferp, buffer_min);
        if (lp->bufferp == NULL) {
            lp->buffer_real = 0;
#ifdef DEBUG_LOG
            perror("logdebug: realloc failed");
#endif
            return -1;
        }
        if (lp->buffer_real < buffer_min)
            lp->buffer_real = buffer_min;

        break;
    }

    case LOG_OPT_SET_HANDLER_BUFFERSIZEMAX: {
        log_handler_t *lp = va_arg(ap, log_handler_t *);
        if (lp == NULL) {
#ifdef DEBUG_LOG
            fprintf(stderr, "logdebug: invalid ident\n");
#endif
            return -1;
        }

        size_t buffer_max = va_arg(ap, size_t);
        lp->buffer_max    = buffer_max;
        if (lp->buffer_real > buffer_max)
            lp->bufferp = (char *)realloc(lp->bufferp, buffer_max);
        if (lp->bufferp == NULL) {
            lp->buffer_real = 0;
#ifdef DEBUG_LOG
            perror("logdebug: realloc failed");
#endif
            return -1;
        }
        if (lp->buffer_real > buffer_max)
            lp->buffer_real = buffer_max;

        break;
    }
    case LOG_OPT_GET_HANDLER_BUFFERSIZEMIN: {
        log_handler_t *lp = va_arg(ap, log_handler_t *);
        if (lp == NULL) {
#ifdef DEBUG_LOG
            fprintf(stderr, "logdebug: invalid ident\n");
#endif
            return -1;
        }

        size_t *buffer_min = va_arg(ap, size_t *);
        *buffer_min        = lp->buffer_min;
        break;
    }
    case LOG_OPT_GET_HANDLER_BUFFERSIZEMAX: {
        log_handler_t *lp = va_arg(ap, log_handler_t *);
        if (lp == NULL) {
#ifdef DEBUG_LOG
            fprintf(stderr, "logdebug: invalid ident\n");
#endif
            return -1;
        }

        size_t *buffer_max = va_arg(ap, size_t *);
        *buffer_max        = lp->buffer_max;
        break;
    }
    case LOG_OPT_GET_HANDLER_BUFFERSIZEREAL: {
        log_handler_t *lp = va_arg(ap, log_handler_t *);
        if (lp == NULL) {
#ifdef DEBUG_LOG
            fprintf(stderr, "logdebug: invalid ident\n");
#endif
            return -1;
        }
        size_t *buffer_real = va_arg(ap, size_t *);
        *buffer_real        = lp->buffer_real;
        break;
    }
    case LOG_OPT_SET_HANDLER_IDENT: {
        log_handler_t *lp = va_arg(ap, log_handler_t *);
        if (lp == NULL) {
#ifdef DEBUG_LOG
            fprintf(stderr, "logdebug: invalid ident\n");
#endif
            return -1;
        }

        char *ident = va_arg(ap, char *);
        strncpy(lp->ident, ident, strlen(ident) + 1);
        break;
    }
    case LOG_OPT_GET_HANDLER_IDENT: {
        log_handler_t *lp = va_arg(ap, log_handler_t *);
        if (lp == NULL) {
#ifdef DEBUG_LOG
            fprintf(stderr, "logdebug: invalid ident\n");
#endif
            return -1;
        }
        char *ident = va_arg(ap, char *);
        if (ident) {
            strncpy(ident, lp->ident, strlen(lp->ident) + 1);
        } else {
            return -1;
        }
        break;
    }
    default:
        return -1;
    }

    return 0;
}

static size_t
log_parse_format(log_rule_t *r, log_handler_t *handler, const LOG_LEVEL_E level,
             const char *file, const char *func, const long line,
             const char *fmt, va_list ap)
{

    if (r == NULL || r->format == NULL || handler == NULL
        || handler->bufferp == NULL) {
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
#ifdef DEBUG_LOG
                fprintf(stderr, "logdebug: realloc buffer\n");
#endif
                if (handler->buffer_real * 2 <= handler->buffer_max) {
                    handler->bufferp = (char *)realloc(
                        handler->bufferp, handler->buffer_real * 2);
                    if (handler->bufferp) {
                        handler->buffer_real *= 2;
                        goto begin;
                    }

                    handler->buffer_real = 0;
#ifdef DEBUG_LOG
                    fprintf(stderr, "logdebug: realloc failed\n");
#endif
                    goto err;
                } else {
                    handler->bufferp =
                        (char *)realloc(handler->bufferp, handler->buffer_max);
                    if (handler->bufferp) {
                        handler->buffer_real = handler->buffer_max;
                        goto begin;
                    }

                    handler->buffer_real = 0;
#ifdef DEBUG_LOG
                    fprintf(stderr, "logdebug: realloc failed\n");
#endif
                    goto err;
                }
            } else {
                snprintf(buf + len - 13, 13, "%s", "(truncated)\n");
#ifdef DEBUG_LOG
                fprintf(stderr, "logdebug: msg too long, truncated to %u.\n",
                        (unsigned)idx);
#endif
                goto end;
            }
        }

        if (*p == '%') {
            nread = 0;
            nscan = sscanf(p, "%%%[.0-9]%n", format_buf, &nread);
            if (nscan == 1) {
#ifdef DEBUG_LOG
                fprintf(stderr, "logdebug: parse format [%s] failed.\n", p);
#endif
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
#ifdef DEBUG_LOG
                    fprintf(stderr,
                            "logdebug: parse foramt [%s] "
                            "failed, can't find "
                            "\')\'.\n",
                            p);
#endif
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
#ifdef DEBUG_LOG
                        fprintf(stderr,
                                "logdebug: parse format "
                                "[%s] failed, can't "
                                "find \')\''.\n",
                                p);
#endif
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
log_update_stat(const LOG_LEVEL_E level, size_t len, log_output_t *output)
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
     const char *function, const long line, const char *format, va_list args)
{
    uint16_t i;
    log_rules *pr;
    int ret;
    LOG_LEVEL_E level;

    if (handler == NULL)
        return;

    level = lvl;
    if (level > LOG_VERBOSE)
        level = LOG_VERBOSE;
    if (lvl < LOG_EMERG)
        level = LOG_EMERG;

    pthread_mutex_lock(&handler->mutex);
    list_for_each_entry(pr, &(handler->rules), l)
    {
        log_rule_t *r = pr->rule;
        if (r->level_begin < level || r->level_end > level) {
            continue;
        }

        va_list ap;
        va_copy(ap, args);
        size_t len =
            log_parse_format(r, handler, level, file, function, line, format, ap);
        va_end(ap);
        if (len <= 0) {
            continue;
        }

        r->output->emit(handler, level, len);

#if 0
        switch (r->output->type) {
        case LOG_OUTTYPE_STDOUT:
            r->output->emit(handler, level, len);
            fprintf(stdout, "%s", handler->bufferp);
            fflush(stdout);
            update_stat(level, len, r->output);
            break;

        case LOG_OUTTYPE_STDERR:
            fprintf(stderr, "%s", handler->bufferp);
            update_stat(level, len, r->output);
            break;

        case LOG_OUTTYPE_FILE: {
            file_output_config *config = (file_output_config *)r->output->config;
            if (!config) {
                continue;
            }
            if (config->fp == NULL) {
                ret = log_open_logfile(r->poutput);
                if (ret < 0) {
#ifdef DEBUG_LOG
                    fprintf(stderr, "logdebug: open logfile failed\n");
#endif
                    continue;
                }
            }

            if (r->poutput->u.file.fp) {
                int nwrite = r->poutput->u.file.file_size
                             - r->poutput->u.file.data_offset;
                if (nwrite <= len) {
                    char *str = strndup(handler->bufferp, nwrite);
                    if (!str) {
#ifdef DEBUG_LOG
                        perror("logdebug: strndup");
#endif
                        continue;
                    }
                    if (fprintf(r->poutput->u.file.fp, "%s", str) != nwrite) {
#ifdef DEBUG_LOG
                        perror("logdebug: fprintf");
#endif
                    }
                    r->poutput->u.file.data_offset += nwrite;
                    free(str);

                    log_rename_logfile(r->poutput);

                    ret = log_open_logfile(r->poutput);
                    if (ret != 0) {
#ifdef DEBUG_LOG
                        fprintf(stderr, "logdebug: open logfile failed\n");
#endif
                        continue;
                    }

                    if (fprintf(r->poutput->u.file.fp, "%s",
                                handler->bufferp + nwrite)
                        != len - nwrite) {
#ifdef DEBUG_LOG
                        perror("logdebug: fprintf ");
#endif
                    }
                    r->poutput->u.file.data_offset += (len - nwrite);
                } else {
                    if (fprintf(r->poutput->u.file.fp, "%s", handler->bufferp)
                        != (int)len) {
                        if (errno != 0) {
#ifdef DEBUG_LOG
                            perror("logdebug: fprintf");
#endif
                        }
                    }
                    r->poutput->u.file.data_offset += len;
                }
                update_stat(level, len, r->poutput);

                /* fflush(r->poutput->u.file.fp); */
            }
            break;
        }

        case LOG_OUTTYPE_TCP:
        case LOG_OUTTYPE_UDP: {
            if (r->poutput->u.sock.sockfd != -1) {
                int total = 0;
                int nsend = 0;
                while (total < len) {
                    nsend = send(r->poutput->u.sock.sockfd,
                                 handler->bufferp + total, len - total,
                                 MSG_NOSIGNAL);
                    if (nsend < 0) {
                        if (errno == EAGAIN || errno == EINTR) {
                            continue;
                        } else {
#ifdef DEBUG_LOG
                            perror("logdebug: send");
#endif
                            close(r->poutput->u.sock.sockfd);
                            r->poutput->u.sock.sockfd = -1;
                            break;
                        }
                    } else if (nsend == 0) {
#ifdef DEBUG_LOG
                        fprintf(stderr, "logdebug: sock closed\n");
#endif
                        close(r->poutput->u.sock.sockfd);
                        r->poutput->u.sock.sockfd = -1;
                        break;
                    } else {
                        total += nsend;
                    }
                }
                update_stat(level, len, r->poutput);
            }
            break;
        }

        case LOG_OUTTYPE_LOGCAT: {
#ifdef ANDROID
            android_LogPriority l;
            switch (level) {
            case LOG_VERBOSE:
                l = ANDROID_LOG_VERBOSE;
                break;
            case LOG_DEBUG:
                l = ANDROID_LOG_DEBUG;
                break;
            case LOG_INFO:
                l = ANDROID_LOG_INFO;
                break;
            case LOG_WARNING:
                l = ANDROID_LOG_WARN;
                break;
            case LOG_ERROR:
                l = ANDROID_LOG_ERROR;
                break;
            case LOG_FATAL:
                l = ANDROID_LOG_FATAL;
                break;
            case LOG_ALERT:
                l = ANDROID_LOG_FATAL;
                break;
            case LOG_EMERG:
                l = ANDROID_LOG_SILENT;
                break;
            default:
                l = ANDROID_LOG_DEFAULT;
                break;
            }
            __android_log_vprint(l, r->indent, format, args);
            update_stat(level, len, r->poutput);
#endif
            break;
        }

        case LOG_OUTTYPE_SYSLOG:
            syslog(level, "%s", handler->bufferp);
            update_stat(level, len, r->poutput);
            break;

        default:
            break;
        }
#endif
    }
    pthread_mutex_unlock(&handler->mutex);
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
    log_handler_t *lp = log_handler_get(ident);
    if (lp == NULL) {
        lp = (log_handler_t *)malloc(sizeof(log_handler_t));
        if (lp != NULL) {
            pthread_mutex_init(&lp->mutex, NULL);
            strncpy(lp->ident, ident, strlen(ident) + 1);
            lp->buffer_max  = BUFFER_MAX;
            lp->buffer_min  = BUFFER_MIN;
            lp->buffer_real = BUFFER_MIN;
            lp->bufferp     = (char *)calloc(1, BUFFER_MIN);
            if (lp->bufferp == NULL) {
#ifdef DEBUG_LOG
                fprintf(stderr, "logdebug: calloc failed\n");
#endif
                free(lp);
                return NULL;
            }

            INIT_LIST_HEAD(&lp->rules);
            list_add_tail(&lp->l, &handler_header);
        }
    }
    return lp;
}

log_handler_t *
log_handler_get(const char *ident)
{
    log_handler_t *handler = NULL;
    list_for_each_entry(handler, &handler_header, l)
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
            list_add_tail(&fp->l, &format_header);
            return fp;
        } else {
#ifdef DEBUG_LOG
            fprintf(stderr, "logdebug: alloc failed\n");
#endif
            return NULL;
        }
    }
    return NULL;
}

log_format_t *
log_format_get_default()
{
    if (list_empty(&format_header)) {
        return NULL;
    } else {
        return list_entry(format_header.next, log_format_t, l);
    }
}

static log_output_t*
log_output_create_(enum LOG_OUTTYPE type, va_list ap)
{

    log_output_t*output = NULL;
    #if 0
    switch (type) {
    case LOG_OUTTYPE_STDOUT:
    case LOG_OUTTYPE_STDERR:
    case LOG_OUTTYPE_LOGCAT:
    case LOG_OUTTYPE_SYSLOG:
        output = (log_output_t*)calloc(1, sizeof(log_output_t));
        if (output) {
            pthread_mutex_init(&output->mutex, NULL);
            output->type = type;
            list_add_tail(&output->l, &output_header);
            return output;
        } else {
#ifdef DEBUG_LOG
            fprintf(stderr, "logdebug: calloc failed\n");
#endif
            return NULL;
        }
        break;
    case LOG_OUTTYPE_FILE:
        output = (log_output_t*)calloc(1, sizeof(log_output_t));
        if (output) {
            file_output_config *config = (file_output_config *)calloc(1, sizeof(file_output_config));
            if (!config) {
                free(output);
                return NULL;
            }
            output->config = config;
            pthread_mutex_init(&output->mutex, NULL);
            output->type = type;

            char *file_path = va_arg(ap, char *);
            if (file_path && strlen(file_path) > 0) {
                config->file_path = strdup(file_path);
            } else {
                output->u.file.file_path = strdup(DEFAULT_FILEPATH);
            }

            char *file_name = va_arg(ap, char *);
            if (file_name && strlen(file_name) > 0) {
                output->u.file.log_name = strdup(file_name);
            } else {
                output->u.file.log_name = strdup(DEFAULT_FILENAME);
            }

            unsigned long file_size = va_arg(ap, unsigned long);
            if (file_size > 0) {
                output->u.file.file_size = file_size;
            } else {
                output->u.file.file_size = DEFAULT_FILESIZE;
            }

            int num_files = va_arg(ap, int);
            if (num_files >= 0) {
                output->u.file.num_files = num_files;
            } else {
                output->u.file.num_files = DEFAULT_BAKUP;
            }

            output->u.file.fp       = NULL;
            output->u.file.file_idx = 0;
            list_add_tail(&output->l, &output_header);
            return output;
        } else {
#ifdef DEBUG_LOG
            fprintf(stderr, "logdebug: calloc failed\n");
#endif
            return NULL;
        }
        break;

    case LOG_OUTTYPE_UDP:
    case LOG_OUTTYPE_TCP:
        output = (log_output_t*)calloc(1, sizeof(logoutput));
        if (output) {
            pthread_mutex_init(&output->mutex, NULL);
            output->type = type;
            char *addr   = va_arg(ap, char *);
            if (addr && strlen(addr) > 0) {
                strncpy(output->u.sock.addr, addr, strlen(addr) + 1);
            } else {
                strncpy(output->u.sock.addr, DEFAULT_SOCKADDR,
                        strlen(DEFAULT_SOCKADDR) + 1);
            }

            unsigned port = va_arg(ap, unsigned);
            if (port > 0 && port < 65535) {
                output->u.sock.port = port;
            } else {
                output->u.sock.port = DEFAULT_SOCKPORT;
            }
            output->u.sock.sockfd = -1;

            if (output->u.sock.sockfd == -1) {
                struct hostent *host = NULL;
                if ((host = gethostbyname(output->u.sock.addr)) != NULL) {
                    if (output->type == LOG_OUTTYPE_TCP) {
                        output->u.sock.sockfd = socket(AF_INET, SOCK_STREAM, 0);
                    } else {
                        output->u.sock.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                    }

                    if (output->u.sock.sockfd != -1) {
                        struct sockaddr_in addr;
                        memset(&addr, 0, sizeof(addr));
                        addr.sin_family = AF_INET;
                        addr.sin_port   = htons(output->u.sock.port);
                        addr.sin_addr =
                            *(struct in_addr *)(host->h_addr_list[0]);
                        if (connect(output->u.sock.sockfd,
                                    (struct sockaddr *)&addr, sizeof(addr))
                            < 0) {
#ifdef DEBUG_LOG
                            perror("logdebug: connect");
#endif
                            close(output->u.sock.sockfd);
                            output->u.sock.sockfd = -1;
                        }
                    }
                }
            }

            list_add_tail(&output->l, &output_header);
            return output;
        } else {
#ifdef DEBUG_LOG
            fprintf(stderr, "logdebug: calloc failed\n");
#endif
            return NULL;
        }
        break;
    default:
        return NULL;
    }
    #endif
    return NULL;
}

log_output_t*
log_output_create(enum LOG_OUTTYPE type, ...)
{
    log_output_t*output = NULL;
    va_list ap;
    va_start(ap, type);
    output = log_output_create_(type, ap);
    va_end(ap);
    return output;
}

log_output_t*
log_output_get_default()
{
    if (list_empty(&output_header)) {
        return NULL;
    } else {
        return list_entry(output_header.next, log_output_t, l);
    }
}

int
log_bind(log_handler_t *handler, LOG_LEVEL_E level_begin, LOG_LEVEL_E level_end,
        log_format_t *format, log_output_t*output)
{
    if (handler == NULL || format == NULL || output == NULL)
        return -1;

    log_rule_t *r = (log_rule_t *)calloc(1, sizeof(log_rule_t));
    if (r) {
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
        list_add_tail(&r->l, &rule_header);
    } else {
#ifdef DEBUG_LOG
        fprintf(stderr, "logdebug: malloc failed\n");
#endif
        return -1;
    }

    log_rules *pr = (log_rules *)calloc(1, sizeof(log_rules));
    if (pr) {
        pr->rule = r;
        list_add_tail(&pr->l, &handler->rules);
        return 0;
    } else {
        free(r);
#ifdef DEBUG_LOG
        fprintf(stderr, "logdebug: malloc failed\n");
#endif
        return -1;
    }
    return -1;
}

int
log_unbind(log_handler_t *handler, log_output_t*output)
{
    if (handler == NULL || output == NULL) {
        return -1;
    }

    log_rules *pr;
    list_for_each_entry(pr, &(handler->rules), l)
    {
        if (pr->rule->output == output) {
            list_del(&pr->l);
            return 0;
        }
    }
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
     const char *format, ...)
{
    log_handler_t *handler = log_handler_get(DEFAULT_IDENT);
    if (handler) {
        va_list args;
        va_start(args, format);
        vlog(handler, level, file, function, line, format, args);
        va_end(args);
    }
    return;
}

static void
log_dump_stat(log_output_t*output)
{
    int i;
    for (i = LOG_VERBOSE; i >= LOG_EMERG; i--) {
        printf("%-8s  count: %-8d  bytes: %-10lu\n", LOGLEVELSTR[i],
               output->stat.stats[i].count,
               output->stat.stats[i].bytes);
    }
    printf("%-8s  count: %-8lu  bytes: %-10lu\n", "TOTAL",
           output->stat.count_total, output->stat.bytes_total);
}

void
log_dump(void)
{
    int i = 0;
    int j = 0;
    printf("=====================log profile==============================\n");
    log_rule_t *ru;
    list_for_each_entry(ru, &rule_header, l) { i++; }
    printf("ctx: rule: %d", i);
    i = 0;
    log_format_t *format;
    list_for_each_entry(format, &format_header, l) { i++; }
    printf(" format: %d", i);
    i = 0;
    log_output_t*output;
    list_for_each_entry(output, &output_header, l) { i++; }
    printf(" output: %d\n", i);
    i = 0;
    log_handler_t *handler;
    list_for_each_entry(handler, &handler_header, l)
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
        log_rules *pr;
        list_for_each_entry(pr, &handler->rules, l)
        {
            j++;
            log_rule_t *r = pr->rule;
            if (r) {
                printf("rule: %d\n", j);
                #if 0
                switch (r->output->type) {
                case LOG_OUTTYPE_STDOUT:
                    printf("type: stdout\n");
                    printf("format: %s\n", r->format->format);
                    printf("level: %s -- %s\n", LOGLEVELSTR[r->level_begin],
                           LOGLEVELSTR[r->level_end]);
                    break;
                case LOG_OUTTYPE_STDERR:
                    printf("type: stdout\n");
                    printf("format: %s\n", r->format->format);
                    printf("level: %s\n", LOGLEVELSTR[r->level_begin]);
                    break;
                case LOG_OUTTYPE_FILE:
                    printf("type: file\n");
                    printf("format: %s\n", r->format->format);
                    printf("level: %s -- %s\n", LOGLEVELSTR[r->level_begin],
                           LOGLEVELSTR[r->level_end]);
                    printf("filepath: %s\n", r->output->u.file.file_path);
                    printf("filename: %s\n", r->output->u.file.log_name);
                    printf("filesize: %u\n", r->output->u.file.file_size);
                    printf("bakup: %d\n", r->poutput->u.file.num_files);
                    printf("idx: %d\n", r->poutput->u.file.file_idx);
                    printf("offset: %u\n", r->poutput->u.file.data_offset);
                    break;
                case LOG_OUTTYPE_UDP:
                case LOG_OUTTYPE_TCP:
                    printf("type: socket %s\n",
                           r->poutput->type == LOG_OUTTYPE_TCP ? "tcp" : "udp");
                    printf("format: %s\n", r->pformat->format);
                    printf("level: %s -- %s\n", LOGLEVELSTR[r->level_begin],
                           LOGLEVELSTR[r->level_end]);
                    printf("addr: %s:%d\n", r->poutput->u.sock.addr,
                           r->poutput->u.sock.port);
                    break;
                case LOG_OUTTYPE_LOGCAT:
                    printf("type: logcat\n");
                    printf("format: %s\n", r->pformat->format);
                    printf("level: %s -- %s\n", LOGLEVELSTR[r->level_begin],
                           LOGLEVELSTR[r->level_end]);
                    break;
                case LOG_OUTTYPE_SYSLOG:
                    printf("type: syslog\n");
                    printf("format: %s\n", r->pformat->format);
                    printf("level: %s -- %s\n", LOGLEVELSTR[r->level_begin],
                           LOGLEVELSTR[r->level_end]);
                    break;
                case LOG_OUTTYPE_NONE:
                    break;
                default:
                    printf("type: unknown\n");
                    break;
                }
                #endif
                log_dump_stat(r->output);

                printf("\n");
            }
        }
    }
}
