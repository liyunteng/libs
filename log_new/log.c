/*
 * log.c - log
 *
 * Date   : 2021/01/13
 */
#include "log.h"

#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

/* pointer to environment */
extern char **environ;

typedef struct {
    const char *token;
    const int level;
} loglevel_t;

static const loglevel_t levels[] = {
    {"EMERG", LOG_EMERG},  {"ALERT", LOG_ALERT},
    {"CRIT", LOG_CRIT},    {"ERROR", LOG_ERR},
    {"WARN", LOG_WARNING}, {"NOTICE", LOG_NOTICE},
    {"INFO", LOG_INFO},    {"DEBUG", LOG_DEBUG},
};

/* dump the environment */
static void
dump_environment(void)
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
            printf("Can't parse environment variable %s", buf);
            continue;
        }

        *e = 0;
        ++e;
        printf("Environment: [%s] = [%s]", buf, e);
    }
}

static int
log_get_level(const char *ep)
{
    int lvl = -1;
    int idx = 0;

    if (!ep) {
        return LOG_LEVEL_MIN;
    }

    while (levels[idx].token) {
        if (!strcasecmp(ep, levels[idx].token)) {
            lvl = levels[idx].level;
            break;
        }
        ++idx;
    }

    if (lvl == -1) {
        lvl = atoi(ep);
    }

    if (lvl > LOG_LEVEL_MAX) {
        lvl = LOG_LEVEL_MAX;
    } else if (lvl < LOG_LEVEL_MIN) {
        lvl = LOG_LEVEL_MIN;
    }
    return lvl;
}

static int
log_file_name(char *filename, uint16_t len, log_ctx_p ctx)
{
    if (!ctx) {
        return -1;
    }

    snprintf(filename, len, "%s/%s.log",
             (ctx->file_path ? ctx->file_path : LOG_PATH_DEFAULT),
             (ctx->log_name ? ctx->log_name : LOG_APP_NAME_DEFAULT));

    return 0;
}

static void
log_time_str(char *time_str, uint32_t len)
{
    struct timeb cur_time;
    struct tm *tf;
    ftime(&cur_time);
    tf = localtime(&cur_time.time);

    snprintf(time_str, len, "%02d/%02d %02d:%02d:%02d.%03d",
             tf->tm_mon + 1, tf->tm_mday, tf->tm_hour, tf->tm_min,
             tf->tm_sec, cur_time.millitm);
}

inline static int
log_open_logfile(log_ctx_p ctx, int *fd)
{
    size_t len;
    char *file_name;

    if (!ctx || !fd) {
        return -1;
    }
    if (ctx->file_path)
        len = strlen(ctx->file_path);
    else
        len = strlen(LOG_PATH_DEFAULT);

    len += 1;

    if (ctx->log_name)
        len += strlen(ctx->log_name);
    else
        len += strlen(LOG_APP_NAME_DEFAULT);

    len += 4; /* ".log" */
    len += 1; /* NULL char */

    file_name = malloc(len * sizeof(char));
    if (!file_name) {
        return -1;
    }

    log_file_name(file_name, len, ctx);

    if ((*fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0644))
        < 0) {
        free(file_name);
        return -1;
    }

    ftruncate(*fd, ctx->file_size * 1024);

    free(file_name);
    return 0;
}

static int
log_rename_logfile(log_ctx_p ctx)
{
    size_t len, num, num_files;
    char *old_file_name, *new_file_name;

    if (!ctx) {
        return -1;
    }

    for (num = 0, num_files = ctx->num_files; num_files;
         num_files /= 10) {
        ++num;
    }

    if (ctx->file_path)
        len = strlen(ctx->file_path);
    else
        len = strlen(LOG_PATH_DEFAULT);

    len += 1; /* "/" */

    if (ctx->log_name)
        len += strlen(ctx->log_name);
    else
        len += strlen(LOG_APP_NAME_DEFAULT);

    len += 4;       /* ".log" */
    len += num + 1; /* ".<digit>" */
    len += 1;       /* NULL char */

    old_file_name = malloc(len * sizeof(char));
    new_file_name = malloc(len * sizeof(char));
    if (!old_file_name || !new_file_name) {
        free(old_file_name);
        free(new_file_name);
        return -1;
    }

    log_file_name(old_file_name, len, ctx);
    snprintf(new_file_name, len, "%s.%lu", old_file_name,
             ctx->file_idx);
    rename(old_file_name, new_file_name);
    printf("%s\n", new_file_name);

    ++ctx->file_idx;
    if (ctx->file_idx >= ctx->num_files)
        ctx->file_idx = 0;

    free(old_file_name);
    free(new_file_name);
    return 0;
}

