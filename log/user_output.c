/*
 * user_output.c - user_output
 *
 * Date   : 2021/03/25
 */
#include "user_output.h"

#include <errno.h>
#include <string.h>

struct user_output_ctx {
    log_user_callback cb;
    void *param;
};


static int
user_emit(struct log_output *output, struct log_handler *handler)
{
    log_buf_t *buf              = NULL;
    size_t len                  = 0;
    int write_len               = 0;
    struct user_output_ctx *ctx = NULL;

    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    ctx = (struct user_output_ctx *)output->ctx;
    if (!ctx) {
        ERROR_LOG("ctx is NULL\n");
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

    len = buf_len(buf);
    if (ctx->cb) {
        write_len = ctx->cb(handler->ident, handler->event.level, buf->start,
                            len, ctx->param);
        if (write_len != (int)len) {
            ERROR_LOG("user cb failed: %d != %lu\n", write_len, len);
        }
    }

    return write_len;
}

static void
user_ctx_dump(struct log_output *output)
{
    if (output) {
        DUMP_LOG("type: %s\n", output->priv->type_name);
        struct user_output_ctx *ctx = (struct user_output_ctx *)output->ctx;
        if (ctx) {
            DUMP_LOG("callback: %p\n", ctx->cb);
            DUMP_LOG("param: %p\n", ctx->param);
        }
        dump_statstic(output);
    }
}

static int
user_ctx_init(struct log_output *output, va_list ap)
{
    struct user_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (output->ctx == NULL) {
        output->ctx =
            (struct user_output_ctx *)calloc(1, sizeof(struct user_output_ctx));
        if (!output->ctx) {
            ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
            goto failed;
        }
    }

    ctx                  = (struct user_output_ctx *)output->ctx;
    log_user_callback cb = va_arg(ap, log_user_callback);
    ctx->cb              = cb;

    void *param = va_arg(ap, void *);
    ctx->param  = param;

    return 0;

failed:
    if (ctx != NULL) {
        free(ctx);
        ctx         = NULL;
        output->ctx = NULL;
    }

    return -1;
}

static void
user_ctx_uninit(struct log_output *output)
{
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return;
    }

    if (output->ctx) {
        free(output->ctx);
        output->ctx = NULL;
    }
    return;
}

struct log_output_priv user_output_priv = {
    .type       = LOG_OUTTYPE_USER,
    .type_name  = "user",
    .emit       = user_emit,
    .ctx_init   = user_ctx_init,
    .ctx_uninit = user_ctx_uninit,
    .dump       = user_ctx_dump,
};
