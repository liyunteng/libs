/*
 * log_foamat.c - log_format
 *
 * Date   : 2021/01/15
 */
#include "log_format.h"
#include "log_priv.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_TIME_FORMAT "%F %T"
#define DEFAULT_FORMAT      "%d.%ms %c:%p [%V] %F:%U(%L) %m%n"

#define COLOR_EMERG   "\033[7;01;91m"
#define COLOR_ALERT   "\033[7;01;93m"
#define COLOR_FATAL   "\033[7;01;95m"
#define COLOR_ERROR   "\033[0;01;91m"
#define COLOR_WARNING "\033[0;01;95m"
#define COLOR_NOTICE  "\033[0;00;93m"
#define COLOR_INFO    "\033[0;00;92m"
#define COLOR_DEBUG   "\033[0;00;97m"
#define COLOR_VERBOSE "\033[0;00;00m"
#define COLOR_RESET   "\033[0;00;00m"

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
    COLOR_NOTICE, COLOR_INFO,  COLOR_DEBUG, COLOR_VERBOSE, COLOR_RESET,
};

static int
spec_write_str(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)e;
    return buf_append(buf, s->str, s->len);
}

static int
spec_write_time(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    time_t now_sec = e->timestamp.tv_sec;
    struct tm tm;

    if (!now_sec) {
        gettimeofday(&(e->timestamp), NULL);
        now_sec = e->timestamp.tv_sec;
    }

    if (e->ts != now_sec) {
        localtime_r(&(now_sec), &tm);
        e->ts = now_sec;
        e->time_str_len =
            strftime(e->time_str, sizeof(e->time_str) - 1, s->time_fmt, &tm);
    }

    if (e->time_str_len > 0) {
        return buf_append(buf, e->time_str, e->time_str_len);
    } else {
        return buf_append(buf, "(time=null)", strlen("(time=null)"));
    }
}

static int
spec_write_ms(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    if (!e->timestamp.tv_sec) {
        gettimeofday(&(e->timestamp), NULL);
    }
    return buf_printf_dec32(buf, (e->timestamp.tv_usec / 1000), 3);
}

static int
spec_write_us(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    if (!e->timestamp.tv_sec) {
        gettimeofday(&e->timestamp, NULL);
    }
    return buf_printf_dec32(buf, e->timestamp.tv_usec, 6);
}

static int
spec_write_ident(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    return buf_append(buf, e->ident, e->ident_len);
}

static int
spec_write_file(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    if (!e->file) {
        return buf_append(buf, "(file=null)", strlen("(file=null)"));
    } else {
        return buf_append(buf, e->file, e->file_len);
    }
}

static int
spec_write_line(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    return buf_printf_dec64(buf, e->line, 0);
}

static int
spec_write_func(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    if (!e->func) {
        return buf_append(buf, "(func=null)", strlen("(func=null)"));
    } else {
        return buf_append(buf, e->func, e->func_len);
    }
}

static int
spec_write_hostname(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    return buf_append(buf, e->hostname, e->hostname_len);
}

static int
spec_write_newline(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    (void)e;
    return buf_append(buf, "\n", 1);
}

static int
spec_write_cr(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    (void)e;
    return buf_append(buf, "\r", 1);
}

static int
spec_write_percent(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    (void)e;
    return buf_append(buf, "%", 1);
}

static int
spec_write_pid(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    if (!e->pid) {
        e->pid = getpid();

        if (e->pid != e->last_pid) {
            e->last_pid    = e->pid;
            e->pid_str_len = sprintf(e->pid_str, "%u", e->pid);
        }
    }
    return buf_append(buf, e->pid_str, e->pid_str_len);
}

static int
spec_write_tid(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    e->tid_str_len = sprintf(e->tid_str, "%lu", (unsigned long)pthread_self());
    return buf_append(buf, e->tid_str, e->tid_str_len);
}

static int
spec_write_tid_hex(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    e->tid_str_len = sprintf(e->tid_str, "0x%x", (unsigned int)pthread_self());
    return buf_append(buf, e->tid_str, e->tid_str_len);
}

static int
spec_write_level(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    return buf_append(buf, LOGLEVELSTR[e->level],
                      strlen(LOGLEVELSTR[e->level]));
}

static int
spec_write_level_lower(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    return buf_append(buf, loglevelstr[e->level],
                      strlen(loglevelstr[e->level]));
}

static int
spec_write_message(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    if (e->fmt) {
        return buf_vprintf(buf, e->fmt, e->ap);
    } else {
        return buf_append(buf, "format=(null)", strlen("format=(null)"));
    }
}

static int
spec_write_color(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    return buf_append(buf, COLORSTR[e->level], strlen(COLORSTR[e->level]));
}

static int
spec_write_reset_color(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)s;
    (void)e;
    return buf_append(buf, COLOR_RESET, strlen(COLOR_RESET));
}

static int
spec_write_env(struct log_spec *s, struct log_event *e, log_buf_t *buf)
{
    (void)e;
    if (strlen(s->env_name) > 0) {
        char *env = getenv(s->env_name);
        if (env) {
            return buf_append(buf, env, strlen(env));
        } else {
            char tmp[64];
            snprintf(tmp, 64, "env(%s)=(null)", s->env_name);
            return buf_append(buf, tmp, strlen(tmp));
        }
    }
    return 0;
}

/* ********************************************************************** */

static int
spec_gen_msg_direct(struct log_spec *s, struct log_event *e)
{
    return s->write_buf(s, e, e->msg_buf);
}