static int
log_map_file(log_ctx_t *log_ctx, int window_size_factor)
{
    int page_size, fd;
    size_t mmap_window_size, len, file_size;
    int ret;

    if (!log_ctx || !log_ctx->file_size)
        return -1;

    file_size = log_ctx->file_size * 1024;

    if (log_ctx->mmap_window.offset == 0) {
        memset(&log_ctx->mmap_window, 0,
               sizeof(log_ctx->mmap_window));

        page_size = getpagesize();
        mmap_window_size =
            (4 * 1024 * 1024 * window_size_factor + page_size - 1)
            & (~(page_size - 1));
        if (mmap_window_size > 4 * 1024 * 1024) {
            return -1;
        }

        if (mmap_window_size > file_size) {
            log_ctx->file_size =
                ((mmap_window_size + 1024 - 1) & (~(1024 - 1)))
                / 1024;
            printf("1 file_size: %d\n", log_ctx->file_size);
        }
        ret = log_open_logfile(log_ctx, &fd);
        if (ret != 0) {
            return ret;
        }

        log_ctx->mmap_window.addr =
            mmap(0, mmap_window_size, PROT_READ | PROT_WRITE,
                 MAP_SHARED, fd, 0);
        if (log_ctx->mmap_window.addr == MAP_FAILED) {
            close(fd);
            return -1;
        }

        log_ctx->mmap_window.fd     = fd;
        log_ctx->mmap_window.len    = mmap_window_size;
        log_ctx->mmap_window.offset = mmap_window_size;
    } else if (log_ctx->mmap_window.offset >= file_size) {
        char *c, *tail, *str = NULL;
        size_t size;

        len = log_ctx->mmap_window.offset
              - (log_ctx->mmap_window.len
                 - log_ctx->mmap_window.data_offset);

        tail = log_ctx->mmap_window.addr
               + log_ctx->mmap_window.data_offset;
        for (c = tail - 1; c >= (char *)log_ctx->mmap_window.addr;
             --c) {
            if (*c == '\n') {
                break;
            }
        }

        size = tail - (++c);
        if (size) {
            str = malloc((size + 1) * sizeof(char));
            if (!str) {
                munmap(log_ctx->mmap_window.addr,
                       log_ctx->mmap_window.len);
                log_ctx->mmap_window.addr = NULL;
                if (log_ctx->mmap_window.data_offset
                    < log_ctx->mmap_window.len) {
                    ftruncate(log_ctx->mmap_window.fd, len);
                }
                close(log_ctx->mmap_window.fd);
                memset(&log_ctx->mmap_window, 0,
                       sizeof(log_ctx->mmap_window));
                return -1;
            }
            strncpy(str, c, size + 1);
            *(str + size) = '\0';
        }

        munmap(log_ctx->mmap_window.addr, log_ctx->mmap_window.len);
        log_ctx->mmap_window.addr = NULL;
        if (log_ctx->mmap_window.data_offset
            < log_ctx->mmap_window.len) {
            ftruncate(log_ctx->mmap_window.fd, len - size);
        }
        close(log_ctx->mmap_window.fd);

        log_rename_logfile(log_ctx);

        page_size = getpagesize();
        mmap_window_size =
            (4 * 1024 * 1024 * window_size_factor + page_size - 1)
            & (~(page_size - 1));
        if (mmap_window_size > 4 * 1024 * 1024) {
            free(str);
            return -1;
        }

        if (mmap_window_size > file_size) {
            log_ctx->file_size =
                ((mmap_window_size + 1024 - 1) & (~(1024 - 1)))
                / 1024;
            printf("2 file_size: %d\n", log_ctx->file_size);
        }
        ret = log_open_logfile(log_ctx, &fd);
        if (ret != 0) {
            free(str);
            return ret;
        }

        memset(&log_ctx->mmap_window, 0,
               sizeof(log_ctx->mmap_window));

        log_ctx->mmap_window.addr =
            mmap(0, mmap_window_size, PROT_READ | PROT_WRITE,
                 MAP_SHARED, fd, 0);
        if (log_ctx->mmap_window.addr == MAP_FAILED) {
            close(fd);
            free(str);
            return -1;
        }

        log_ctx->mmap_window.fd  = fd;
        log_ctx->mmap_window.len = mmap_window_size;
        log_ctx->mmap_window.offset += mmap_window_size;
        if (str) {
            if (size <= log_ctx->mmap_window.len) {
                strncpy(log_ctx->mmap_window.addr, str,
                        log_ctx->mmap_window.len);
                log_ctx->mmap_window.data_offset += size;
            } else {
                /* possible ?? */
                free(str);
            }
        }
    } else {
        if (log_ctx->mmap_window.addr) {
            munmap(log_ctx->mmap_window.addr,
                   log_ctx->mmap_window.len);
            log_ctx->mmap_window.addr = NULL;
        }

        page_size = getpagesize();
        mmap_window_size =
            (4 * 1024 * 1024 * window_size_factor + page_size - 1)
            & (~(page_size - 1));
        if (mmap_window_size > 4 * 1024 * 1024) {
            close(log_ctx->mmap_window.fd);
            return -1;
        }

        if (log_ctx->mmap_window.data_offset
            < log_ctx->mmap_window.len) {
            len = log_ctx->mmap_window.len
                  - log_ctx->mmap_window.data_offset;
            len = (len + page_size - 1) & (~(page_size - 1));
            log_ctx->mmap_window.offset -= len;
            if (log_ctx->mmap_window.offset < 0) {
                log_ctx->mmap_window.offset = 0;
            }
            log_ctx->mmap_window.data_offset -=
                log_ctx->mmap_window.len - len;
            if (log_ctx->mmap_window.data_offset < 0) {
                log_ctx->mmap_window.data_offset = 0;
            }
            len = file_size - log_ctx->mmap_window.offset;
            if (mmap_window_size > len) {
                mmap_window_size = len;
            }

            log_ctx->mmap_window.addr =
                mmap(0, mmap_window_size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, log_ctx->mmap_window.fd,
                     log_ctx->mmap_window.offset);
            if (log_ctx->mmap_window.addr == MAP_FAILED) {
                close(log_ctx->mmap_window.fd);
                return -1;
            }
            log_ctx->mmap_window.len = mmap_window_size;
            log_ctx->mmap_window.offset += mmap_window_size;
            log_ctx->mmap_window.msync_offset = 0;
            memset(&log_ctx->mmap_window.msync_time, 0,
                   sizeof(log_ctx->mmap_window.msync_time));
        } else {
            len = file_size - log_ctx->mmap_window.offset;
            if (mmap_window_size > len) {
                mmap_window_size = len;
            }
            log_ctx->mmap_window.addr =
                mmap(0, mmap_window_size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, log_ctx->mmap_window.fd,
                     log_ctx->mmap_window.offset);
            if (log_ctx->mmap_window.addr == MAP_FAILED) {
                close(log_ctx->mmap_window.fd);
                return -1;
            }

            log_ctx->mmap_window.len = mmap_window_size;
            log_ctx->mmap_window.offset += mmap_window_size;
            log_ctx->mmap_window.msync_offset = 0;
            memset(&log_ctx->mmap_window.msync_time, 0,
                   sizeof(log_ctx->mmap_window.msync_time));
        }
    }

    return 0;
}

