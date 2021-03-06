/*
 * file_output.c - file_output
 *
 * Date   : 2021/01/15
 */
#include "file_output.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#define MAX_FILE_PATH   256
#define FLUSH_INTERVAL  200          // ms
#define ROTATE_INTERVAL (60 * 1000)  // ms

struct file_output_ctx {
    char *file_path;
    char *log_name;

    int rotate_police;
    int num_files;
    int file_idx;
    uint64_t file_size;
    uint64_t file_timestamp;

    uint64_t data_offset;
    uint64_t flush_timestamp;
    FILE *fp;
    char current_file_name[MAX_FILE_PATH];
};

/* pointer to environment */
extern char **environ;

/* dump the environment */
static void
dump_environment(struct log_output *output)
{
    int cnt = 0;
    static char buf[BUFSIZ];

    struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;
    fprintf(ctx->fp, "########## LOG STARTED ##########\n\n");

    while (1) {
        char *e = environ[cnt++];

        if (!e || !*e) {
            break;
        }

        snprintf(buf, sizeof(buf), "%s", e);
        e = strchr(buf, '=');
        if (!e) {
            ERROR_LOG("Can't parse environment variable %s", buf);
            continue;
        }

        *e = 0;
        ++e;

        /* INFO("Environment: [%s] = [%s]", buf, e); */
        fprintf(ctx->fp, "Environment: [%s] = [%s]\n", buf, e);
    }
    fprintf(ctx->fp, "\n");
}

static size_t
check_can_write_bytes(struct log_output *output, struct log_handler *handler)
{
    uint64_t ts = 0;
    size_t left = buf_len(handler->event.msg_buf);

    struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;
    if (ctx->rotate_police == ROTATE_POLICE_BY_SIZE && ctx->file_size > 0) {
        left = ((ctx->file_size > ctx->data_offset) ?
                    ctx->file_size - ctx->data_offset :
                    0);
    } else if (ctx->rotate_police == ROTATE_POLICE_BY_TIME) {
        ts = (handler->event.timestamp.tv_sec == 0 ?
                  log_get_ms() :
                  ((uint64_t)handler->event.timestamp.tv_sec * 1000
                   + handler->event.timestamp.tv_usec / 1000));

        if (ts > ctx->file_timestamp
            && ts - ctx->file_timestamp / ROTATE_INTERVAL * ROTATE_INTERVAL
                   > ROTATE_INTERVAL) {
            time_t t = (time_t)(ctx->file_timestamp / ROTATE_INTERVAL
                                * ROTATE_INTERVAL / 1000);
            DEBUG_LOG(
                "ts: %" PRIu64 " ctx->file_timestamp: %" PRIu64
                " delta: %" PRIu64 " last: %" PRIu64 " %s",
                ts, ctx->file_timestamp,
                (ts - ctx->file_timestamp / ROTATE_INTERVAL * ROTATE_INTERVAL),
                (ctx->file_timestamp / ROTATE_INTERVAL * ROTATE_INTERVAL),
                asctime(localtime(&t)));
            left = 0;
        }
    }
    return left;
}

#if 0
static int
do_file_rotate_by_time(struct log_output *output)
{
    struct tm tm;
    struct stat st;
    int i                             = 0;
    char time[128]                    = {0};
    char old_file_name[MAX_FILE_PATH] = {0};
    char new_file_name[MAX_FILE_PATH] = {0};

    struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;
    snprintf(old_file_name, MAX_FILE_PATH - 1, "%s/%s.log", ctx->file_path,
             ctx->log_name);

    if (lstat(old_file_name, &st) < 0) {
        ERROR_LOG("lstat failed: (%s)\n", strerror(errno));
        return -1;
    }

    localtime_r(&(st.st_atime), &tm);
    strftime(time, 127, "%Y%m%d_%H%M%S", &tm);
    snprintf(new_file_name, MAX_FILE_PATH - 1, "%s/%s.log.%s", ctx->file_path,
             ctx->log_name, time);
    rename(old_file_name, new_file_name);
    DEBUG_LOG("rename %s ==> %s\n", old_file_name, new_file_name);

    return 0;
}
#endif

