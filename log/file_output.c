/*
 * file_output.c - file_output
 *
 * Date   : 2021/01/15
 */
#include "file_output.h"
#include "buf.h"
#include "log.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

/* pointer to environment */
extern char **environ;

/* dump the environment */
static void
dump_environment(struct log_output *output)
{
    static char buf[BUFSIZ];
    int cnt              = 0;
    file_output_ctx *ctx = (file_output_ctx *)output->ctx;

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

static int file_rotate(struct log_output *output)
{
    uint32_t num, num_files, len;
    char *old_file_name, *new_file_name;
    struct stat st;
    uint32_t bak_file_num;
    int i = 0;
    file_output_ctx *ctx = (file_output_ctx *)output->ctx;

    // don't rotate
    if (ctx->num_files == 0) {
        return 0;
    }

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
        ERROR_LOG("malloc failed: (%s)\n", strerror(errno));
        free(old_file_name);
        free(new_file_name);
        return -1;
    }

    bak_file_num = 0;
    for (i = 0; i < ctx->num_files; i++) {
        snprintf(old_file_name, len, "%s/%s.log.%u", ctx->file_path,
                 ctx->log_name, i);
        if (lstat(old_file_name, &st) < 0) {
            if (errno == ENOENT) {
                break;
            }
        }
        bak_file_num++;
    }

    for (i = bak_file_num; i >= 0; i--) {
        if (i == ctx->num_files) {
            continue;
        }

        if (i == 0) {
            snprintf(old_file_name, len, "%s/%s.log", ctx->file_path,
                     ctx->log_name);
        } else {
            snprintf(old_file_name, len, "%s/%s.log.%u", ctx->file_path,
                     ctx->log_name, (i - 1));
        }

        snprintf(new_file_name, len, "%s/%s.log.%u", ctx->file_path,
                 ctx->log_name, i);
        rename(old_file_name, new_file_name);
        DEBUG_LOG("rename %s ==> %s\n", old_file_name, new_file_name);
    }

    ++ctx->file_idx;
    free(old_file_name);
    free(new_file_name);

    return 0;
}

static int
file_open_logfile(struct log_output *output)
{
    char *file_name;
    uint32_t len;
    struct stat st;
    int need_create_dir = 0;

    file_output_ctx *ctx = (file_output_ctx *)output->ctx;

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

    len = strlen(ctx->file_path);
    len += 1; /* "/" */
    len += strlen(ctx->log_name);
    len += 4; /* ".log" */
    len += 1; /* NULL char */

    file_name = malloc(len * sizeof(char));
    if (!file_name) {
        ERROR_LOG("malloc failed: (%s)\n", strerror(errno));
        return -1;
    }

    snprintf(file_name, len, "%s/%s.log", ctx->file_path, ctx->log_name);

    if (lstat(file_name, &st) < 0) {
        if (errno == ENOENT) {
            // create new file
            if ((ctx->fp = fopen(file_name, "w")) == NULL) {
                ERROR_LOG("fopen %s failed: (%s)\n", file_name,
                          strerror(errno));
                goto failed;
            }
            DEBUG_LOG("open file(create) %s\n", file_name);
            ctx->data_offset = 0;
        } else {
            ERROR_LOG("lstat %s failed: (%s)\n", file_name, strerror(errno));
            goto failed;
        }
    } else {
        // file exits

        // rotate
        if (ctx->file_size > 0 && ctx->num_files > 0) {
            if (file_rotate(output) < 0) {
                ERROR_LOG("rename %s failed\n", file_name);
                goto failed;
            }
            DEBUG_LOG("create file %s\n", file_name);
            st.st_size = 0;
        }

        if (ctx->file_size > 0 && ctx->file_size <= st.st_size) {
            // truncated
            if ((ctx->fp = fopen(file_name, "w")) == NULL) {
                ERROR_LOG("fopen %s failed: (%s)\n", file_name,
                          strerror(errno));
                goto failed;
            }
            DEBUG_LOG("open file(truncated) %s\n", file_name);
            ctx->data_offset = 0;
        } else {
            // append
            if ((ctx->fp = fopen(file_name, "a+")) == NULL) {
                ERROR_LOG("fopen %s(append) failed: (%s)\n", file_name,
                          strerror(errno));
                goto failed;
            }
            ctx->data_offset = st.st_size;
        }
    }

    free(file_name);
    return 0;

failed:
    if (ctx->fp) {
        fclose(ctx->fp);
        ctx->fp = NULL;
    }
    if (file_name) {
        free(file_name);
    }
    return -1;
}


