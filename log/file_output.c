/*
 * file_output.c - file_output
 *
 * Date   : 2021/01/15
 */
#include "file_output.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#define MAX_FILE_PATH 256

struct file_output_ctx {
    char *file_path;
    char *log_name;

    int rotate_police;
    uint16_t num_files;
    uint16_t file_idx;
    uint64_t file_size;
    uint64_t data_offset;
    uint64_t file_timestamp;

    FILE *fp;
};

/* pointer to environment */
extern char **environ;

/* dump the environment */
static void
dump_environment(struct log_output *output)
{
    static char buf[BUFSIZ];
    int cnt                     = 0;
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

static inline uint64_t
file_get_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1e3 + tv.tv_usec / 1e3;
}

static size_t
check_can_write_bytes(struct log_output *output, struct log_handler *handler)
{
    struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;
    uint64_t ts;
    int left = buf_len(handler->event.msg_buf);

    if (ctx->rotate_police == ROTATE_POLICE_BY_SIZE) {
        if (ctx->file_size > 0) {
            left = ctx->file_size - ctx->data_offset;
        }
    } else if (ctx->rotate_police == ROTATE_POLICE_BY_TIME) {
        if (handler->event.timestamp.tv_sec != 0) {
            ts = handler->event.timestamp.tv_sec * 1e3
                 + handler->event.timestamp.tv_usec / 1e3;
        } else {
            ts = file_get_ms();
        }

        if (ts - ctx->file_timestamp > 60 * 60 * 1e3) {
            left = 0;
        }
    }
    return left;
}

static int
do_file_rotate_by_time(struct log_output *output)
{
    char old_file_name[MAX_FILE_PATH] = {0};
    char new_file_name[MAX_FILE_PATH] = {0};
    struct tm tm;
    char time[128] = {0};
    struct stat st;
    int i                       = 0;
    struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;

    snprintf(old_file_name, MAX_FILE_PATH - 1, "%s/%s.log", ctx->file_path,
             ctx->log_name);

    if (lstat(old_file_name, &st) < 0) {
        ERROR_LOG("lstat failed: (%s)\n", strerror(errno));
        return -1;
    }

    localtime_r(&(st.st_atime), &tm);
    strftime(time, 127, "%04Y%02m%02d%02H%02M%02S", &tm);
    snprintf(new_file_name, MAX_FILE_PATH - 1, "%s/%s.log.%s", ctx->file_path,
             ctx->log_name, time);
    rename(old_file_name, new_file_name);
    DEBUG_LOG("rename %s ==> %s\n", old_file_name, new_file_name);

    return 0;
}

static int
file_rotate_by_time(struct log_output *output)
{
    char file_name[MAX_FILE_PATH] = {0};
    struct stat st;
    struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;

    snprintf(file_name, MAX_FILE_PATH - 1, "%s/%s.log", ctx->file_path,
             ctx->log_name);

    if (lstat(file_name, &st) < 0) {
        if (errno == ENOENT) {
            // create new file
            if ((ctx->fp = fopen(file_name, "w")) == NULL) {
                ERROR_LOG("fopen %s failed: (%s)\n", file_name,
                          strerror(errno));
                return -1;
            }
            DEBUG_LOG("open file(new) %s\n", file_name);
            ctx->file_timestamp = file_get_ms();
            ctx->data_offset    = 0;
            return 0;
        }

        ERROR_LOG("lstat %s failed: (%s)\n", file_name, strerror(errno));
        return -1;
    }

    // rotate
    if (do_file_rotate_by_time(output) < 0) {
        ERROR_LOG("rename %s failed\n", file_name);
        return -1;
    }

    if ((ctx->fp = fopen(file_name, "w")) == NULL) {
        ERROR_LOG("fopen %s failed: (%s)\n", file_name, strerror(errno));
        return -1;
    }
    DEBUG_LOG("open file(truncated) %s\n", file_name);
    ctx->file_timestamp = file_get_ms();
    ctx->data_offset    = 0;

    ++ctx->file_idx;

    return 0;
}

