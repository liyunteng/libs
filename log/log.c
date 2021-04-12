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

#define BUFFER_MIN 1024 * 4
#define BUFFER_MAX 1024 * 1024 * 4

extern char *LOGLEVELSTR[];

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
    struct log_spec *s;
    struct log_event *event = NULL;
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

static void
log_event_update(struct log_handler *handler, struct log_rule *rule, int level,
                 const char *file, const char *func, long line, const char *tag,
                 const char *fmt, va_list ap)
{
    struct log_event *e = &handler->event;
    if (!e->ident) {
        e->ident            = handler->ident;
        e->ident_len        = strlen(handler->ident);
    }

    e->level = level;
    e->file  = file;
    e->func = func;
    e->line = line;
    e->tag  = tag;

    e->fmt = fmt;
    va_copy(e->ap, ap);

    e->pid = (pid_t)0;
    e->tid = 0;

    if (e->hostname_len == 0) {
        if (gethostname(e->hostname, sizeof(e->hostname) - 1) < 0) {
            ERROR_LOG("gethostname failed: (%s)\n", strerror(errno));
        } else {
            e->hostname_len = strlen(e->hostname);
        }
    }

    e->timestamp.tv_sec = 0;
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
    struct log_spec *s;

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
    list_del(&format->format_entry);
    struct log_spec *ps, *tmp;
    list_for_each_entry_safe (ps, tmp, &format->callbacks, spec_entry) {
        list_del(&ps->spec_entry);
        free(ps);
        ps = NULL;
    }
    free(format);
    format = NULL;
}

static struct log_output *
log_output_create_v(enum LOG_OUTTYPE type, va_list ap)
{
    struct log_output *output = NULL;

    output = calloc(1, sizeof(struct log_output));
    if (!output) {
        ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
        return NULL;
    }
    switch (type) {
    case LOG_OUTTYPE_STDOUT:
        output->priv = &stdout_output_priv;
        break;
    case LOG_OUTTYPE_STDERR:
        output->priv = &stderr_output_priv;
        break;
    case LOG_OUTTYPE_LOGCAT:
        output->priv = &logcat_output_priv;
        break;
    case LOG_OUTTYPE_SYSLOG:
        output->priv = &syslog_output_priv;
        break;
    case LOG_OUTTYPE_FILE:
        output->priv = &file_output_priv;
        break;
    case LOG_OUTTYPE_MMAP:
        output->priv = &mmap_output_priv;
        break;
    case LOG_OUTTYPE_TCP:
        output->priv = &tcp_output_priv;
        break;
    case LOG_OUTTYPE_UDP:
        output->priv = &udp_output_priv;
        break;
    case LOG_OUTTYPE_USER:
        output->priv = &user_output_priv;
        break;
    default:
        ERROR_LOG("invalid type: %d\n", type);
        break;
    }

    if (!output->priv) {
        goto failed;
    }

    if (output->priv->ctx_init) {
        if (output->priv->ctx_init(output, ap) != 0) {
            ERROR_LOG("%s ctx_init failed\n", output->priv->type_name);
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
    list_del(&output->output_entry);


    if (output->priv->ctx_uninit) {
        output->priv->ctx_uninit(output);
    }
    output->priv = NULL;

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
log_rule_set_level(struct log_rule *rule, int level_begin, int level_end)
{
    if (!rule) {
        ERROR_LOG("invalid argument\n");
        return -1;
    }

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

struct log_rule *
log_rule_create(struct log_handler *handler, struct log_format *format,
                struct log_output *output, int level_begin, int level_end)
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

void
log_rule_destroy(struct log_rule *rule)
{
    if (rule) {
        list_del(&rule->rule);
        list_del(&rule->rule_entry);
        free(rule);
        rule = NULL;
    }
}

void
log_vprintf(log_handler_t *handler, const int lvl, const char *file,
            const char *func, const long line, const char *tag, const char *fmt,
            va_list ap)
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

        log_event_update(handler, r, level, file, func, line, tag, fmt, ap);
        len = log_do_format(handler, r);
        if (len <= 0) {
            DEBUG_LOG("len: %d\n", len);
            continue;
        }

        ret = r->output->priv->emit(r->output, handler);
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
log_printf(struct log_handler *handler, int level, const char *file,
           const char *function, long line, const char *tag, const char *fmt,
           ...)
{
    va_list ap;

    va_start(ap, fmt);
    log_vprintf(handler, level, file, function, line, tag, fmt, ap);
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
    int i             = 0;
    int j             = 0;
    int rule_count    = 0;
    int format_count  = 0;
    int output_count  = 0;
    int handler_count = 0;

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

            if (rule->output->priv->dump) {
                rule->output->priv->dump(rule->output);
            }

            printf("\n");
        }
    }
}