inline static int
vlog_print(log_ctx_p ctx, const char *fmt, va_list ap)
{
    size_t len;
    int ret, factor;
    va_list ap2;

    va_copy(ap2, ap);

    len = ctx->mmap_window.len - ctx->mmap_window.data_offset;
    ret = vsnprintf(ctx->mmap_window.addr
                        + ctx->mmap_window.data_offset,
                    len, fmt, ap);
    if (ret < 0) {
        ret = errno;
        goto err_quit;
    }

    if (ret >= len) {
        factor = ret / (4 * 1024 * 1024);
        if (ret % (4 * 1024 * 1024))
            ++factor;
        ret = log_map_file(ctx, factor);
        if (ret != 0) {
            goto err_quit;
        }

        len = ctx->mmap_window.len - ctx->mmap_window.data_offset;
        ret = vsnprintf(ctx->mmap_window.addr
                            + ctx->mmap_window.data_offset,
                        len, fmt, ap2);
        if (ret < 0) {
            ret = errno;
            goto err_quit;
        }
    }
    ctx->mmap_window.data_offset += ret;

err_quit:
    va_end(ap2);
    return ret;
}

int
log_print(log_ctx_p ctx, const char *format, ...)
{
    int l;

    va_list ap;
    va_start(ap, format);
    l = vlog_print(ctx, format, ap);
    va_end(ap);

    return l;
}

