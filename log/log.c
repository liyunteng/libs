/*
 * log.c - log
 *
 * Date   : 2021/01/14
 */

#include "list.h"
#ifdef __cplusplus
#ifndef __STDC_FORMAT_MARCOS
#define __STDC_FORMAT_MARCOS
#endif
#endif

#include "file_output.h"
#include "mmap_output.h"
#include "other_outputs.h"
#include "sock_output.h"
#include "syslog_output.h"
#include "user_output.h"

#include "format.h"
#include "log_priv.h"

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

#define COLOR_EMERG "\033[7;49;31m"
#define COLOR_ALERT "\033[7;49;35m"
#define COLOR_FATAL "\033[7;49;33m"
#define COLOR_ERROR "\033[1;1;31m"
#define COLOR_WARNING "\033[1;1;35m"
#define COLOR_NOTICE "\033[1;1;33m"
#define COLOR_INFO "\033[1;1;32m"
#define COLOR_DEBUG "\033[1;1;37m"
#define COLOR_VERBOSE "\033[0;0;00m"
#define COLOR_NORMAL "\033[0;0;00m"

#define DEFAULT_FORMAT "%d.%ms %c:%p [%V] %F:%U(%L) %m%n"

#define BUFFER_MIN 1024 * 4
#define BUFFER_MAX 1024 * 1024 * 4

const char *const LOGLEVELSTR[] = {
    "PANIC",  "ALERT", "FATAL", "ERROR",   "WARNING",
    "NOTICE", "INFO",  "DEBUG", "VERBOSE",
};

const char *const loglevelstr[] = {
    "panic",  "alert", "fatal", "error",   "warning",
    "notice", "info",  "debug", "verbose",
};

