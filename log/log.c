/**
 *   @file log.cpp
 *   @brief log
 *
 *   @author liyunteng <liyunteng@streamocean.com>
 *   @copyright CopyRight (C) 2015 StreamOcean
 *   @date Update time:  2016/09/04 02:30:11
 */

#ifdef __cplusplus
#error  "please use gcc to compile"
#ifndef __STDC_FORMAT_MARCOS
#define __STDC_FORMAT_MARCOS
#endif
#endif

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <syslog.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#include <fcntl.h>

#include "log.h"
#include "list.h"

#define ESC_START       "\033["
#define ESC_END         "\033[0m"
#define COLOR_EMERG     ESC_START"31;40;7m"
#define COLOR_ALERT     ESC_START"31;40;6m"
#define COLOR_FATAL     ESC_START"31;40;1m"
#define COLOR_ERROR     ESC_START"33;40;1m"
#define COLOR_WARNING   ESC_START"35;40;1m"
#define COLOR_NOTICE    ESC_START"32;40;1m"
#define COLOR_INFO      ESC_START"37;40;1m"
//#define COLOR_DEBUG    ESC_START"37;40;0m"
#define COLOR_DEBUG     ESC_START"00;00;0m"

#define BAK_LOCK_FILE ".bak.lock"
#define BUFFER_MIN      1024 * 4
#define BUFFER_MAX      1024 * 1024 * 4

#define DEBUG

struct _logformat {
    char format[128];
    char time_format[32];
    char print_fmt[128];
    bool color;
    struct list_head l;
};

struct _logoutput {
    pthread_mutex_t mutex;
    enum LOGOUTTYPE type;
    struct list_head l;
    union {
        struct {
            char filename[256];
            mode_t filemode;
            int bakup;
            uint64_t filesize;
            FILE *fp;
        } file;
        struct {
            char addr[256];
            unsigned port;
            int sockfd;
        } sock;

    } u;
};

typedef struct _logrule {
    LOGLEVEL level_begin;
    LOGLEVEL level_end;
    logoutput *poutput;
    logformat *pformat;
    struct list_head l;
} logrule;

typedef struct _prule {
    logrule *prule;
    struct list_head l;
} prule;

struct  _loghandler {
    pthread_mutex_t mutex;
    char ident[128];

    char *bufferp;
    size_t buffer_max;
    size_t buffer_min;
    size_t buffer_real;

    struct list_head rules; // prule
    struct list_head l;
};

static const char * const LOGLEVELSTR[] = {
    "EMERG",
    "ALERT",
    "FATAL",
    "ERROR",
    "WARN",
    "NOTICE",
    "INFO",
    "DEBUG",
};

static struct list_head output_header;
static struct list_head format_header;
static struct list_head rule_header;
static struct list_head handler_header;
static loghandler *default_handler;

static inline int lock_get(int fd, int type, int start, int len, int whence)
{
    struct flock lock;
    lock.l_type = type;
    lock.l_whence = whence;
    lock.l_start = start;
    lock.l_len = len;

    fcntl(fd, F_GETLK, &lock);
    if (lock.l_type == F_UNLCK) {
        return 0;
    }
    return -1;
}

static inline int lock_set(int fd, int type, int start, int len, int whence)
{
    struct flock lock;
    lock.l_type = type;
    lock.l_whence = whence;
    lock.l_start = start;
    lock.l_len = len;

    if (fcntl(fd, F_SETLK, &lock) != 0) {
        if (type == F_UNLCK) {
            //perror("unlock file failed");
        } else {
            //perror("lock file failed");
        }
        return -1;
    }
    return 0;
}

