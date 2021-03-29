/*
 * mmap_output.c - mmap_output
 *
 * Date   : 2021/01/18
 */
#include "mmap_output.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_FILE_PATH 256

struct mmap_output_ctx {
    char *file_path;
    char *log_name;

    int rotate_police;
    uint16_t num_files;
    uint16_t file_idx;
    uint64_t file_size;
    uint64_t data_offset; /* data offset to file */
    uint64_t file_timestamp;

    uint32_t map_size;
    uint32_t msync_interval;
    struct {
        void *addr;
        uint32_t window_size; /* mmap size */
        uint32_t msync_offset;
        uint64_t offset;      /* addr offset to file  */
        uint64_t data_offset; /* data offset to addr */
        uint64_t msync_time;
    } mmap_window;

    int fd;
    uint64_t file_current_size;
};

static inline uint64_t
mmap_get_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static inline int
mmap_msync_file(struct log_output *output)
{
    int ret;
    int page_size;
    int len;
    uint64_t cur                = mmap_get_ms();
    struct mmap_output_ctx *ctx = (struct mmap_output_ctx *)output->ctx;

    page_size = getpagesize();
    if (ctx->mmap_window.data_offset - ctx->mmap_window.msync_offset
        >= page_size) {
        if ((ctx->mmap_window.data_offset == ctx->mmap_window.window_size)
            || (cur - ctx->mmap_window.msync_time > ctx->msync_interval)) {
            len =
                ((ctx->mmap_window.data_offset - ctx->mmap_window.msync_offset)
                 / page_size)
                * page_size;
            ret = msync(ctx->mmap_window.addr + ctx->mmap_window.msync_offset,
                        len, MS_ASYNC);
            if (ret < 0) {
                ERROR_LOG("msync failed: (%s)\n", strerror(errno));
            }

            ctx->mmap_window.msync_time = cur;
            ctx->mmap_window.msync_offset += len;
            if (ctx->mmap_window.data_offset == ctx->mmap_window.window_size) {
                /* DEBUG_LOG(
                 *     "msync full: window_size: %u data_offset: %u "
                 *     "msync_offset: %u len: %d\n",
                 *     ctx->mmap_window.window_size, ctx->mmap_window.data_offset,
                 *     ctx->mmap_window.msync_offset, len); */
            } else {
                DEBUG_LOG(
                    "msync timeout: window_size: %u data_offset: %lu "
                    "mmap_window.msync_offset: %u len: %d\n",
                    ctx->mmap_window.window_size, ctx->mmap_window.data_offset,
                    ctx->mmap_window.msync_offset, len);
            }
        }
    }

    return 0;
}

static int
mmap_unmap_file(struct log_output *output)
{
    struct mmap_output_ctx *ctx = (struct mmap_output_ctx *)output->ctx;

    if (ctx->mmap_window.addr) {
        mmap_msync_file(output);
        munmap(ctx->mmap_window.addr, ctx->mmap_window.window_size);
        ctx->mmap_window.addr         = NULL;
        ctx->mmap_window.data_offset  = 0;
        ctx->mmap_window.window_size  = 0;
        ctx->mmap_window.msync_offset = 0;
        ctx->mmap_window.msync_time   = mmap_get_ms();
        return 0;
    }
    return -1;
}

static int
mmap_map_file(struct log_output *output)
{
    size_t mmap_window_size;
    int page_size;
    struct mmap_output_ctx *ctx = (struct mmap_output_ctx *)output->ctx;

    if (ctx->fd < 0) {
        ERROR_LOG("file not open\n");
        return -1;
    }

    if (ctx->mmap_window.offset == 0) {
        memset(&ctx->mmap_window, 0, sizeof(ctx->mmap_window));
    }

    if (ctx->mmap_window.addr) {
        mmap_unmap_file(output);
    }

    page_size        = getpagesize();
    mmap_window_size = (ctx->map_size + page_size - 1) & (~(page_size - 1));

    if (ctx->file_size > 0
        && (mmap_window_size > ctx->file_size - ctx->data_offset)) {
        mmap_window_size = ((ctx->file_size - ctx->data_offset + page_size - 1)
                            & (~(page_size - 1)));
    }

    if (mmap_window_size == 0) {
        ERROR_LOG("mmap_window_size == 0\n");
        return -1;
    }

    ctx->file_current_size += mmap_window_size;
    /* lseek(ctx->fd, ctx->file_current_size, SEEK_SET); */
    if (ftruncate(ctx->fd, ctx->file_current_size) != 0) {
        ERROR_LOG("ftrucate failed: (%s)\n", strerror(errno));
        return -1;
    }

    ctx->mmap_window.addr = mmap(NULL, mmap_window_size, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, ctx->fd, ctx->mmap_window.offset);
    if (ctx->mmap_window.addr == MAP_FAILED) {
        ERROR_LOG("mmap failed: (%s)\n", strerror(errno));
        return -1;
    }

    ctx->mmap_window.offset += mmap_window_size;
    ctx->mmap_window.window_size  = mmap_window_size;
    ctx->mmap_window.data_offset  = 0;
    ctx->mmap_window.msync_offset = 0;
    return 0;
}

