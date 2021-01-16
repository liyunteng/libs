/*
 * foamat.c - format
 *
 * Date   : 2021/01/15
 */
#include "format.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define DEFAULT_TIME_FORMAT "%F %T"
extern char *LOGLEVELSTR[];

static int
spec_write_str(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    memcpy(buf, s->str, s->len);
    return s->len;
}

static int
spec_write_time(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    time_t now_sec = e->timestamp.tv_sec;
    struct tm *tm = &(e->tm);

    if (!now_sec) {
        gettimeofday(&(e->timestamp), NULL);
        now_sec = e->timestamp.tv_sec;
    }

    if (e->ts != now_sec) {
        localtime_r(&(now_sec), tm);
        e->ts = now_sec;
    }
    return strftime(buf, len, s->time_fmt, tm);
}

static int
spec_write_ms(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    if (!e->timestamp.tv_sec) {
        gettimeofday(&e->timestamp, NULL);
    }
    return snprintf(buf, len, "%03d", (int)(e->timestamp.tv_usec / 1000));
}

static int
spec_write_us(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    if (!e->timestamp.tv_sec) {
        gettimeofday(&e->timestamp, NULL);
    }

    return snprintf(buf, len, "%06d", (int)(e->timestamp.tv_usec));
}

static int
spec_write_ident(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    memcpy(buf, e->ident, e->ident_len);
    return e->ident_len;
}

static int
spec_write_file(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    memcpy(buf, e->file, e->file_len);
    return e->file_len;
}

static int
spec_write_line(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    return snprintf(buf, len, "%ld", e->line);
}

static int
spec_write_func(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    memcpy(buf, e->func, e->func_len);
    return e->func_len;
}

static int
spec_write_hostname(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    if (e->hostname_len) {
        memcpy(buf, e->hostname, e->hostname_len);
    }
    return e->hostname_len;
}

static int
spec_write_newline(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    memcpy(buf, "\n", 1);
    return 1;
}

static int
spec_write_percent(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    memcpy(buf, "%", 1);
    return 1;
}

static int
spec_write_pid(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    if (!e->pid) {
        e->pid = getpid();

        if (e->pid != e->last_pid) {
            e->last_pid = e->pid;
            e->pid_str_len = sprintf(e->pid_str, "%u", e->pid);
        }
    }
    memcpy(buf, e->pid_str, e->pid_str_len);
    return e->pid_str_len;
}

static int
spec_write_tid(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    return snprintf(buf, len, "%lu", (unsigned long)pthread_self());
}

static int
spec_write_level(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    return snprintf(buf, len, "%5.5s", LOGLEVELSTR[e->level]);
}

static int
spec_write_message(log_spec_t *s, log_event_t *e, char *buf, size_t len)
{
    return vsnprintf(buf, len, e->fmt, e->ap);
}


static int
spec_parse_print_fmt(log_spec_t *s)
{
    char *p, *q;
    long i, j;

    p = s->print_fmt;
    if (*p == '-') {
        s->left_adjust = TRUE;
        p++;
    } else {
        if (*p == '0') {
            s->left_fill_zeros = TRUE;
        }
        s->left_adjust = FALSE;
    }

    i = j = 0;
    sscanf(p, "%ld.", &i);
    q = strchr(p, '.');
    if (q) sscanf(q, ".%ld", &j);


    s->min_width = (size_t)i;
    s->max_width = (size_t)j;
    return 0;
}

