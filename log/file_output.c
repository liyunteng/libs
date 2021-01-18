/*
 * file_output.c - file_output
 *
 * Date   : 2021/01/15
 */
#include "file_output.h"
#include "buf.h"
#include "log_priv.h"
#include <errno.h>
#include <string.h>

#define DEFAULT_FILEPATH "."
#define DEFAULT_FILENAME "test"
#define DEFAULT_BAKUP 0
#define DEFAULT_FILESIZE 4 * 1024 * 1024


/* pointer to environment */
extern char **environ;

/* dump the environment */
static void
dump_environment(log_output_t *output)
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


static int
file_getname(log_output_t *output, char *file_name, uint16_t len)
{
    file_output_ctx *ctx = (file_output_ctx *)output->ctx;
    snprintf(file_name, len, "%s/%s.log", ctx->file_path, ctx->log_name);
    return 0;
}

static int
file_open_logfile(log_output_t *output)
{
    char *file_name;
    uint32_t len;


    file_output_ctx *ctx = (file_output_ctx *)output->ctx;

    if (ctx->fp != NULL) {
        fclose(ctx->fp);
        ctx->fp = NULL;
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

    if ((ctx->fp = fopen(file_name, "w")) == NULL) {
        ERROR_LOG("fopen %s failed:(%s)\n", file_name, strerror(errno));
        free(file_name);
        return -1;
    }

    ctx->data_offset = 0;

    free(file_name);
    return 0;
}

static int
file_rename_logfile(log_output_t *output)
{
    uint32_t num, num_files, len;
    char *old_file_name, *new_file_name;
    file_output_ctx *ctx = (file_output_ctx *)output->ctx;

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
file_emit(log_output_t *output, log_handler_t *handler)
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
    int left = ctx->file_size - ctx->data_offset;
    if (left > len) {
        if (fwrite(buf->start, len, 1, ctx->fp) != 1) {
            ERROR_LOG("fwrite failed(%s) len:%lu\n", strerror(errno), len);
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
                    "fwrite failed(%s) nwrite: %lu total: %lu len: %lu left: "
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

        file_rename_logfile(output);
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
file_ctx_dump(log_output_t *output)
{
    if (output) {
        printf("type: %s\n", output->type_name);
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
file_ctx_uninit(log_output_t *output)
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
file_ctx_init(log_output_t *output, va_list ap)
{
    file_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (!output->ctx) {
        output->ctx = (file_output_ctx *)calloc(1, sizeof(file_output_ctx));
        if (!output->ctx) {
            ERROR_LOG("calloc failed(%s)\n", strerror(errno));
            goto failed;
        }
    }
    ctx = (file_output_ctx *)output->ctx;

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

    unsigned long file_size = va_arg(ap, unsigned long);
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


log_output_t *
file_output_create(void)
{
    log_output_t *output = NULL;
    output               = (log_output_t *)calloc(1, sizeof(log_output_t));
    if (!output) {
        ERROR_LOG("calloc failed(%s)\n", strerror(errno));
        return NULL;
    }

    output->type       = LOG_OUTTYPE_FILE;
    output->type_name  = "file";
    output->emit       = file_emit;
    output->ctx_init   = file_ctx_init;
    output->ctx_uninit = file_ctx_uninit;
    output->dump       = file_ctx_dump;

    return output;
}