void
log_destroy(log_ctx_p ctx)
{
    uint8_t ix;

    if (ctx) {
        printf("num_files: %d file_size: %d\n", ctx->num_files,
               ctx->file_size);
        if (ctx->mmap_window.addr) {
            log_print(ctx, "\n");
        }

        if (ctx->log_name) {
            free(ctx->log_name);
        }
        if (ctx->file_path) {
            free(ctx->file_path);
        }
        if (ctx->mmap_window.addr) {
            munmap(ctx->mmap_window.addr, ctx->mmap_window.len);
            ftruncate(ctx->mmap_window.fd,
                      ctx->mmap_window.offset
                          - (ctx->mmap_window.len
                             - ctx->mmap_window.data_offset));
            close(ctx->mmap_window.fd);
            ctx->mmap_window.addr = NULL;
        }
        free(ctx);
    }
}

int
log_init_ex(const char *log_name, const char *file_path,
            const uint16_t files, const uint32_t file_size,
            log_ctx_p *out_ctx)
{
    log_ctx_t *log_ctx;
    char filename[256];
    struct stat file_info;

    int malloc_size;
    size_t fs;
    char time_str[128];
    int ret;

    if (!out_ctx) {
        return -1;
    }

    *out_ctx = NULL;

    malloc_size = sizeof(log_ctx_t);
    log_ctx     = calloc(1, malloc_size);
    if (!log_ctx) {
        return errno;
    }

    if (log_name) {
        log_ctx->log_name = strdup(log_name);
    }
    if (file_path) {
        log_ctx->file_path = strdup(file_path);
    }
    log_ctx->file_size = (file_size ? file_size : 4096);
    log_ctx->num_files = (files ? files : 4);

    printf("num_files: %d file_size: %d\n", log_ctx->num_files,
           log_ctx->file_size);
    if ((ret = log_file_name(filename, sizeof(filename), log_ctx))
        != 0) {
        return -1;
    }

    if (stat(filename, &file_info) < 0) {
        if (errno != ENOENT) {
            ret = errno;
            goto init_error;
        }
    } else {
        if (unlink(filename) < 0) {
            ret = errno;
            goto init_error;
        }
    }

    ret = log_map_file(log_ctx, 1);
    if (ret != 0) {
        goto init_error;
    }

    log_time_str(time_str, sizeof(time_str));
    log_print(log_ctx, "ADDX logging file\n");
    log_print(log_ctx, "%s: log started for process ID %d\n",
              time_str, (int)getpid());

    *out_ctx = log_ctx;
    return 0;

init_error:
    if (log_ctx) {
        log_destroy(log_ctx);
    }
    return ret;
}

log_ctx_p
log_init(const char *log_name, const char *file_path,
         const uint8_t files, const uint16_t file_size)
{
    log_ctx_p c = NULL;
    int ret;

    ret = log_init_ex(log_name, file_path, files, file_size, &c);
    if (ret == 0) {
        return c;
    }
    return NULL;
}