static void _dobak(logoutput *out)
{
    int bakcnt;
    char old_bakfile[256];
    char new_bakfile[256];

    int fd;
    fd = open(BAK_LOCK_FILE, O_RDWR|O_CREAT, DEFAULT_FILEMODE);
    if (fd < 0) {
        perror("open lockfile failed");
        return;
    }

    if(lock_set(fd, F_WRLCK, 0, 0, SEEK_SET) == 0) {
#ifdef DEBUG
        fprintf(stderr, "log: do bak\n");
#endif
        bakcnt = out->u.file.bakup;
        while (bakcnt >= 0) {
            if (bakcnt == 0) {
                snprintf(old_bakfile, sizeof(old_bakfile), "%s", out->u.file.filename);
            } else {
                snprintf(old_bakfile, sizeof(old_bakfile), "%s.%d", out->u.file.filename, bakcnt-1);
            }
            if(!access(old_bakfile, F_OK)) {
                if (bakcnt == out->u.file.bakup) {
                    if (unlink(old_bakfile) != 0) {
                        perror("unlink");
                    }
                    /* fprintf(stderr, "unlink %s bakcnt: %d\n", old_bakfile, bakcnt); */
                    continue;
                } else {
                    snprintf(new_bakfile, sizeof(new_bakfile), "%s.%d", out->u.file.filename, bakcnt);
                    /* fprintf(stderr, "rename %s to %s bakcnt: %d\n", old_bakfile, new_bakfile, bakcnt); */
                    if (rename(old_bakfile, new_bakfile) != 0) {
                        perror("rename");
                    }

                }
            }
            bakcnt--;
        }

        if (lock_set(fd, F_UNLCK, 0, 0, SEEK_SET) != 0) {
            close(fd);
            return;
        }

    } else {
#ifdef DEBUG
        fprintf(stderr, "log: wait other process bak\n");
#endif
        int ret = 1;
        do {
            ret = lock_get(fd, F_WRLCK, 0, 0, SEEK_SET);
        } while (ret);
    }
    close(fd);
}

static void checkfile(logoutput *out, size_t len)
{
    if (out->u.file.fp == NULL) {
        mode_t mask_old = umask(~(out->u.file.filemode));
        out->u.file.fp = fopen(out->u.file.filename, "a+");
        umask(mask_old);
        if (out->u.file.fp == NULL) {
            fprintf(stderr, "create file %s failed\n", out->u.file.filename);
            return ;
        }
    }

    /* dont bak */
    if (out->u.file.bakup <= 0) {
        return;
    }

    /* limit by size */
    struct stat st;
    if (fstat(fileno(out->u.file.fp), &st) == 0) {
        if ((st.st_size + len  >= out->u.file.filesize)) {
            if (out->u.file.fp != NULL) {
                fsync(fileno(out->u.file.fp));
                fclose(out->u.file.fp);
                out->u.file.fp = NULL;
            }

            _dobak(out);

            if (out->u.file.fp == NULL) {
                mode_t mask_old = umask(~(out->u.file.filemode));
                out->u.file.fp = fopen(out->u.file.filename, "a+");
                umask(mask_old);
            }
        }
    } else {
        fprintf(stderr, "fstat failed\n");
    }

    return;
}