static int
file_rotate_by_time(struct log_output *output)
{
    time_t t;
    struct tm tm;
    struct stat st;
    char file_path[MAX_FILE_PATH] = {0};

    struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;
    // create date directory
    t = (time_t)(log_get_ms() / 1000);
    localtime_r(&t, &tm);
    snprintf(file_path, MAX_FILE_PATH - 1, "%s/%04d%02d%02d", ctx->file_path,
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    if (lstat(file_path, &st) < 0) {
        if (errno == ENOENT) {
            if (mkdir(file_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
                ERROR_LOG("mkdir %s failed: (%s)\n", file_path,
                          strerror(errno));
                return -1;
            }
            DEBUG_LOG("mkdir %s\n", file_path);
        } else {
            if ((st.st_mode & S_IFMT) != S_IFDIR) {
                ERROR_LOG("%s is not directory\n", file_path);
                return -1;
            }
        }
    }

    // create log file
    if (ROTATE_INTERVAL >= 60 * 60 * 1000) {
        snprintf(ctx->current_file_name, MAX_FILE_PATH - 1, "%s/%s.%02d.log", file_path,
                 ctx->log_name, tm.tm_hour);
    } else if (ROTATE_INTERVAL >= 60 * 1000) {
        snprintf(ctx->current_file_name, MAX_FILE_PATH - 1, "%s/%s.%02d%02d.log", file_path,
                 ctx->log_name, tm.tm_hour, tm.tm_min);
    } else {
        snprintf(ctx->current_file_name, MAX_FILE_PATH - 1, "%s/%s.%02d%02d%02d.log",
                 file_path, ctx->log_name, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }

    if (lstat(ctx->current_file_name, &st) < 0) {
        if (errno == ENOENT) {
            // create new file
            if ((ctx->fp = fopen(ctx->current_file_name, "a+")) < 0) {
                ERROR_LOG("open %s failed: (%s)\n", ctx->current_file_name, strerror(errno));
                return -1;
            }
            DEBUG_LOG("open file(new) %s\n", ctx->current_file_name);
            ctx->data_offset    = 0;
            ctx->file_timestamp = log_get_ms();

            return 0;
        }

        ERROR_LOG("lstat %s failed: (%s)\n", ctx->current_file_name, strerror(errno));
        return -1;
    }

#if 0
    // rotate
    if (do_file_rotate_by_time(output) < 0) {
        ERROR_LOG("rename %s failed\n", ctx->current_file_name);
        return -1;
    }
#endif

    if ((ctx->fp = fopen(ctx->current_file_name, "a+")) == NULL) {
        ERROR_LOG("fopen %s failed: (%s)\n", ctx->current_file_name, strerror(errno));
        return -1;
    }
    DEBUG_LOG("open file(append) %s\n", ctx->current_file_name);
    ctx->data_offset    = (uint64_t)st.st_size;
    ctx->file_timestamp = log_get_ms();

    ++ctx->file_idx;

    return 0;
}

static int
do_file_rotate_by_size(struct log_output *output)
{
    struct stat st;
    int i                             = 0;
    uint32_t bak_file_num             = 0;
    char old_file_name[MAX_FILE_PATH] = {0};
    char new_file_name[MAX_FILE_PATH] = {0};

    struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;
    // don't rotate
    if (ctx->num_files <= 0) {
        return 0;
    }

    // rotate
    for (i = 0;; i++) {
        snprintf(old_file_name, MAX_FILE_PATH - 1, "%s/%s.log.%u",
                 ctx->file_path, ctx->log_name, i);
        if (lstat(old_file_name, &st) < 0) {
            if (errno == ENOENT) {
                break;
            }
        }
        bak_file_num++;
        if (ctx->num_files > 0 && bak_file_num >= ctx->num_files) {
            break;
        }
    }

    for (i = bak_file_num; i >= 0; i--) {
        if (i == ctx->num_files) {
            continue;
        }

        if (i == 0) {
            snprintf(old_file_name, MAX_FILE_PATH - 1, "%s/%s.log",
                     ctx->file_path, ctx->log_name);
        } else {
            snprintf(old_file_name, MAX_FILE_PATH - 1, "%s/%s.log.%u",
                     ctx->file_path, ctx->log_name, (i - 1));
        }

        snprintf(new_file_name, MAX_FILE_PATH - 1, "%s/%s.log.%u",
                 ctx->file_path, ctx->log_name, i);
        rename(old_file_name, new_file_name);
        DEBUG_LOG("rename %s ==> %s\n", old_file_name, new_file_name);
    }

    ++ctx->file_idx;

    return 0;
}

static int
file_rotate_by_size(struct log_output *output)
{
    struct stat st;
    /* char file_name[MAX_FILE_PATH] = {0}; */
    struct file_output_ctx *ctx   = (struct file_output_ctx *)output->ctx;

    snprintf(ctx->current_file_name, MAX_FILE_PATH - 1, "%s/%s.log", ctx->file_path,
             ctx->log_name);
    if (lstat(ctx->current_file_name, &st) < 0) {
        if (errno == ENOENT) {
            // create new file
            if ((ctx->fp = fopen(ctx->current_file_name, "a+")) == NULL) {
                ERROR_LOG("fopen %s failed: (%s)\n", ctx->current_file_name,
                          strerror(errno));
                return -1;
            }
            DEBUG_LOG("open file(new) %s\n", file_name);
            ctx->data_offset    = 0;
            ctx->file_timestamp = log_get_ms();
            return 0;
        }

        ERROR_LOG("lstat %s failed: (%s)\n", ctx->current_file_name, strerror(errno));
        return -1;
    }

    if (ctx->file_size > 0) {
        // rotate
        if (ctx->num_files > 0 && ctx->file_size <= st.st_size) {
            if (do_file_rotate_by_size(output) < 0) {
                ERROR_LOG("rename %s failed\n", ctx->current_file_name);
                return -1;
            }

            if ((ctx->fp = fopen(ctx->current_file_name, "w+")) == NULL) {
                ERROR_LOG("fopen %s failed: (%s)\n", ctx->current_file_name,
                          strerror(errno));
                return -1;
            }
            DEBUG_LOG("open file(new) %s\n", ctx->current_file_name);
            ctx->data_offset    = 0;
            ctx->file_timestamp = log_get_ms();
            return 0;
        }

        // truncated
        if (ctx->num_files <= 0 && ctx->file_size <= st.st_size) {
            if ((ctx->fp = fopen(ctx->current_file_name, "w")) == NULL) {
                ERROR_LOG("fopen %s failed: (%s)\n", ctx->current_file_name,
                          strerror(errno));
                return -1;
            }
            DEBUG_LOG("open file(truncated) %s\n", ctx->current_file_name);
            ctx->data_offset    = 0;
            ctx->file_timestamp = log_get_ms();
            return 0;
        }
    }

    // append
    if ((ctx->fp = fopen(ctx->current_file_name, "a+")) == NULL) {
        ERROR_LOG("fopen %s(append) failed: (%s)\n", ctx->current_file_name,
                  strerror(errno));
        return -1;
    }
    DEBUG_LOG("open file(append) %s\n", ctx->current_file_name);
    ctx->data_offset    = (uint64_t)st.st_size;
    ctx->file_timestamp = log_get_ms();
    return 0;
}


static int
file_open_logfile(struct log_output *output)
{
    struct stat st;
    struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;

    if (ctx->fp != NULL) {
        fclose(ctx->fp);
        ctx->fp = NULL;
    }

    if (lstat(ctx->file_path, &st) < 0) {
        if (errno == ENOENT) {
            if (mkdir(ctx->file_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
                < 0) {
                ERROR_LOG("mkdir %s failed: (%s)\n", ctx->file_path,
                          strerror(errno));
                goto failed;
            }
            DEBUG_LOG("mkdir %s\n", ctx->file_path);
        } else {
            ERROR_LOG("lstat %s failed: (%s)\n", ctx->file_path,
                      strerror(errno));
            goto failed;
        }
    } else {
        if ((st.st_mode & S_IFMT) != S_IFDIR) {
            ERROR_LOG("%s is not directory\n", ctx->file_path);
            goto failed;
        }
    }


    if (ctx->rotate_police == ROTATE_POLICE_BY_SIZE) {
        if (file_rotate_by_size(output) < 0) {
            ERROR_LOG("file rotate by size failed\n");
            goto failed;
        }
    } else if (ctx->rotate_police == ROTATE_POLICE_BY_TIME) {
        if (file_rotate_by_time(output) < 0) {
            ERROR_LOG("file rotate by time failed\n");
            goto failed;
        }
    }

    // TODO: line buffer - low write speed
    // setvbuf(ctx->fp, NULL, _IOLBF, 0);
    return 0;

failed:
    if (ctx->fp) {
        fclose(ctx->fp);
        ctx->fp = NULL;
    }
    return -1;
}


static int
file_emit(struct log_output *output, struct log_handler *handler)
{
    int ret                     = 0;
    log_buf_t *buf              = NULL;
    struct file_output_ctx *ctx = NULL;

    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (!handler) {
        ERROR_LOG("handler is NULL\n");
        return -1;
    }

    buf = handler->event.msg_buf;
    if (!buf) {
        ERROR_LOG("msg_buf is NULL\n");
        return -1;
    }

    ctx = (struct file_output_ctx *)output->ctx;
    if (!ctx) {
        ERROR_LOG("ctx is NULL\n");
        return -1;
    }

    if (!ctx->fp) {
        ret = file_open_logfile(output);
        if (ret < 0) {
            ERROR_LOG("open logfile failed\n");
            return -1;
        }
    }

    size_t len       = buf_len(buf);
    size_t file_left = check_can_write_bytes(output, handler);
    if (file_left >= len) {
        if (fwrite(buf->start, len, 1, ctx->fp) != 1) {
            ERROR_LOG("fwrite failed: (%s) len:%lu\n", strerror(errno), len);
            return -1;
        }
        ctx->data_offset += len;

        // flush
        uint64_t ts = log_get_ms();
        if (ts - ctx->flush_timestamp >= FLUSH_INTERVAL) {
            fflush(ctx->fp);
            /* fsync(fileno(ctx->fp)); */
            ctx->flush_timestamp = ts;
        }
        return len;
    }

    size_t nwrite      = 0;
    size_t total_write = 0;
    while (total_write < len) {
        nwrite =
            (len - total_write) > file_left ? file_left : (len - total_write);

        if (nwrite > 0) {
            if (fwrite(buf->start + total_write, nwrite, 1, ctx->fp) != 1) {
                ERROR_LOG(
                    "fwrite failed: (%s) nwrite: %lu total: %lu len: %lu left: "
                    "%lu\n",
                    strerror(errno), nwrite, total_write, len, file_left);
                return -1;
            }
            ctx->data_offset += nwrite;
            total_write += nwrite;
        }

        if (total_write >= len) {
            break;
        }

        ret = file_open_logfile(output);
        if (ret != 0) {
            ERROR_LOG("open logfile failed\n");
            return total_write;
        }

        file_left = check_can_write_bytes(output, handler);
        if (file_left == 0) {
            ERROR_LOG("after file_open_log file_left: %lu\n", file_left);
            return total_write;
        }
    }
    return total_write;
}

static void
file_ctx_dump(struct log_output *output)
{
    if (output) {
        DUMP_LOG("type: %s\n", output->priv->type_name);
        struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;
        if (ctx) {
            DUMP_LOG("filepath: %s\n", ctx->file_path);
            DUMP_LOG("logname:  %s\n", ctx->log_name);
            DUMP_LOG("filesize: %" PRIu64 "\n", ctx->file_size);
            DUMP_LOG("bakup:    %d\n", ctx->num_files);
            DUMP_LOG("offset:   %" PRIu64 "\n", ctx->data_offset);
            DUMP_LOG("idx:      %d\n", ctx->file_idx);
            DUMP_LOG("rotate:   %s\n",
                     ctx->rotate_police == ROTATE_POLICE_BY_SIZE ? "size" :
                                                                   "time");
        }
        dump_statstic(output);
    }
}

static void
file_ctx_uninit(struct log_output *output)
{
    struct file_output_ctx *ctx = NULL;

    if (!output) {
        ERROR_LOG("output is NULL\n");
        return;
    }
    ctx = (struct file_output_ctx *)output->ctx;
    if (!ctx) {
        return;
    }

    if (ctx->file_path) {
        free(ctx->file_path);
        ctx->file_path = NULL;
    }
    if (ctx->log_name) {
        free(ctx->log_name);
        ctx->log_name = NULL;
    }
    if (ctx->fp) {
        fclose(ctx->fp);
        ctx->fp = NULL;
    }
    free(ctx);
    ctx         = NULL;
    output->ctx = NULL;
}

static int
file_ctx_init(struct log_output *output, va_list ap)
{
    struct file_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (!output->ctx) {
        output->ctx =
            (struct file_output_ctx *)calloc(1, sizeof(struct file_output_ctx));
        if (!output->ctx) {
            ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
            goto failed;
        }
    }
    ctx = (struct file_output_ctx *)output->ctx;

    char *file_path = va_arg(ap, char *);
    ctx->file_path  = strdup(file_path);
    DEBUG_LOG("ctx->file_path: %s\n", ctx->file_path);

    char *file_name = va_arg(ap, char *);
    ctx->log_name   = strdup(file_name);
    DEBUG_LOG("ctx->log_name: %s\n", ctx->log_name);

    ctx->rotate_police = va_arg(ap, int);
    if (ctx->rotate_police == ROTATE_POLICE_BY_SIZE) {
        ctx->file_size = va_arg(ap, uint64_t);
        DEBUG_LOG("ctx->file_size: %lu\n", ctx->file_size);

        ctx->num_files = va_arg(ap, int);
        DEBUG_LOG("ctx->num_files: %d\n", ctx->num_files);

    } else if (ctx->rotate_police == ROTATE_POLICE_BY_TIME) {
        ;
    }

    ctx->fp       = NULL;
    ctx->file_idx = 0;

    if (file_open_logfile(output) != 0) {
        ERROR_LOG("open file failed\n");
        goto failed;
    }

    /* dump_environment(output); */
    return 0;

failed:
    if (ctx) {
        file_ctx_uninit(output);
    }
    return -1;
}

struct log_output_priv file_output_priv = {
    .type       = LOG_OUTTYPE_FILE,
    .type_name  = "file",
    .emit       = file_emit,
    .ctx_init   = file_ctx_init,
    .ctx_uninit = file_ctx_uninit,
    .dump       = file_ctx_dump,
};
