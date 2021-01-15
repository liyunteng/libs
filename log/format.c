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

inline static int
formater_write_u32(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "%u", f->u.u32);
}

inline static int
formater_write_u64(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "%lu", f->u.u64);
}

inline static int
formater_write_char(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "%c", f->u.c);
}


inline static int
formater_write_str(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "%s", f->u.str);
}

inline static int
formater_write_hex(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "0x%x", f->u.hex);
}

inline static int
formater_write_env(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "%s", getenv(f->key));
}

inline static int
formater_write_datetime(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    time_t t;
    struct tm now;
    t = time(NULL);
    localtime_r(&t, &now);
    return strftime(buf, len, f->key, &now);
}

inline static int
formater_write_ms(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return snprintf(buf, len, "%03d", (int)(tv.tv_usec / 1000));
}

inline static int
formater_write_us(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return snprintf(buf, len, "%06d", (int)(tv.tv_usec));
}


inline static int
formater_write_hostname(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "%s", f->key);
}

inline static int
formater_write_file(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "%s", s->file);
}

inline static int
formater_write_func(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "%s", s->func);
}

inline static int
formater_write_line(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "%ld", s->line);
}

inline static int
formater_write_level(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "%5.5s", LOGLEVELSTR[s->level]);
}

inline static int
formater_write_ident(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "%s", s->handler->ident);
}

inline static int
formater_write_tid(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return snprintf(buf, len, "%lu", (unsigned long)pthread_self());
}

inline static int
formater_write_message(log_formater_t *f, log_argument_t *s, char *buf, size_t len)
{
    return vsnprintf(buf, len, s->fmt, s->ap);
}

static log_formater_t *
formater_create()
{
    log_formater_t *f = NULL;
    f                 = (log_formater_t *)calloc(1, sizeof(log_formater_t));
    if (!f) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }
    return f;
}

int
format_parse(log_format_t *fmt)
{
    char *p             = fmt->format;
    int nscan           = 0;
    int nread           = 0;
    char buf[128]       = {0};
    log_formater_t *f   = NULL;
    log_formater_t *tmp = NULL;

    while (*p) {
        if (*p == '%') {
            nread = 0;
            nscan = sscanf(p, "%%%[.0-9-]%n", buf, &nread);
            if (nscan == 1) {
                ERROR_LOG("parse format [%s] failed.\n", p);
                goto failed;
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
                    ERROR_LOG("parse format [%s] failed\n", p);
                    goto failed;
                }
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "env";
                f->formater = formater_write_env;
                f->key      = strdup(env);
                list_add(&f->formater_entry, &fmt->callbacks);
            }

            if (*p == 'd') {
                if (*(p + 1) != '(') {
                    strcpy(buf, DEFAULT_TIME_FORMAT);
                    p++;
                } else if (strncmp(p, "d()", 3) == 0) {
                    strcpy(buf, DEFAULT_TIME_FORMAT);
                    p += 3;
                } else {
                    nread = 0;
                    nscan = sscanf(p, "d(%[^)])%n", buf, &nread);
                    if (nscan != 1) {
                        nread = 0;
                    }
                    p += nread;
                    if (*(p - 1) != ')') {
                        ERROR_LOG("parse format [%s] failed\n", p);
                        goto failed;
                    }
                }

                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "datetime";
                f->formater = formater_write_datetime;
                f->key      = strdup(buf);
                list_add(&f->formater_entry, &fmt->callbacks);
            }

            if (strncmp(p, "ms", 2) == 0) {
                p += 2;

                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "ms";
                f->formater = formater_write_ms;
                list_add(&f->formater_entry, &fmt->callbacks);
            }

            if (strncmp(p, "us", 2) == 0) {
                p += 2;

                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "us";
                f->formater = formater_write_us;
                list_add(&f->formater_entry, &fmt->callbacks);
            }

            switch (*p) {
            case 'D': /* 2020-01-01 */
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "time";
                f->formater = formater_write_datetime;
                f->key      = strdup("%F");
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            case 'T': /* 12:00:00 */
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "time";
                f->formater = formater_write_datetime;
                f->key      = strdup("%T");
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            case 'F': /* __FILE__ */
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "file";
                f->formater = formater_write_file;
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            case 'U': /* __FUNC__ */
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "func";
                f->formater = formater_write_func;
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            case 'L': /* __LINE__ */
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "line";
                f->formater = formater_write_line;
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            case 'n': /* \n */
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "str";
                f->formater = formater_write_str;
                f->u.str    = "\n";
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            case 'p': /* pid */
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "pid";
                f->formater = formater_write_u32;
                f->u.u32    = getpid();
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            case 'm': /* message */
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "message";
                f->formater = formater_write_message;
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            case 'c': /* handler->ident */
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "ident";
                f->formater = formater_write_ident;
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            case 'V': /* level */
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "level";
                f->formater = formater_write_level;
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            case 'H': /* hostname */
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                gethostname(buf, 128);
                f->mode     = "hostname";
                f->formater = formater_write_hostname;
                f->key      = strdup(buf);
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            case 't': /* tid */
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "tid";
                f->formater = formater_write_tid;
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            case '%':
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "str";
                f->formater = formater_write_str;
                f->u.str    = "%";
                list_add(&f->formater_entry, &fmt->callbacks);
                break;

            default:
                f = formater_create();
                if (!f) {
                    goto failed;
                }
                f->mode     = "char";
                f->formater = formater_write_char;
                f->u.c      = *p;
                list_add(&f->formater_entry, &fmt->callbacks);
                break;
            }
        } else {
            f = formater_create();
            if (!f) {
                goto failed;
            }
            f->mode     = "char";
            f->formater = formater_write_char;
            f->u.c      = *p;
            list_add(&f->formater_entry, &fmt->callbacks);
        }
        p++;
    }
    return 0;
failed:
    list_for_each_entry_safe(f, tmp, &fmt->callbacks, formater_entry)
    {
        if (f) {
            list_del(&f->formater_entry);
            if (f->key) {
                free(f->key);
                f->key = NULL;
            }
            free(f);
            f = NULL;
        }
    }
    return -1;
}