static int
do_file_rotate_by_size(struct log_output *output)
{
    char old_file_name[MAX_FILE_PATH] = {0};
    char new_file_name[MAX_FILE_PATH] = {0};
    struct stat st;
    uint32_t bak_file_num;
    int i                       = 0;
    struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;

    // don't rotate
    if (ctx->num_files == 0) {
        return 0;
    }

    // rotate
    bak_file_num = 0;
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
    char file_name[MAX_FILE_PATH] = {0};
    struct stat st;
    struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;

    snprintf(file_name, MAX_FILE_PATH - 1, "%s/%s.log", ctx->file_path,
             ctx->log_name);

    if (lstat(file_name, &st) < 0) {
        if (errno == ENOENT) {
            // create new file
            if ((ctx->fp = fopen(file_name, "w")) == NULL) {
                ERROR_LOG("fopen %s failed: (%s)\n", file_name,
                          strerror(errno));
                return -1;
            }
            DEBUG_LOG("open file(new) %s\n", file_name);
            ctx->data_offset    = 0;
            ctx->file_timestamp = file_get_ms();
            return 0;
        }

        ERROR_LOG("lstat %s failed: (%s)\n", file_name, strerror(errno));
        return -1;
    }

    // rotate
    if (ctx->file_size > 0) {
        if (ctx->num_files != 0) {
            if (do_file_rotate_by_size(output) < 0) {
                ERROR_LOG("rename %s failed\n", file_name);
                return -1;
            }
        }

        // truncated
        if ((ctx->fp = fopen(file_name, "w")) == NULL) {
            ERROR_LOG("fopen %s failed: (%s)\n", file_name, strerror(errno));
            return -1;
        }
        DEBUG_LOG("open file(truncated) %s\n", file_name);
        ctx->data_offset    = 0;
        ctx->file_timestamp = file_get_ms();
        return 0;
    }

    // append
    if ((ctx->fp = fopen(file_name, "a+")) == NULL) {
        ERROR_LOG("fopen %s(append) failed: (%s)\n", file_name,
                  strerror(errno));
        return -1;
    }
    ctx->data_offset    = st.st_size;
    ctx->file_timestamp = file_get_ms();
    return 0;
}


static int
file_open_logfile(struct log_output *output)
{
    struct stat st;
    int need_create_dir = 0;

    struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;

    if (ctx->fp != NULL) {
        fclose(ctx->fp);
        ctx->fp = NULL;
    }

    if (lstat(ctx->file_path, &st) < 0) {
        if (errno != ENOENT) {
            ERROR_LOG("lstat %s failed: (%s)\n", ctx->file_path,
                      strerror(errno));
            goto failed;
        }
        need_create_dir = 1;
    } else {
        if ((st.st_mode & S_IFMT) != S_IFDIR) {
            ERROR_LOG("%s is not directory\n", ctx->file_path);
            goto failed;
        }
    }

    if (need_create_dir) {
        if (mkdir(ctx->file_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
            ERROR_LOG("mkdir %s failed: (%s)\n", ctx->file_path,
                      strerror(errno));
            goto failed;
        }
        DEBUG_LOG("mkdir %s\n", ctx->file_path);
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
    int ret;
    struct file_output_ctx *ctx = NULL;
    log_buf_t *buf              = NULL;
    size_t len;

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

    len      = buf_len(buf);
    int file_left = check_can_write_bytes(output, handler);
    if (file_left >= len) {
        if (fwrite(buf->start, len, 1, ctx->fp) != 1) {
            ERROR_LOG("fwrite failed: (%s) len:%lu\n", strerror(errno), len);
            return -1;
        }
        ctx->data_offset += len;
        return len;
    }

    size_t nwrite      = 0;
    size_t total_write = 0;
    while (total_write < len) {
        nwrite = (len - total_write) > file_left ? file_left : (len - total_write);

        if (nwrite > 0) {
            if (fwrite(buf->start + total_write, nwrite, 1, ctx->fp) != 1) {
                ERROR_LOG(
                    "fwrite failed: (%s) nwrite: %lu total: %lu len: %lu left: "
                    "%d\n",
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
        file_left = ctx->file_size;
    }
    return total_write;
}

static void
file_ctx_dump(struct log_output *output)
{
    if (output) {
        printf("type: %s\n", output->priv->type_name);
        struct file_output_ctx *ctx = (struct file_output_ctx *)output->ctx;
        if (ctx) {
            printf("filepath: %s\n", ctx->file_path);
            printf("logname:  %s\n", ctx->log_name);
            printf("filesize: %lu\n", ctx->file_size);
            printf("bakup:    %d\n", ctx->num_files);
            printf("offset:   %lu\n", ctx->data_offset);
            printf("idx:      %d\n", ctx->file_idx);
            printf("rotate:   %s\n",
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
