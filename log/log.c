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

#include "file_output.h"
#include "format.h"
#include "log_priv.h"
#include "other_outputs.h"
#include "sock_output.h"

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

#define COLOR_NORMAL "\033[0;0;00m"
#define COLOR_EMERG "\033[5;7;31m"
#define COLOR_ALERT "\033[5;7;35m"
#define COLOR_FATAL "\033[5;7;33m"
#define COLOR_ERROR "\033[1;0;31m"
#define COLOR_WARNING "\033[1;0;35m"
#define COLOR_NOTICE "\033[1;0;34m"
#define COLOR_INFO "\033[1;0;37m"
#define COLOR_DEBUG "\033[0;0;32m"
#define COLOR_VERBOSE "\033[0;0;00m"

#define DEFAULT_FORMAT "%d.%ms %c:%p [%V] %F:%U(%L) %m%n"

#define BUFFER_MIN 1024 * 4
#define BUFFER_MAX 1024 * 1024 * 4

/* pointer to environment */
extern char **environ;

const char *const LOGLEVELSTR[] = {
    "EMERG",  "ALERT", "FATAL", "ERROR",   "WARN",
    "NOTICE", "INFO",  "DEBUG", "VERBOSE",
};

static const char *const COLORSTR[] = {
    COLOR_EMERG,  COLOR_ALERT, COLOR_FATAL, COLOR_ERROR,   COLOR_WARNING,
    COLOR_NOTICE, COLOR_INFO,  COLOR_DEBUG, COLOR_VERBOSE,
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
dump_environment(log_handler_t *handler)
{
    static char buf[BUFSIZ];
    int cnt = 0;

    MLOGI(handler,
          "############################## LOG SYSTEM "
          "##############################\n");
    while (1) {
        char *e = environ[cnt++];

        if (!e || !*e) {
            break;
        }

        snprintf(buf, sizeof(buf), "%s", e);
        e = strchr(buf, '=');
        if (!e) {
            MLOGE(handler, "Can't parse environment variable %s", buf);
            continue;
        }

        *e = 0;
        ++e;
        MLOGI(handler, "Environment: [%s] = [%s]", buf, e);
    }
}


static size_t
log_format(log_handler_t *handler, log_rule_t *r, const LOG_LEVEL_E level,
           const char *file, const char *func, const long line, const char *fmt,
           va_list ap)
{
    size_t idx;
    size_t len;
    char *buf;
    log_argument_t arg;
    log_formater_t *f;
    int n = 0;

    if (r == NULL || r->format == NULL || handler == NULL
        || handler->bufferp == NULL) {
        ERROR_LOG("invalid argument\n");
        return 0;
    }

    arg.file = file;
    arg.func = func;
    arg.fmt = fmt;
    va_copy(arg.ap, ap);
    arg.level = level;
    arg.handler=handler;
    arg.line = line;

begin:
    buf = handler->bufferp;
    len = handler->buffer_real;
    memset(buf, 0, handler->buffer_real);

    idx = 0;

    if (r->format->color) {
        idx += snprintf(buf + idx, len - idx, "%s", COLORSTR[level]);
    }

    list_for_each_entry_reverse(f, &r->format->callbacks, formater_entry) {
        if (f == NULL) {
            break;
        }

        n = f->formater(f, &arg, buf+idx, len-idx);
        if (n <= 0) {
            goto err;
        }
        idx += n;

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
    }

end:
    if (r->format->color) {
        idx += snprintf(buf + idx, len - idx, "%s", COLOR_NORMAL);
    }
    return idx;

err:
    return 0;
}

#if 0
static size_t
log_format(log_handler_t *handler, log_rule_t *r, const LOG_LEVEL_E level,
           const char *file, const char *func, const long line, const char *fmt,
           va_list ap)
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
        idx += snprintf(buf + idx, len - idx, "%s", COLORSTR[level]);
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
                idx +=
                    snprintf(buf + idx, len - idx, "%06d", (int)(tv.tv_usec));
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
#endif

static int
log_ctl_v(enum LOG_OPTS opt, va_list ap)
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
    int ret = log_ctl_v(opt, ap);
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
log_format_create(const char *fmt, int color)
{
    log_format_t *fp = NULL;
    if (fmt) {
        fp = (log_format_t *)calloc(1, sizeof(log_format_t));
        if (fp) {
            strncpy(fp->format, fmt, strlen(fmt) + 1);
            if (color) {
                fp->color = TRUE;
            } else {
                fp->color = FALSE;
            }
            INIT_LIST_HEAD(&fp->callbacks);
            if (format_parse(fp) < 0) {
                free(fp);
                return NULL;
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

static log_output_t *
log_output_create_v(enum LOG_OUTTYPE type, va_list ap)
{
    log_output_t *output = NULL;

    switch (type) {
    case LOG_OUTTYPE_STDOUT:
        output = stdout_output_create();
        break;
    case LOG_OUTTYPE_STDERR:
        output = stderr_output_create();
        break;
    case LOG_OUTTYPE_LOGCAT:
        output = logcat_output_create();
        break;
    case LOG_OUTTYPE_SYSLOG:
        output = syslog_output_create();
        break;
    case LOG_OUTTYPE_FILE:
        output = file_output_create();
        break;
    case LOG_OUTTYPE_TCP:
        output = tcp_output_create();
        break;
    case LOG_OUTTYPE_UDP:
        output = udp_output_create();
        break;
    default:
        ERROR_LOG("invalid type: %d\n", type);
        break;
    }

    if (!output) {
        goto failed;
    }

    if (output->ctx_init) {
        if (output->ctx_init(output, ap) != 0) {
            ERROR_LOG("%s ctx_init failed\n", output->type_name);
            goto failed;
        }
    }

    list_add_tail(&output->output_entry, &output_header);
    return output;

failed:
    free(output);
    return NULL;
}

log_output_t *
log_output_create(enum LOG_OUTTYPE type, ...)
{
    log_output_t *output = NULL;
    va_list ap;
    va_start(ap, type);
    output = log_output_create_v(type, ap);
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

    if (output->ctx_uninit) {
        output->ctx_uninit(output);
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

    /* dump_environment(handler); */
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

static void
mlogv(log_handler_t *handler, const LOG_LEVEL_E lvl, const char *file,
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
            log_format(handler, r, level, file, function, line, fmt, ap2);
        va_end(ap2);
        if (len <= 0) {
            continue;
        }

        ret = r->output->emit(r->output, level, handler->bufferp, len);
        if (ret >= 0) {
            r->output->stat.stats[level].count++;
            r->output->stat.stats[level].bytes += len;
            r->output->stat.count_total++;
            r->output->stat.bytes_total += len;
        }
    }
    pthread_mutex_unlock(&handler->mutex);
}

void
mlog(log_handler_t *handle, LOG_LEVEL_E level, const char *file,
     const char *function, long line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    mlogv(handle, level, file, function, line, format, args);
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
        mlogv(handler, level, file, function, line, fmt, ap);
        va_end(ap);
    } else {
        ERROR_LOG("can't find handler: %s\n", DEFAULT_IDENT);
    }
    return;
}

void
dump_statstic(log_output_t *output)
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

void
log_dump(void)
{
    int i          = 0;
    int j          = 0;
    int rule_count = 0, format_count = 0, output_count = 0, handler_count = 0;
    printf("=====================log profile==============================\n");
    log_rule_t *ru;
    list_for_each_entry(ru, &rule_header, rule_entry) { rule_count++; }
    log_format_t *format;
    list_for_each_entry(format, &format_header, format_entry)
    {
        format_count++;
    }
    log_output_t *output;
    list_for_each_entry(output, &output_header, output_entry)
    {
        output_count++;
    }
    log_handler_t *handler;
    list_for_each_entry(handler, &handler_header, handler_entry)
    {
        handler_count++;
    }
    printf("handler: %d output: %d format: %d ru: %d\n", handler_count,
           output_count, format_count, rule_count);
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