static int
file_emit(struct log_output *output, struct log_handler *handler)
{
    int ret;
    file_output_ctx *ctx = NULL;
    log_buf_t *buf       = NULL;
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

    ctx = (file_output_ctx *)output->ctx;
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
    int left = 0;
    if (ctx->file_size > 0) {
        left = ctx->file_size - ctx->data_offset;
    }

    if ((ctx->file_size > 0 && left > len) || ctx->file_size == 0) {
        if (fwrite(buf->start, len, 1, ctx->fp) != 1) {
            ERROR_LOG("fwrite failed: (%s) len:%lu\n", strerror(errno), len);
            return -1;
        }
        ctx->data_offset += len;
        return len;
    }

    size_t nwrite = 0;
    size_t total  = 0;
    while (total < len) {
        if (len - total > left) {
            nwrite = left;
        } else {
            nwrite = len - total;
        }

        if (nwrite > 0) {
            if (fwrite(buf->start + total, nwrite, 1, ctx->fp) != 1) {
                ERROR_LOG(
                    "fwrite failed: (%s) nwrite: %lu total: %lu len: %lu left: "
                    "%d\n",
                    strerror(errno), nwrite, total, len, left);
                return -1;
            }
            ctx->data_offset += nwrite;
            total += nwrite;
        }

        if (total >= len) {
            break;
        }

        ret = file_open_logfile(output);
        if (ret != 0) {
            ERROR_LOG("open logfile failed\n");
            return total;
        }
        left = ctx->file_size;
    }
    return total;
}

static void
file_ctx_dump(struct log_output *output)
{
    if (output) {
        printf("type: %s\n", output->priv->type_name);
        file_output_ctx *ctx = (file_output_ctx *)output->ctx;
        if (ctx) {
            printf("filepath: %s\n", ctx->file_path);
            printf("logname:  %s\n", ctx->log_name);
            printf("filesize: %d\n", ctx->file_size);
            printf("bakup:    %d\n", ctx->num_files);
            printf("idx:      %d\n", ctx->file_idx);
            printf("offset:   %u\n", ctx->data_offset);
        }
        dump_statstic(output);
    }
}

static void
file_ctx_uninit(struct log_output *output)
{
    file_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return;
    }
    ctx = (file_output_ctx *)output->ctx;
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
    file_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (!output->ctx) {
        output->ctx = (file_output_ctx *)calloc(1, sizeof(file_output_ctx));
        if (!output->ctx) {
            ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
            goto failed;
        }
    }
    ctx = (file_output_ctx *)output->ctx;

    char *file_path = va_arg(ap, char *);
    ctx->file_path  = strdup(file_path);
    DEBUG_LOG("ctx->file_path: %s\n", ctx->file_path);

    char *file_name = va_arg(ap, char *);
    ctx->log_name   = strdup(file_name);
    DEBUG_LOG("ctx->log_name: %s\n", ctx->log_name);

    unsigned long file_size = va_arg(ap, unsigned long);
    ctx->file_size          = file_size;
    DEBUG_LOG("ctx->file_size: %u\n", ctx->file_size);

    int num_files  = va_arg(ap, int);
    ctx->num_files = num_files;
    DEBUG_LOG("ctx->num_files: %d\n", ctx->num_files);

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
    .type = LOG_OUTTYPE_FILE,
    .type_name = "file",
    .emit = file_emit,
    .ctx_init = file_ctx_init,
    .ctx_uninit = file_ctx_uninit,
    .dump = file_ctx_dump
};