static int __log_ctl(enum LOG_OPTS opt, va_list ap)
{
    switch (opt) {
    case LOG_OPT_SET_HANDLER_BUFFERSIZEMIN:
    {
        loghandler *lp = va_arg(ap, loghandler *);
        if (lp == NULL) {
            fprintf(stderr, "invalid ident\n");
            return -1;
        }
        size_t buffer_min = va_arg(ap, size_t);
        lp->buffer_min = buffer_min;
        if (lp->buffer_real < buffer_min)
            lp->bufferp = (char *)realloc(lp->bufferp, buffer_min);
        if (lp->bufferp == NULL) {
            lp->buffer_real = 0;
            fprintf(stderr, "realloc failed\n");
            return -1;
        }
        if (lp->buffer_real < buffer_min)
            lp->buffer_real = buffer_min;

        break;
    }

    case LOG_OPT_SET_HANDLER_BUFFERSIZEMAX:
    {
        loghandler *lp = va_arg(ap, loghandler *);
        if (lp == NULL) {
            fprintf(stderr, "invalid ident\n");
            return -1;
        }

        size_t buffer_max = va_arg(ap, size_t);
        lp->buffer_max = buffer_max;
        if (lp->buffer_real > buffer_max)
            lp->bufferp = (char *)realloc(lp->bufferp, buffer_max);
        if (lp->bufferp == NULL) {
            lp->buffer_real = 0;
            fprintf(stderr, "realloc failed\n");
            return -1;
        }
        if (lp->buffer_real > buffer_max)
            lp->buffer_real = buffer_max;

        break;
    }
    case LOG_OPT_GET_HANDLER_BUFFERSIZEMIN:
    {
        loghandler *lp = va_arg(ap, loghandler *);
        if (lp == NULL) {
            fprintf(stderr, "invalid ident\n");
            return -1;
        }

        size_t *buffer_min = va_arg(ap, size_t *);
        *buffer_min = lp->buffer_min;
        break;
    }
    case LOG_OPT_GET_HANDLER_BUFFERSIZEMAX:
    {
        loghandler *lp = va_arg(ap, loghandler *);
        if (lp == NULL) {
            fprintf(stderr, "invalid ident\n");
            return -1;
        }

        size_t *buffer_max = va_arg(ap, size_t *);
        *buffer_max = lp->buffer_max;
        break;
    }
    case LOG_OPT_GET_HANDLER_BUFFERSIZEREAL:
    {
        loghandler *lp = va_arg(ap, loghandler *);
        if (lp == NULL) {
            fprintf(stderr, "invalid ident\n");
            return -1;
        }
        size_t *buffer_real =  va_arg(ap, size_t *);
        *buffer_real = lp->buffer_real;
        break;
    }
    case LOG_OPT_SET_HANDLER_IDENT:
    {
        loghandler *lp = va_arg(ap, loghandler *);
        if (lp == NULL) {
            fprintf(stderr, "invalid ident\n");
            return -1;
        }

        char *ident = va_arg(ap, char *);
        strncpy(lp->ident, ident, strlen(ident)+1);
        break;
    }
    case LOG_OPT_GET_HANDLER_IDENT:
    {
        loghandler *lp = va_arg(ap, loghandler *);
        if (lp == NULL) {
            fprintf(stderr, "invalid ident\n");
            return -1;
        }
        char *ident = va_arg(ap, char *);
        if (ident) {
            strncpy(ident, lp->ident, strlen(lp->ident)+1);
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

static size_t parse_format(logrule *r,
                           loghandler *handler, LOGLEVEL level,
                           const char *file, const char *func,  const long line,
                           const char *fmt, va_list ap)
{

    if (r == NULL || r->pformat == NULL || handler == NULL || handler->bufferp == NULL) {
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
    idx = 0;


    if (r->pformat->color) {
        switch (level) {
        case LOG_EMERG:
            idx += snprintf(buf+idx, len-idx, "%s", COLOR_EMERG);
            break;
        case LOG_ALERT:
            idx += snprintf(buf+idx, len-idx, "%s", COLOR_ALERT);
            break;
        case LOG_FATAL:
            idx += snprintf(buf+idx, len-idx, "%s", COLOR_FATAL);
            break;
        case LOG_ERROR:
            idx += snprintf(buf+idx, len-idx, "%s", COLOR_ERROR);
            break;
        case LOG_WARNING:
            idx += snprintf(buf+idx, len-idx, "%s", COLOR_WARNING);
            break;
        case LOG_NOTICE:
            idx += snprintf(buf+idx, len-idx, "%s", COLOR_NOTICE);
            break;
        case LOG_INFO:
            idx += snprintf(buf+idx, len-idx, "%s", COLOR_INFO);
            break;
        case LOG_DEBUG:
            idx += snprintf(buf+idx, len-idx, "%s", COLOR_DEBUG);
            break;
        default:
            idx += snprintf(buf+idx, len-idx, "%s", COLOR_DEBUG);
            break;
        }
    }

    char *p = r->pformat->format;
    char format_buf[128];
    while (*p) {

        if (idx >= len) {
            if (handler->buffer_real < handler->buffer_max) {
                fprintf(stderr, "realloc buffer\n");
                if (handler->buffer_real*2 <= handler->buffer_max) {
                    handler->bufferp = (char *)realloc(handler->bufferp, handler->buffer_real * 2);
                    if (handler->bufferp) {
                        handler->buffer_real *= 2;
                        goto begin;
                    }

                    handler->buffer_real = 0;
                    fprintf(stderr, "realloc failed\n");
                    goto err;
                } else {
                    handler->bufferp = (char *)realloc(handler->bufferp, handler->buffer_max);
                    if (handler->bufferp) {
                        handler->buffer_real = handler->buffer_max;
                        goto begin;
                    }

                    handler->buffer_real = 0;
                    fprintf(stderr, "realloc failed\n");
                    goto err;
                }
            } else  {
                fprintf(stderr, "msg too long, truncated to %u.\n", (unsigned)idx);
                goto end;
            }
        }


        if (*p == '%') {
            nread = 0;
            nscan = sscanf(p, "%%%[.0-9]%n", format_buf, &nread);
            if (nscan == 1) {
                fprintf(stderr, "parse format [%s] failed.\n",p);
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
                    fprintf(stderr, "parse foramt [%s] failed, can't find \')\'.\n", p);
                    goto err;
                }
                idx += snprintf(buf+idx, len-idx, "%s", getenv(env));
                continue;
            }

            if (*p == 'd') {
                if (*(p+1) != '(')  {
                    strcpy(r->pformat->time_format, DEFAULT_TIME_FORMAT);
                    p++;
                } else if (strncmp(p, "d()", 3) == 0) {
                    strcpy(r->pformat->time_format, DEFAULT_TIME_FORMAT);
                    p += 3;
                } else {
                    nread = 0;
                    nscan = sscanf(p, "d(%[^)])%n", r->pformat->time_format, &nread);
                    if (nscan != 1) {
                        nread = 0;
                    }
                    p += nread;
                    if (*(p - 1) != ')') {
                        fprintf(stderr, "parse format [%s] failed, can't find \')\''.\n", p);
                        goto err;
                    }
                }

                t = time(NULL);
                localtime_r(&t, &now);
                idx += strftime(buf+idx, len-idx,  r->pformat->time_format, &now);
                continue;
            }

            if (strncmp(p, "ms", 2) == 0) {
                p += 2;
                struct timeval tv;
                gettimeofday(&tv, NULL);
                idx += snprintf(buf+idx, len-idx, "%3d", (int)(tv.tv_usec/1000));
                continue;
            }

            if (strncmp(p, "us", 2) == 0) {
                p += 2;
                struct timeval tv;
                gettimeofday(&tv, NULL);
                idx += snprintf(buf+idx, len-idx, "%6d", (int)(tv.tv_usec));
                continue;
            }

            switch (*p) {
            case 'D':
                t = time(NULL);
                localtime_r(&t, &now);
                idx += strftime(buf+idx, len-idx,  "%F", &now);
                break;
            case 'T':
                t = time(NULL);
                localtime_r(&t, &now);
                idx += strftime(buf+idx, len-idx, "%T", &now);
                break;
            case 'F':
                idx += snprintf(buf+idx, len-idx, "%s", file);
                break;
            case 'U':
                idx += snprintf(buf+idx, len-idx, "%s", func);
                break;
            case 'L':
                idx += snprintf(buf+idx, len-idx, "%ld", line);
                break;
            case 'n':
                idx += snprintf(buf+idx, len-idx, "%c", '\n');
                break;
            case 'p':
                idx += snprintf(buf+idx, len-idx, "%u", getpid());
                break;
            case 'm':
                idx += vsnprintf(buf+idx, len-idx, fmt, local_ap);
                break;
            case 'c':
                idx += snprintf(buf+idx, len-idx, "%s", handler->ident);
                break;
            case 'V':
                idx += snprintf(buf+idx, len-idx, "%5.5s", LOGLEVELSTR[level]);
                break;
            case 'H':
                gethostname(hostname, 128);
                idx +=  snprintf(buf+idx, len-idx, "%s", hostname);
                break;
            case 't':
                idx += snprintf(buf+idx, len-idx, "%lu", pthread_self());
                break;
            case '%':
                idx += snprintf(buf+idx, len-idx, "%c", *p);
                break;
            default:
                idx += snprintf(buf+idx, len-idx, "%c", *p);
                break;
            }
        } else {
            idx += snprintf(buf+idx, len-idx, "%c", *p);
        }

        p++;
    }

  end:
    if (r->pformat->color) {
        idx += snprintf(buf+idx, len-idx, "%s", ESC_END);
    }
    return idx;

  err:
    return 0;
}


static void vlog(loghandler *handler, LOGLEVEL level,
                 const char *file, const char *function,  long line,
                 const char *format, va_list args) {


    if (handler == NULL)
        return;

    pthread_mutex_lock(&handler->mutex);
#if 0
    int idx = 0;
    if (handle->flags & LOGFLAG_VERBOSE) {
        char timebuf[24];
        time_t t= time(NULL);
        struct tm now;
        localtime_r(&t, &now);
        strftime(timebuf, sizeof(timebuf), handle->format, &now);
        idx = snprintf(handle->bufferp, handle->buffer_size, "%s [%-5.5s]  %s %s():%ld ",
                       timebuf, LOGLEVELSTR[level], file, function, line);
    }

    int n = vsnprintf(handle->bufferp + idx, handle->buffer_size - idx, format, args);
    if (n < 0) {
        fprintf(stderr, "vsnprintf failed\n");
        pthread_mutex_unlock(&handle->mutex);
        return ;
    }

    size_t len = n + idx;
    if (len >= handle->buffer_size) {
        fprintf(stderr, "msg too long, truancated.\n");
        len = handle->buffer_size - 1;
        handle->bufferp[len-1] = '\n';
        handle->bufferp[len] = '\0';
    } else {
        handle->bufferp[len] = '\n';
        len++;
        handle->bufferp[len] = '\0';
    }
#endif
#if 0
    int n = vsnprintf(handle->bufferp, handle->buffer_size, format, args);
    if (n < 0) {
        fprintf(stderr, "vsnprintf failed\n");
        pthread_mutex_unlock(&handle->mutex);
        return;
    }
    handle->bufferp[n] = '\n';
    handle->bufferp[n+1] = '\0';
    size_t len = n+1;
#endif

    uint16_t i;
    prule *pr;
    list_for_each_entry (pr, &(handler->rules), l) {
        if (level > LOGLEVEL_DEBUG)
            level = LOGLEVEL_DEBUG;
        if (level < LOGLEVEL_EMERG)
            level = LOGLEVEL_EMERG;

        logrule *r = pr->prule;
        if (r->level_begin < level || r->level_end > level) {
            continue;
        }

        va_list ap;
        va_copy(ap, args);
        size_t len = parse_format(r, handler, level, file, function, line, format, ap);
        va_end(ap);

        switch (r->poutput->type) {
        case LOGOUTTYPE_STDOUT:
            if (len > 0) {
                fprintf(stdout, "%s", handler->bufferp);
                fflush(stdout);
            }
            break;
        case LOGOUTTYPE_STDERR:
            if (len > 0) {
                fprintf(stderr, "%s", handler->bufferp);
            }
            break;
        case LOGOUTTYPE_FILE:
            if (len > 0) {
                checkfile(r->poutput, len);
                if (r->poutput->u.file.fp) {
                    fseek(r->poutput->u.file.fp, 0, SEEK_END);
                    if (fprintf(r->poutput->u.file.fp, "%s", handler->bufferp) != (int)len) {
                        if (errno != 0) {
                            perror("fprintf");
                        }
                    }
                    fflush(r->poutput->u.file.fp);
                }
            }
            break;
        case LOGOUTTYPE_SOCK:
            if (len > 0) {

                if (r->poutput->u.sock.sockfd == -1) {
                    struct hostent *host = NULL;
                    if ((host = gethostbyname(r->poutput->u.sock.addr))) {
                        r->poutput->u.sock.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                        if (r->poutput->u.sock.sockfd != -1) {
                            struct sockaddr_in addr;
                            memset(&addr, 0, sizeof(addr));
                            addr.sin_family = AF_INET;
                            addr.sin_port = htons(r->poutput->u.sock.port);
                            addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];
                            bind(r->poutput->u.sock.sockfd, (struct sockaddr *)&addr, sizeof(addr));
                        }
                    }
                }
                if (r->poutput->u.sock.sockfd != -1)
                    send(r->poutput->u.sock.sockfd, handler->bufferp, len+1, 0);
            }
            break;
        case LOGOUTTYPE_LOGCAT:
            if (len > 0) {
#if defined ANDROID
                android_LogPriority l;
                switch (level) {
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
                __android_log_vprint(l, "ihi", format, args);
#endif
            }
            break;
        case LOGOUTTYPE_SYSLOG:
            if (len > 0) {
                syslog(level, "%s", handler->bufferp);
            }
            break;
        default:
            break;
        }
    }
    pthread_mutex_unlock(&handler->mutex);
}


int log_init()
{
    INIT_LIST_HEAD(&format_header);
    INIT_LIST_HEAD(&output_header);
    INIT_LIST_HEAD(&rule_header);
    INIT_LIST_HEAD(&handler_header);

    logformat *default_format = logformat_create(DEFAULT_FORMAT, FALSE);
    logoutput *default_output = logoutput_create(LOGOUTTYPE_STDOUT);
    /* logoutput *file_output = logoutput_create(LOGOUTTYPE_FILE, "ihi.log", DEFAULT_FILESIZE, DEFAULT_FILEMODE, DEFAULT_BAKUP); */
    default_handler = loghandler_create(DEFAULT_IDENT);

    if (default_handler && default_format && default_output) {
        logbind(default_handler, DEFAULT_LEVEL, -1, default_format, default_output);
        /* logbind(default_handler, DEFAULT_LEVEL, -1, default_format, file_output); */
        return 0;
    } else {
        return -1;
    }
}

int log_ctl(enum LOG_OPTS opt, ...)
{
    va_list ap;
    va_start(ap, opt);
    int ret = __log_ctl(opt, ap);
    va_end(ap);
    return ret;
}


loghandler *loghandler_create(const char *ident)
{
    loghandler *lp = (loghandler *)malloc(sizeof(loghandler));
    if (lp != NULL) {
        pthread_mutex_init(&lp->mutex, NULL);
        strncpy(lp->ident, ident, strlen(ident)+1);
        lp->buffer_max =  BUFFER_MAX;
        lp->buffer_min =  BUFFER_MIN;
        lp->buffer_real = BUFFER_MIN;
        lp->bufferp = (char *)calloc(1, BUFFER_MIN);
        if  (lp->bufferp == NULL) {
            fprintf(stderr, "calloc failed\n");
            free(lp);
            return NULL;
        }

        INIT_LIST_HEAD(&lp->rules);
        list_add_tail(&lp->l, &handler_header);
    }
    return lp;
}

loghandler *loghandler_get(const char *ident)
{
    loghandler *handler = NULL;
    list_for_each_entry(handler, &handler_header, l) {
        if (handler && strcmp(handler->ident, ident) == 0) {
            return handler;
        }
    }
    return NULL;
}

logformat *logformat_create(const char *format, int color)
{
    logformat *fp = NULL;
    if (format) {
        fp = (logformat *)calloc(1, sizeof(logformat));
        if (fp) {
            strncpy(fp->format, format, strlen(format)+1);
            if (color) {
                fp->color = TRUE;
            } else {
                fp->color = FALSE;
            }
            list_add_tail(&fp->l, &format_header);
            return fp;
        } else {
            fprintf(stderr, "alloc failed\n");
            return NULL;
        }
    }
    return NULL;
}

logformat *logformat_get_default()
{
    if (list_empty(&format_header)) {
        return NULL;
    } else {
        return list_entry(format_header.next, logformat, l);
    }
}

static logoutput *__logoutput_create(enum LOGOUTTYPE type, va_list ap)
{

    logoutput *output = NULL;
    switch (type) {
    case LOGOUTTYPE_STDOUT:
    case LOGOUTTYPE_STDERR:
    case LOGOUTTYPE_LOGCAT:
    case LOGOUTTYPE_SYSLOG:
        output = (logoutput *) calloc(1, sizeof(logoutput));
        if (output) {
            pthread_mutex_init(&output->mutex, NULL);
            output->type = type;
            list_add_tail(&output->l, &output_header);
            return output;
        } else {
            fprintf(stderr, "calloc failed");
            return NULL;
        }
        break;
    case LOGOUTTYPE_FILE:
        output = (logoutput *) calloc(1, sizeof(logoutput));
        if (output) {
            pthread_mutex_init(&output->mutex, NULL);
            output->type = type;
            char *filename = va_arg(ap, char *);
            if (filename && strlen(filename) > 0) {
                strncpy(output->u.file.filename, filename, strlen(filename)+1);
            } else {
                strncpy(output->u.file.filename, DEFAULT_FILENAME, strlen(DEFAULT_FILENAME)+1);
            }
            unsigned long filesize = va_arg(ap, unsigned long);
            if (filesize > 0) {
                output->u.file.filesize = filesize;
            } else {
                output->u.file.filesize = DEFAULT_FILESIZE;
            }

            mode_t filemode = va_arg(ap, mode_t);
            if (filemode) {
                output->u.file.filemode = filemode;
            } else {
                output->u.file.filemode = DEFAULT_FILEMODE;
            }

            int bakup = va_arg(ap, int);
            if (bakup >= 0) {
                output->u.file.bakup = bakup;
            } else {
                output->u.file.bakup = DEFAULT_BAKUP;
            }

            list_add_tail(&output->l, &output_header);
            return output;
        } else {
            fprintf(stderr, "calloc failed");
            return NULL;
        }
        break;
    case LOGOUTTYPE_SOCK:
        output = (logoutput *) calloc(1, sizeof(logoutput));
        if (output) {
            pthread_mutex_init(&output->mutex, NULL);
            output->type = type;
            char *addr = va_arg(ap, char *);
            if (addr && strlen(addr) > 0) {
                strncpy(output->u.sock.addr, addr, strlen(addr)+1);
            } else {
                strncpy(output->u.sock.addr, DEFAULT_SOCKADDR, strlen(DEFAULT_SOCKADDR)+1);
            }

            unsigned port = va_arg(ap, unsigned);
            if (port > 0 && port < 65535) {
                output->u.sock.port = port;
            } else {
                output->u.sock.port = DEFAULT_SOCKPORT;
            }

            list_add_tail(&output->l, &output_header);
            return output;
        } else {
            fprintf(stderr, "calloc failed");
            return NULL;
        }
        break;
    default:
        return NULL;
    }
    return NULL;
}

logoutput *logoutput_create(enum LOGOUTTYPE type, ...)
{
    logoutput *output = NULL;
    va_list ap;
    va_start(ap, type);
    output = __logoutput_create(type, ap);
    va_end(ap);
    return output;
}

logoutput *logoutput_get_default()
{
    if (list_empty(&output_header)) {
        return NULL;
    } else {
        return list_entry(output_header.next, logoutput, l);
    }
}

int logbind(loghandler *handler, LOGLEVEL level_begin, LOGLEVEL level_end, logformat *format, logoutput *output)
{
    if (handler == NULL || format == NULL || output == NULL)
        return -1;

    logrule *r = (logrule *)calloc(1, sizeof(logrule));
    if (r) {
        r->level_begin = level_begin;
        if (LOGLEVEL_EMERG < level_end && LOGLEVEL_DEBUG > level_end)
            r->level_end =  level_end;
        else
            r->level_end = LOGLEVEL_EMERG;
        r->pformat = format;
        r->poutput = output;
        list_add_tail(&r->l, &rule_header);
    } else {
        fprintf(stderr, "malloc failed\n");
        return -1;
    }

    prule *pr = (prule *) calloc(1, sizeof(prule));
    if (pr) {
        pr->prule = r;
        list_add_tail(&pr->l, &handler->rules);
        return 0;
    } else {
        free(r);
        fprintf(stderr, "malloc failed\n");
        return -1;
    }
    return -1;
}

void mlog(loghandler *handle, LOGLEVEL level,
          const char *file, const char *function,  long line,
          const char *format, ...)
{

    va_list args;
    va_start(args, format);
    vlog(handle, level, file, function, line, format, args);
    va_end(args);
    return;
}

void slog(LOGLEVEL level, const char * file,
          const char * function, long line,
          const char *format, ...)
{
    if (default_handler) {
        va_list args;
        va_start(args, format);
        vlog(default_handler, level, file, function, line, format,  args);
        va_end(args);
    }
    return ;
}

void log_dump(void)
{
    int i = 0;
    int j = 0;
    printf("=====================log profile==============================\n");
    logrule *ru;
    list_for_each_entry(ru, &rule_header, l) {
        i++;
    }
    printf("ctx: rule total %d\n", i);
    i = 0;
    logformat *format;
    list_for_each_entry(format, &format_header, l) {
        i++;
    }
    printf(" format total %d\n", i);
    i = 0;
    logoutput *output;
    list_for_each_entry(output, &output_header, l) {
        i++;
    }
    printf(" output total %d\n", i);
    i = 0;
    loghandler *handler;
    list_for_each_entry(handler, &handler_header, l) {
        i++;
        j = 0;
        printf("------------------------\n");
        printf("handler: %d\n", i);
        printf("ident: %s\n", handler->ident);
        printf("buffer_min: %u\n", (unsigned)handler->buffer_min);
        printf("buffer_real: %u\n", (unsigned)handler->buffer_real);
        printf("buffer_max: %u\n", (unsigned)handler->buffer_max);
        printf("\n");
        prule *pr;
        list_for_each_entry(pr, &handler->rules, l) {
            j++;
            logrule *r = pr->prule;
            if (r) {
                printf("rule: %d\n", j);
                switch (r->poutput->type)  {
                case LOGOUTTYPE_STDOUT:
                    printf("type: stdout\n");
                    printf("format: %s\n", r->pformat->format);
                    printf("level: %s\n", LOGLEVELSTR[r->level_begin]);
                    break;
                case LOGOUTTYPE_STDERR:
                    printf("type: stdout\n");
                    printf("format: %s\n", r->pformat->format);
                    printf("level: %s\n", LOGLEVELSTR[r->level_begin]);
                    break;
                case LOGOUTTYPE_FILE:
                    printf("type: file\n");
                    printf("format: %s\n", r->pformat->format);
                    printf("level: %s\n", LOGLEVELSTR[r->level_begin]);
                    printf("filename: %s\n", r->poutput->u.file.filename);
                    printf("filemode: %u\n", r->poutput->u.file.filemode);
                    printf("bakup: %"PRIu16"\n", r->poutput->u.file.bakup);
                    printf("filesize: %"PRIu64"\n", r->poutput->u.file.filesize);
                    break;
                case LOGOUTTYPE_SOCK:
                    printf("type: socket\n");
                    printf("format: %s\n", r->pformat->format);
                    printf("level: %s\n", LOGLEVELSTR[r->level_begin]);
                    printf("addr: %s:%d\n", r->poutput->u.sock.addr, r->poutput->u.sock.port);
                    break;
                case LOGOUTTYPE_LOGCAT:
                    printf("type: logcat\n");
                    printf("format: %s\n", r->pformat->format);
                    printf("level: %s\n", LOGLEVELSTR[r->level_begin]);
                    break;
                case LOGOUTTYPE_SYSLOG:
                    printf("type: syslog\n");
                    printf("format: %s\n", r->pformat->format);
                    printf("level: %s\n", LOGLEVELSTR[r->level_begin]);
                    break;
                case LOGOUTTYPE_NONE:
                    break;
                default:
                    printf("type: unknown\n");
                    break;
                }

                printf("\n");
            }
        }
    }
}