const char *const COLORSTR[] = {
    COLOR_EMERG,  COLOR_ALERT, COLOR_FATAL, COLOR_ERROR,   COLOR_WARNING,
    COLOR_NOTICE, COLOR_INFO,  COLOR_DEBUG, COLOR_VERBOSE, COLOR_NORMAL,
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

static struct log_handler *default_log_handler = NULL;

static size_t
log_do_format(struct log_handler *handler, struct log_rule *r)
{
    log_spec_t *s;
    log_event_t *event = NULL;
    int ret;

    if (!handler || !r) {
        ERROR_LOG("invalid argument\n");
        return -1;
    }
    event = &handler->event;

    buf_restart(event->msg_buf);
    list_for_each_entry (s, &r->format->callbacks, spec_entry) {
        ret = s->gen_msg(s, event);
        if (ret < 0) {
            ERROR_LOG("spec %s failed\n", s->str);
            continue;
        } else if (ret == 1) {
            ERROR_LOG("spec %s truncated\n", s->str);
            break;
        }
    }
    buf_seal(event->msg_buf);
    return buf_len(event->msg_buf);
}

static int
log_ctl_v(enum LOG_OPTS opt, va_list ap)
{
    struct log_handler *handler = va_arg(ap, struct log_handler *);
    if (!handler) {
        ERROR_LOG("invalid indent\n");
        return -1;
    }

    switch (opt) {
    case LOG_OPT_SET_HANDLER_IDENT: {
        char *ident = va_arg(ap, char *);
        if (!ident) {
            ERROR_LOG("ident pointer is NULL\n");
            return -1;
        }
        strncpy(handler->ident, ident, sizeof(handler->ident));
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

static int
log_ctl(enum LOG_OPTS opt, ...)
{
    va_list ap;

    va_start(ap, opt);
    int ret = log_ctl_v(opt, ap);
    va_end(ap);

    return ret;
}

struct log_format *
log_format_create(const char *fmt)
{
    struct log_format *fp = NULL;
    char *p, *q;
    log_spec_t *s;

    if (!fmt) {
        ERROR_LOG("fmt is NUL\n");
        return NULL;
    }

    fp = (struct log_format *)calloc(1, sizeof(struct log_format));
    if (!fp) {
        ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
        return NULL;
    }

    strncpy(fp->format, fmt, sizeof(fp->format));

    INIT_LIST_HEAD(&fp->callbacks);
    for (p = fp->format; *p != '\0'; p = q) {

        /* TODO: memleak */
        s = spec_create(p, &q);
        if (s == NULL) {
            ERROR_LOG("spec create failed\n");
            return NULL;
        }
        list_add_tail(&s->spec_entry, &fp->callbacks);
    }
    list_add_tail(&fp->format_entry, &format_header);

    return fp;
}

void
log_format_destroy(struct log_format *format)
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

static struct log_output *
log_output_create_v(enum LOG_OUTTYPE type, va_list ap)
{
    struct log_output *output = NULL;

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
    case LOG_OUTTYPE_MMAP:
        output = mmap_output_create();
        break;
    case LOG_OUTTYPE_TCP:
        output = tcp_output_create();
        break;
    case LOG_OUTTYPE_UDP:
        output = udp_output_create();
        break;
    case LOG_OUTTYPE_USER:
        output = user_output_create();
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

struct log_output *
log_output_create(enum LOG_OUTTYPE type, ...)
{
    struct log_output *output = NULL;
    va_list ap;
    va_start(ap, type);
    output = log_output_create_v(type, ap);
    va_end(ap);
    return output;
}

void
log_output_destroy(struct log_output *output)
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


struct log_handler *
log_handler_create(const char *ident)
{
    struct log_handler *handler = log_handler_get(ident);
    if (handler) {
        return handler;
    }

    handler = (struct log_handler *)calloc(1, sizeof(struct log_handler));
    if (!handler) {
        ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
        return NULL;
    }
    pthread_mutex_init(&handler->mutex, NULL);
    strncpy(handler->ident, ident, sizeof(handler->ident));
    handler->event.msg_buf = buf_create(BUFFER_MIN, BUFFER_MAX);
    if (!handler->event.msg_buf) {
        ERROR_LOG("buf_create failed\n");
        goto failed;
    }
    handler->event.pre_msg_buf = buf_create(BUFFER_MIN, BUFFER_MAX);
    if (!handler->event.pre_msg_buf) {
        ERROR_LOG("buf_create failed\n");
        goto failed;
    }

    INIT_LIST_HEAD(&handler->rules);
    list_add_tail(&handler->handler_entry, &handler_header);

    return handler;

failed:
    if (handler) {
        log_handler_destroy(handler);
    }
    return NULL;
}

void
log_handler_destroy(struct log_handler *handler)
{
    if (!handler) {
        ERROR_LOG("handler is NULL\n");
        return;
    }
    list_del(&handler->handler_entry);

    pthread_mutex_destroy(&handler->mutex);
    if (handler->event.msg_buf) {
        buf_destroy(handler->event.msg_buf);
    }
    if (handler->event.pre_msg_buf) {
        buf_destroy(handler->event.pre_msg_buf);
    }

    log_rule_t *r, *tmp;
    list_for_each_entry_safe (r, tmp, &(handler->rules), rule) {
        list_del(&r->rule_entry);
        list_del(&r->rule);
        free(r);
        r = NULL;
    }
}

struct log_handler *
log_handler_get(const char *ident)
{
    struct log_handler *handler = NULL;
    list_for_each_entry (handler, &handler_header, handler_entry) {
        if (strcmp(handler->ident, ident) == 0) {
            return handler;
        }
    }
    return NULL;
}

int
log_handler_set_default(struct log_handler *handler)
{
    default_log_handler = handler;
    return 0;
}

struct log_handler *
log_handler_get_default(void)
{
    return default_log_handler;
}

int
log_set_level(struct log_handler *handler, struct log_rule *rule,
              int level_begin, int level_end)
{
    if (!handler || !rule) {
        ERROR_LOG("invalid argument\n");
        return -1;
    }
    struct log_rule *r = NULL;
    list_for_each_entry (r, &handler->rules, rule) {
        if (r == rule) {

            if (level_begin >= LOG_EMERG && level_begin <= LOG_VERBOSE) {
                rule->level_begin = level_begin;
            } else {
                rule->level_begin = LOG_VERBOSE;
            }

            if (level_end >= LOG_EMERG && level_end <= LOG_VERBOSE) {
                rule->level_end = level_end;
            } else {
                rule->level_end = LOG_EMERG;
            }

            return 0;
        }
    }
    return -1;
}

struct log_rule*
log_bind(struct log_handler *handler, int level_begin, int level_end,
         struct log_format *format, struct log_output *output)
{
    if (handler == NULL || format == NULL || output == NULL) {
        ERROR_LOG("invalid argument\n");
        return NULL;
    }

    struct log_rule *r = (struct log_rule *)calloc(1, sizeof(struct log_rule));
    if (!r) {
        ERROR_LOG("malloc failed: (%s)\n", strerror(errno));
        return NULL;
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

    return r;
}

int
log_unbind(struct log_handler *handler, struct log_rule *rule)
{
    if (handler == NULL || rule == NULL) {
        ERROR_LOG("invalid argument\n");
        return -1;
    }

    log_rule_t *r;
    list_for_each_entry (r, &(handler->rules), rule) {
        if (r == rule) {
            list_del(&r->rule);
            list_del(&r->rule_entry);
            free(r);
            r = NULL;
            return 0;
        }
    }

    ERROR_LOG("rule not found\n");
    return -1;
}

void
mlog_vprintf(log_handler_t *handler, const int lvl, const char *file,
             const char *func, const long line, const char *fmt, va_list ap)
{
    uint16_t i;
    int ret;
    int level, len;

    struct log_rule *r = NULL;
    if (handler == NULL) {
        ERROR_LOG("handler is NULL\n");
        return;
    }

    level = lvl;
    if (level > LOG_VERBOSE)
        level = LOG_VERBOSE;
    if (lvl < LOG_EMERG)
        level = LOG_EMERG;

    pthread_mutex_lock(&handler->mutex);

    list_for_each_entry (r, &handler->rules, rule) {
        if (r->level_begin < level || r->level_end > level) {
            continue;
        }

        event_update(&handler->event, handler, r, level, file, func, line, fmt,
                     ap);
        len = log_do_format(handler, r);
        if (len <= 0) {
            DEBUG_LOG("len: %d\n", len);
            continue;
        }

        ret = r->output->emit(r->output, handler);
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
mlog_printf(struct log_handler *handler, int level, const char *file,
            const char *function, long line, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    mlog_vprintf(handler, level, file, function, line, fmt, ap);
    va_end(ap);

    return;
}

void
log_cleanup(void)
{

    struct log_handler *handler, *htmp;
    list_for_each_entry_safe (handler, htmp, &handler_header, handler_entry) {
        log_handler_destroy(handler);
    }

    struct log_format *format, *ftmp;
    list_for_each_entry_safe (format, ftmp, &format_header, format_entry) {
        log_format_destroy(format);
    }

    struct log_output *output, *otmp;
    list_for_each_entry_safe (output, otmp, &output_header, output_entry) {
        log_output_destroy(output);
    }

    struct log_rule *rule, *rtmp;
    list_for_each_entry_safe (rule, rtmp, &rule_header, rule_entry) {
        list_del(&rule->rule_entry);
        free(rule);
    }
}

void
dump_statstic(struct log_output *output)
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
    struct log_rule *rule;
    list_for_each_entry (rule, &rule_header, rule_entry) {
        rule_count++;
    }

    struct log_format *format;
    list_for_each_entry (format, &format_header, format_entry) {
        format_count++;
    }

    struct log_output *output;
    list_for_each_entry (output, &output_header, output_entry) {
        output_count++;
    }

    struct log_handler *handler;
    list_for_each_entry (handler, &handler_header, handler_entry) {
        handler_count++;
    }

    printf("handler: %d output: %d format: %d rule: %d\n", handler_count,
           output_count, format_count, rule_count);

    list_for_each_entry (handler, &handler_header, handler_entry) {
        i++;
        j = 0;
        printf("------------------------\n");
        printf("handler: %d\n", i);
        printf("ident: %s\n", handler->ident);
        printf("buffer_min: %u\n", (unsigned)handler->event.msg_buf->size_min);
        printf("buffer_real: %u\n",
               (unsigned)handler->event.msg_buf->size_real);
        printf("buffer_max: %u\n", (unsigned)handler->event.msg_buf->size_max);
        printf("\n");
        list_for_each_entry (rule, &handler->rules, rule) {
            j++;

            printf("rule: %d\n", j);
            printf("format: %s\n", rule->format->format);
            printf("level: %s -- %s\n", LOGLEVELSTR[rule->level_begin],
                   LOGLEVELSTR[rule->level_end]);

            if (rule->output->dump) {
                rule->output->dump(rule->output);
            }

            printf("\n");
        }
    }
}
