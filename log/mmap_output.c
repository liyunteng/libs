/*
 * mmap_output.c - mmap_output
 *
 * Date   : 2021/01/18
 */
#include "mmap_output.h"
#include "log.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_FILEPATH "."
#define DEFAULT_FILENAME "default"
#define DEFAULT_BAKUP 0
#define DEFAULT_FILESIZE 4 * 1024 * 1024
#define DEFAULT_MAP_SIZE 4 * 1024 * 1024
#define DEFAULT_MSYNC_INTERVAL 1 * 1000

inline static int
file_getname(struct log_output *output, char *file_name, uint16_t len)
{
    mmap_output_ctx *ctx = (mmap_output_ctx *)output->ctx;
    snprintf(file_name, len, "%s/%s.log", ctx->file_path, ctx->log_name);
    return 0;
}

inline static uint32_t
mmap_get_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

inline static int
mmap_msync_file(struct log_output *output)
{
    uint32_t cur;
    mmap_output_ctx *ctx = (mmap_output_ctx *)output->ctx;
    int ret;
    int page_size;
    int len;
    cur = mmap_get_ms();

    page_size = getpagesize();
    if (ctx->mmap_window.data_offset - ctx->mmap_window.msync_offset >= page_size) {
        if ((ctx->mmap_window.data_offset == ctx->mmap_window.window_size)
            || (cur - ctx->mmap_window.msync_time > ctx->msync_interval)) {
            len =
                ((ctx->mmap_window.data_offset - ctx->mmap_window.msync_offset)
                 / page_size)
                * page_size;
            ret = msync(ctx->mmap_window.addr + ctx->mmap_window.msync_offset,
                        len,
                        MS_ASYNC);
            if (ret < 0) {
                ERROR_LOG("msync failed(%s)\n", strerror(errno));
            }

            ctx->mmap_window.msync_time = cur;
            ctx->mmap_window.msync_offset += len;
            if (ctx->mmap_window.data_offset == ctx->mmap_window.window_size) {
                DEBUG_LOG(
                    "msync full: window_size: %u data_offset: %u "
                    "msync_offset: %u len: %d\n",
                    ctx->mmap_window.window_size,
                    ctx->mmap_window.data_offset,
                    ctx->mmap_window.msync_offset,
                    len);
            } else {
                DEBUG_LOG(
                    "msync timeout: window_size: %u data_offset: %u "
                    "mmap_window.msync_offset: %u len: %d\n",
                    ctx->mmap_window.window_size,
                    ctx->mmap_window.data_offset,
                    ctx->mmap_window.msync_offset,
                    len);
            }
        }
    }

    return 0;
}

static int
mmap_unmap_file(struct log_output *output)
{
    mmap_output_ctx *ctx = (mmap_output_ctx *)output->ctx;

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
    mmap_output_ctx *ctx = (mmap_output_ctx *)output->ctx;

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

    if (mmap_window_size > ctx->file_size - ctx->data_offset) {
        mmap_window_size = (ctx->file_size - ctx->data_offset + page_size - 1)
                           & (~(page_size - 1));
    }

    if (mmap_window_size == 0) {
        ERROR_LOG("mmap_window_size == 0\n");
        return -1;
    }

    ctx->file_current_size += mmap_window_size;
    lseek(ctx->fd, ctx->file_current_size, SEEK_SET);
    ftruncate(ctx->fd, ctx->file_current_size);

    ctx->mmap_window.addr = mmap(NULL, mmap_window_size, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, ctx->fd, ctx->mmap_window.offset);
    if (ctx->mmap_window.addr == MAP_FAILED) {
        ERROR_LOG("mmap failed(%s)\n", strerror(errno));
        return -1;
    }

    ctx->mmap_window.offset += mmap_window_size;
    ctx->mmap_window.window_size  = mmap_window_size;
    ctx->mmap_window.data_offset  = 0;
    ctx->mmap_window.msync_offset = 0;
    return 0;
}