static size_t
check_can_write_bytes(struct log_output *output, struct log_handler *handler)
{
    struct mmap_output_ctx *ctx = (struct mmap_output_ctx *)output->ctx;
    uint64_t ts;
    size_t left = buf_len(handler->event.msg_buf);

    if (ctx->rotate_police == ROTATE_POLICE_BY_SIZE) {
        if (ctx->file_size > 0) {
            left = ctx->file_size - ctx->data_offset;
        }
    } else if (ctx->rotate_police == ROTATE_POLICE_BY_TIME) {
        if (handler->event.timestamp.tv_sec != 0) {
            ts = handler->event.timestamp.tv_sec * 1e3
                 + handler->event.timestamp.tv_usec / 1e3;
        } else {
            ts = mmap_get_ms();
        }

        if ((ts - ctx->file_timestamp) > 60 * 1000) {
            DEBUG_LOG("ts: %lu  ctx->file_timestamp: %lu\n", ts,
                      ctx->file_timestamp);
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
    struct mmap_output_ctx *ctx = (struct mmap_output_ctx *)output->ctx;

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

    ++ctx->file_idx;
    return 0;
}

static int
file_rotate_by_time(struct log_output *output)
{
    char file_name[MAX_FILE_PATH] = {0};
    struct stat st;
    struct mmap_output_ctx *ctx = (struct mmap_output_ctx *)output->ctx;

    snprintf(file_name, MAX_FILE_PATH - 1, "%s/%s.log", ctx->file_path,
             ctx->log_name);

    if (lstat(file_name, &st) < 0) {
        if (errno == ENOENT) {
            // create new file
            if ((ctx->fd = open(file_name, O_RDWR | O_CREAT, 0644)) < 0) {
                ERROR_LOG("open %s failed: (%s)\n", file_name, strerror(errno));
                return -1;
            }
            DEBUG_LOG("open file(new) %s\n", file_name);
            ctx->data_offset       = 0;
            ctx->file_current_size = 0;
            ctx->file_timestamp    = mmap_get_ms();

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

    if ((ctx->fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
        ERROR_LOG("open %s failed: (%s)\n", file_name, strerror(errno));
        return -1;
    }
    DEBUG_LOG("open file(truncated) %s\n", file_name);
    ctx->data_offset       = 0;
    ctx->file_current_size = 0;
    ctx->file_timestamp    = mmap_get_ms();

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
    struct mmap_output_ctx *ctx = (struct mmap_output_ctx *)output->ctx;

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
    struct mmap_output_ctx *ctx = (struct mmap_output_ctx *)output->ctx;

    snprintf(file_name, MAX_FILE_PATH - 1, "%s/%s.log", ctx->file_path,
             ctx->log_name);

    if (lstat(file_name, &st) < 0) {
        if (errno == ENOENT) {
            // create new file
            if ((ctx->fd = open(file_name, O_RDWR | O_CREAT, 0644)) < 0) {
                ERROR_LOG("open %s failed: (%s)\n", file_name, strerror(errno));
                return -1;
            }
            DEBUG_LOG("open file(new) %s\n", file_name);
            ctx->data_offset       = 0;
            ctx->file_current_size = 0;
            ctx->file_timestamp    = mmap_get_ms();
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
        if ((ctx->fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
            ERROR_LOG("fopen %s failed: (%s)\n", file_name, strerror(errno));
            return -1;
        }
        DEBUG_LOG("open file(truncated) %s\n", file_name);
        ctx->data_offset       = 0;
        ctx->file_current_size = 0;
        ctx->file_timestamp    = mmap_get_ms();
        return 0;
    }

    // append
    if ((ctx->fd = open(file_name, O_RDWR | O_CREAT | O_APPEND, 0644)) < 0) {
        ERROR_LOG("fopen %s(append) failed: (%s)\n", file_name,
                  strerror(errno));
        return -1;
    }
    ctx->data_offset       = st.st_size;
    ctx->file_current_size = st.st_size;
    ctx->file_timestamp    = mmap_get_ms();
    return 0;
}

static int
file_open_logfile(struct log_output *output)
{
    struct stat st;
    int need_create_dir = 0;

    struct mmap_output_ctx *ctx = (struct mmap_output_ctx *)output->ctx;

    if (ctx->fd != -1) {
        mmap_unmap_file(output);
        close(ctx->fd);
        ctx->fd = -1;
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

    ctx->mmap_window.offset = 0;
    if (mmap_map_file(output) != 0) {
        goto failed;
    }
    return 0;

failed:
    if (ctx->fd != -1) {
        close(ctx->fd);
        ctx->fd = -1;
    }
    return -1;
}

static int
mmap_emit(struct log_output *output, struct log_handler *handler)
{
    int ret;
    struct mmap_output_ctx *ctx = NULL;
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

    ctx = (struct mmap_output_ctx *)output->ctx;
    if (!ctx) {
        ERROR_LOG("ctx is NULL\n");
        return -1;
    }

    if (ctx->fd < 0) {
        ret = file_open_logfile(output);
        if (ret < 0) {
            ERROR_LOG("open logfile failed\n");
            return -1;
        }
    }

    len              = buf_len(buf);
    size_t file_left = check_can_write_bytes(output, handler);
    size_t mmap_left =
        ctx->mmap_window.window_size - ctx->mmap_window.data_offset;
    if (file_left >= len && mmap_left >= len) {
        memcpy(ctx->mmap_window.addr + ctx->mmap_window.data_offset, buf->start,
               len);

        ctx->mmap_window.data_offset += len;
        ctx->data_offset += len;

        mmap_msync_file(output);
        return len;
    }

    size_t nwrite      = 0;
    size_t total_write = 0;
    while (total_write < len) {
        nwrite =
            (len - total_write) > file_left ? file_left : (len - total_write);
        nwrite = mmap_left > nwrite ? nwrite : mmap_left;

        if (nwrite > 0) {
            memcpy(ctx->mmap_window.addr + ctx->mmap_window.data_offset,
                   buf->start + total_write, nwrite);

            ctx->mmap_window.data_offset += nwrite;
            ctx->data_offset += nwrite;
            total_write += nwrite;

            mmap_msync_file(output);
        }

        if (total_write >= len) {
            break;
        }

        if (file_left == 0) {
            ret = file_open_logfile(output);
            if (ret != 0) {
                ERROR_LOG("open logfile failed\n");
                return total_write;
            }
        } else {
            ret = mmap_map_file(output);
            if (ret != 0) {
                ERROR_LOG("mmap map filed\n");
                return total_write;
            }
        }
        file_left = check_can_write_bytes(output, handler);
        mmap_left = ctx->mmap_window.window_size - ctx->mmap_window.data_offset;
    }
    return total_write;
}

static void
mmap_ctx_dump(struct log_output *output)
{
    if (output) {
        printf("type: %s\n", output->priv->type_name);
        struct mmap_output_ctx *ctx = (struct mmap_output_ctx *)output->ctx;
        if (ctx) {
            printf("filepath:       %s\n", ctx->file_path);
            printf("logname:        %s\n", ctx->log_name);
            printf("filesize:       %lu\n", ctx->file_size);
            printf("currentsize:    %lu\n", ctx->file_current_size);
            printf("bakup:          %d\n", ctx->num_files);
            printf("idx:            %d\n", ctx->file_idx);
            printf("rotate          %s\n",
                   ctx->rotate_police == ROTATE_POLICE_BY_SIZE ? "size" :
                                                                 "time");
            printf("map_size:       %u\n", ctx->map_size);
            printf("msync_interval: %u\n", ctx->msync_interval);
            printf("data_offset:    %lu\n", ctx->data_offset);
            printf("mmap.addr:      %p\n", ctx->mmap_window.addr);
            printf("mmap.size:      %u\n", ctx->mmap_window.window_size);
            printf("mmap.offset:    %lu\n", ctx->mmap_window.offset);
            printf("mmap.doffset:   %lu\n", ctx->mmap_window.data_offset);
        }
        dump_statstic(output);
    }
}

static void
mmap_ctx_uninit(struct log_output *output)
{
    struct mmap_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return;
    }
    ctx = (struct mmap_output_ctx *)output->ctx;
    if (ctx == NULL) {
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

    mmap_unmap_file(output);

    if (ctx->fd != -1) {
        close(ctx->fd);
        ctx->fd = -1;
    }
    free(ctx);
    ctx         = NULL;
    output->ctx = NULL;
}

static int
mmap_ctx_init(struct log_output *output, va_list ap)
{
    struct mmap_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (output->ctx == NULL) {
        output->ctx =
            (struct mmap_output_ctx *)calloc(1, sizeof(struct mmap_output_ctx));
        if (output->ctx == NULL) {
            ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
            goto failed;
        }
    }
    ctx = (struct mmap_output_ctx *)output->ctx;

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

    ctx->map_size = va_arg(ap, uint32_t);
    DEBUG_LOG("ctx->map_size: %u\n", ctx->map_size);

    ctx->msync_interval = va_arg(ap, uint32_t);
    DEBUG_LOG("ctx->msync_interval: %u\n", ctx->msync_interval);

    ctx->fd       = -1;
    ctx->file_idx = 0;

    if (file_open_logfile(output) != 0) {
        ERROR_LOG("open file failed\n");
        goto failed;
    }

    /* dump_environment(output); */
    return 0;

failed:
    if (ctx) {
        mmap_ctx_uninit(output);
    }
    return -1;
}

struct log_output_priv mmap_output_priv = {
    .type       = LOG_OUTTYPE_MMAP,
    .type_name  = "file mmap",
    .emit       = mmap_emit,
    .ctx_init   = mmap_ctx_init,
    .ctx_uninit = mmap_ctx_uninit,
    .dump       = mmap_ctx_dump,
};