log_spec_t *
spec_create(char *pstart, char **pnext)
{
    char *p;
    int nscan           = 0;
    int nread           = 0;
    log_spec_t *s       = NULL;

    if (!pstart || !pnext) {
        return NULL;
    }


    s = calloc(1, sizeof(log_spec_t));
    if (!s) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }
    s->str = p = pstart;

    switch (*p) {
    case '%':
        nread = 0;
        nscan = sscanf(p, "%%%[.0-9-]%n", s->print_fmt, &nread);
        if (nscan == 1) {
            /* TODO: parse like %-02d */
            if (spec_parse_print_fmt(s)) {
                ERROR_LOG("spec_parse_print_fmt failed\n");
                goto failed;
            }
        } else {
            /* skip the % char */
            nread = 1;
        }
        p += nread;

        if (*p == 'd') {
            if (*(p+1) != '(') {
                /* without '(', use default */
                strcpy(s->time_fmt, DEFAULT_TIME_FORMAT);
                p++;
            } else if (strncmp(p, "d()", 3) == 0) {
                /* with () but without detail time format. */
                strcpy(s->time_fmt, DEFAULT_TIME_FORMAT);
                p += 3;
            } else {
                nread = 0;
                nscan = sscanf(p, "d(%[^)])%n", s->time_fmt, &nread);
                if (nscan != 1) {
                    nread = 0;
                }

                p += nread;
                if (*(p-1) != ')') {
                    ERROR_LOG("in string[%s] can't find match \')\'\n", s->str);
                    goto failed;
                }
            }

            s->write_buf = spec_write_time;
            *pnext = p;
            s->len = p - s->str;
            break;
        }

        if (strncmp(p, "ms", 2) == 0) {
            p += 2;
            *pnext = p;
            s->len = p - s->str;
            s->write_buf = spec_write_ms;
            break;
        } else if (strncmp(p, "us", 2) == 0) {
            p += 2;
            *pnext = p;
            s->len = p - s->str;
            s->write_buf = spec_write_us;
            break;
        }

        *pnext = p+1;
        s->len = p - s->str + 1;

        switch(*p) {
        case 'c':
            s->write_buf = spec_write_ident;
            break;
        case 'D':
            strcpy(s->time_fmt, "%F");
            s->write_buf = spec_write_time;
            break;
        case 'T':
            strcpy(s->time_fmt, "%T");
            s->write_buf = spec_write_time;
            break;
        case 'F':
            s->write_buf = spec_write_file;
            break;
        case 'H':
            s->write_buf = spec_write_hostname;
            break;
        case 'L':
            s->write_buf = spec_write_line;
            break;
        case 'm':
            s->write_buf = spec_write_message;
            break;
        case 'n':
            s->write_buf = spec_write_newline;
            break;
        case 'p':
            s->write_buf = spec_write_pid;
            break;
        case 'U':
            s->write_buf = spec_write_func;
            break;
        case 'v':
        case 'V':
            s->write_buf = spec_write_level;
            break;
        case 't':
            s->write_buf = spec_write_tid;
            break;
        case '%':
            s->write_buf = spec_write_percent;
            break;
        default:
            ERROR_LOG("str[%s] in wrong format, p[%c]\n", s->str, *p);
            goto failed;
        }
        break;

    default:
        /* a cont string:/home/bb */
        *pnext = strchr(p, '%');
        if (*pnext) {
            s->len = *pnext - p;
        } else {
            s->len = strlen(p);
            *pnext = p + s->len;
        }
        s->write_buf = spec_write_str;
    }

    return s;
failed:
    free(s);
    return NULL;
}

void
event_update(log_event_t *e, log_handler_t *handler, log_rule_t *rule,
             LOG_LEVEL_E level, const char *file, const char *func, long line,
             const char *fmt, va_list ap)
{
    e->ident = handler->ident;
    e->ident_len = strlen(handler->ident);

    e->level   = level;

    e->file    = file;
    e->file_len = strlen(file);
    e->func    = func;
    e->func_len = strlen(func);
    e->line    = line;

    e->fmt     = fmt;
    va_copy(e->ap, ap);

    e->pid = (pid_t)0;
    e->tid = 0;

    if (e->hostname_len == 0) {
        if (gethostname(e->hostname, sizeof(e->hostname) - 1) < 0) {
            ERROR_LOG("gethostname failed(%s)\n", strerror(errno));
        } else {
            e->hostname_len = strlen(e->hostname);
        }
    }

    e->hostname_len = strlen(e->hostname);
    e->timestamp.tv_sec = 0;
}