static int
file_open_logfile(struct log_output *output)
{
    char *file_name;
    uint32_t len;
    mmap_output_ctx *ctx = (mmap_output_ctx *)output->ctx;

    if (ctx->fd != -1) {
        mmap_unmap_file(output);
        close(ctx->fd);
        ctx->fd = -1;
    }

    len = strlen(ctx->file_path);
    len += 1; /* "/" */
    len += strlen(ctx->log_name);
    len += 4; /* ".log" */
    len += 1; /* NULL char */

    file_name = malloc(len * sizeof(char));
    if (!file_name) {
        ERROR_LOG("malloc failed(%s)\n", strerror(errno));
        return -1;
    }

    file_getname(output, file_name, len);

    if ((ctx->fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
        ERROR_LOG("fopen %s failed:(%s)\n", file_name, strerror(errno));
        free(file_name);
        return -1;
    }

    ctx->data_offset        = 0;
    ctx->file_current_size  = 0;
    ctx->mmap_window.offset = 0;

    if (mmap_map_file(output) != 0) {
        return -1;
    }
    free(file_name);
    return 0;
}

static int
file_rename_logfile(struct log_output *output)
{
    uint32_t num, num_files, len;
    char *old_file_name, *new_file_name;

    mmap_output_ctx *ctx = (mmap_output_ctx *)output->ctx;

    if (ctx->num_files > 0) {
        for (num = 0, num_files = ctx->num_files; num_files; num_files /= 10) {
            ++num;
        }

        len = strlen(ctx->file_path);
        len += 1; /* "/" */

        len += strlen(ctx->log_name);
        len += 4;         /* ".log" */
        len += (num + 1); /* ".<digit>" */
        len += 1;         /* NULL char */

        old_file_name = malloc(len * sizeof(char));
        new_file_name = malloc(len * sizeof(char));
        if (!old_file_name || !new_file_name) {
            ERROR_LOG("malloc failed(%s)\n", strerror(errno));
            free(old_file_name);
            free(new_file_name);
            return -1;
        }

        file_getname(output, old_file_name, len);
        snprintf(new_file_name, len, "%s.%u", old_file_name,
                 (ctx->file_idx % ctx->num_files));
        rename(old_file_name, new_file_name);

        ++ctx->file_idx;
        DEBUG_LOG("rename %s ==> %s\n", old_file_name, new_file_name);
        free(old_file_name);
        free(new_file_name);
    }
    return 0;
}

static int
mmap_emit(struct log_output *output, struct log_handler *handler)
{
    int ret;
    mmap_output_ctx *ctx = NULL;
    log_buf_t *buf       = NULL;
    size_t len, file_left, mmap_left;
    uint32_t cur;

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

    ctx = (mmap_output_ctx *)output->ctx;
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

    len       = buf_len(buf);
    file_left = ctx->file_size - ctx->data_offset;
    mmap_left = ctx->mmap_window.window_size - ctx->mmap_window.data_offset;
    if (file_left >= len && mmap_left >= len) {
        memcpy(ctx->mmap_window.addr + ctx->mmap_window.data_offset, buf->start,
               len);

        ctx->mmap_window.data_offset += len;
        ctx->data_offset += len;

        mmap_msync_file(output);
        return len;
    }

    int nwrite   = 0;
    size_t total = 0;
    int left     = 0;
    while (total < len) {
        if (mmap_left > file_left) {
            left = file_left;
        } else {
            left = mmap_left;
        }
        if (len - total > left) {
            nwrite = left;
        } else {
            nwrite = len - total;
        }

        if (nwrite > 0) {
            memcpy(ctx->mmap_window.addr + ctx->mmap_window.data_offset,
                   buf->start + total, nwrite);

            ctx->mmap_window.data_offset += nwrite;
            ctx->data_offset += nwrite;
            total += nwrite;

            mmap_msync_file(output);
        }

        if (total >= len) {
            break;
        }

        if (nwrite == file_left) {
            file_rename_logfile(output);
            ret = file_open_logfile(output);
            if (ret != 0) {
                ERROR_LOG("open logfile failed\n");
                return total;
            }
        } else if (nwrite == mmap_left) {
            ret = mmap_map_file(output);
            if (ret != 0) {
                ERROR_LOG("mmap map filed\n");
                return total;
            }
        }

        file_left = ctx->file_size - ctx->data_offset;
        mmap_left = ctx->mmap_window.window_size - ctx->mmap_window.data_offset;
    }
    return total;
}

static void
mmap_ctx_dump(struct log_output *output)
{
    if (output) {
        printf("type: %s\n", output->type_name);
        mmap_output_ctx *ctx = (mmap_output_ctx *)output->ctx;
        if (ctx) {
            printf("filepath:       %s\n", ctx->file_path);
            printf("logname:        %s\n", ctx->log_name);
            printf("filesize:       %u\n", ctx->file_size);
            printf("currentsize:    %u\n", ctx->file_current_size);
            printf("bakup:          %d\n", ctx->num_files);
            printf("idx:            %d\n", ctx->file_idx);
            printf("map_size:       %u\n", ctx->map_size);
            printf("msync_interval: %u\n", ctx->msync_interval);
            printf("data_offset:    %u\n", ctx->data_offset);
            printf("mmap.addr:      %p\n", ctx->mmap_window.addr);
            printf("mmap.size:      %u\n", ctx->mmap_window.window_size);
            printf("mmap.offset:    %u\n", ctx->mmap_window.offset);
            printf("mmap.doffset:   %u\n", ctx->mmap_window.data_offset);
        }
        dump_statstic(output);
    }
}

static void
mmap_ctx_uninit(struct log_output *output)
{
    mmap_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return;
    }
    ctx = (mmap_output_ctx *)output->ctx;
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
    mmap_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (output->ctx == NULL) {
        output->ctx = (mmap_output_ctx *)calloc(1, sizeof(mmap_output_ctx));
        if (output->ctx == NULL) {
            ERROR_LOG("calloc failed(%s)\n", strerror(errno));
            goto failed;
        }
    }
    ctx = (mmap_output_ctx *)output->ctx;

    char *file_path = va_arg(ap, char *);
    if (file_path && strlen(file_path) > 0) {
        ctx->file_path = strdup(file_path);
    } else {
        ctx->file_path = strdup(DEFAULT_FILEPATH);
    }

    char *file_name = va_arg(ap, char *);
    if (file_name && strlen(file_name) > 0) {
        ctx->log_name = strdup(file_name);
    } else {
        ctx->log_name = strdup(DEFAULT_FILENAME);
    }

    size_t file_size = va_arg(ap, size_t);
    if (file_size > 0) {
        ctx->file_size = file_size;
    } else {
        ctx->file_size = DEFAULT_FILESIZE;
    }


    int num_files = va_arg(ap, int);
    if (num_files >= 0) {
        ctx->num_files = num_files;
    } else {
        ctx->num_files = DEFAULT_BAKUP;
    }

    size_t map_size = va_arg(ap, size_t);
    if (map_size > 0) {
        ctx->map_size = map_size;
    } else {
        ctx->map_size = DEFAULT_MAP_SIZE;
    }

    size_t msync_interval = va_arg(ap, size_t);
    if (msync_interval > 0) {
        ctx->msync_interval = msync_interval;
    } else {
        ctx->msync_interval = DEFAULT_MSYNC_INTERVAL;
    }

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

struct log_output *
mmap_output_create(void)
{
    struct log_output *output = NULL;
    output               = (struct log_output *)calloc(1, sizeof(struct log_output));
    if (!output) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }

    output->type       = LOG_OUTTYPE_MMAP;
    output->type_name  = "file mmap";
    output->emit       = mmap_emit;
    output->ctx_init   = mmap_ctx_init;
    output->ctx_uninit = mmap_ctx_uninit;
    output->dump       = mmap_ctx_dump;

    return output;
}