static int
spec_gen_msg_reformat(struct log_spec *s, struct log_event *e)
{
    int ret;

    buf_restart(e->pre_msg_buf);

    ret = s->write_buf(s, e, e->pre_msg_buf);
    if (ret < 0) {
        ERROR_LOG("spec->gen_buf failed\n");
        return -1;
    } else if (ret > 0) {
        /* TODO: buf is full */
    }

    return buf_adjust_append(e->msg_buf, buf_str(e->pre_msg_buf),
                             buf_len(e->pre_msg_buf), s->left_adjust,
                             s->left_fill_zeros, s->min_width, s->max_width);
}
/* ********************************************************************** */

static int
spec_parse_print_fmt(struct log_spec *s)
{
    char *p, *q;
    long i, j;

    p = s->print_fmt;
    if (*p == '-') {
        s->left_adjust = 1;
        p++;
    } else {
        if (*p == '0') {
            s->left_fill_zeros = 1;
        }
        s->left_adjust = 0;
    }

    i = j = 0;
    sscanf(p, "%ld.", &i);
    q = strchr(p, '.');
    if (q)
        sscanf(q, ".%ld", &j);

    s->min_width = (size_t)i;
    s->max_width = (size_t)j;
    return 0;
}

struct log_spec *
spec_create(char *pstart, char **pnext)
{
    char *p;
    int nscan          = 0;
    int nread          = 0;
    struct log_spec *s = NULL;

    if (!pstart || !pnext) {
        return NULL;
    }

    s = calloc(1, sizeof(struct log_spec));
    if (!s) {
        ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
        return NULL;
    }
    s->str = p = pstart;

    switch (*p) {
    case '%': {
        /* a string begin with %: %12.35d(%F %X) */

        /* process width and precision char in %-12.35P */
        nread = 0;
        nscan = sscanf(p, "%%%[.0-9-]%n", s->print_fmt, &nread);
        if (nscan == 1) {
            s->gen_msg = spec_gen_msg_reformat;
            if (spec_parse_print_fmt(s)) {
                ERROR_LOG("spec_parse_print_fmt failed\n");
                goto failed;
            }
        } else {
            /* skip the % char */
            nread      = 1;
            s->gen_msg = spec_gen_msg_direct;
        }
        p += nread;

        if (*p == 'd') { /* datetime */
            if (*(p + 1) != '(') {
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
                if (*(p - 1) != ')') {
                    ERROR_LOG("in string[%s] can't find match \')\'\n", s->str);
                    goto failed;
                }
            }

            s->write_buf = spec_write_time;
            *pnext       = p;
            s->len       = p - s->str;
            break;
        }

        if (*p == 'E') {
            if (*(p + 1) != '(') {
                ERROR_LOG("in string[%s] can't find \'(\'\n", s->str);
                goto failed;
            } else if (strncmp(p, "E()", 3) == 0) {
                ERROR_LOG("in string[%s] can't find key\n", s->str);
                goto failed;
            } else {
                nread = 0;
                nscan = sscanf(p, "E(%[^)])%n", s->env_name, &nread);
                if (nscan != 1) {
                    nread = 0;
                }
                p += nread;
                if (*(p - 1) != ')') {
                    ERROR_LOG("in string[%s] can't find match \')\'\n", s->str);
                    goto failed;
                }
            }

            s->write_buf = spec_write_env;
            *pnext       = p;
            s->len       = p - s->str;
            break;
        }

        if (strncmp(p, "ms", 2) == 0) { /* ms */
            p += 2;
            *pnext       = p;
            s->len       = p - s->str;
            s->write_buf = spec_write_ms;
            break;
        } else if (strncmp(p, "us", 2) == 0) { /* us */
            p += 2;
            *pnext       = p;
            s->len       = p - s->str;
            s->write_buf = spec_write_us;
            break;
        }

        *pnext = p + 1;
        s->len = p - s->str + 1;

        switch (*p) {
        case 'c': /* ident */
            s->write_buf = spec_write_ident;
            break;
        case 'H': /* hostname */
            s->write_buf = spec_write_hostname;
            break;
        case 'F': /* file */
            s->write_buf = spec_write_file;
            break;
        case 'U': /* function */
            s->write_buf = spec_write_func;
            break;
        case 'L': /* line */
            s->write_buf = spec_write_line;
            break;
        case 'p': /* pid */
            s->write_buf = spec_write_pid;
            break;
        case 't': /* tid */
            s->write_buf = spec_write_tid;
            break;
        case 'T': /* tid hex */
            s->write_buf = spec_write_tid_hex;
            break;
        case 'V': /* LEVEL */
            s->write_buf = spec_write_level;
            break;
        case 'v': /* level */
            s->write_buf = spec_write_level_lower;
            break;
        case 'm': /* message */
            s->write_buf = spec_write_message;
            break;
        case 'r': /* '\r' */
            s->write_buf = spec_write_cr;
            break;
        case 'n': /* '\n' */
            s->write_buf = spec_write_newline;
            break;
        case '%': /* '%' */
            s->write_buf = spec_write_percent;
            break;
        case 'C': /* color */
            s->write_buf = spec_write_color;
            break;
        case 'R': /* color reset */
            s->write_buf = spec_write_reset_color;
            break;
        default:
            ERROR_LOG("str[%s] in wrong format, p[%c]\n", s->str, *p);
            goto failed;
        }
        break;
    }

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
        s->gen_msg   = spec_gen_msg_direct;
    }

    return s;
failed:
    free(s);
    return NULL;
}
