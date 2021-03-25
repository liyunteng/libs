/*
 * user_output.c - user_output
 *
 * Date   : 2021/03/25
 */
#include "user_output.h"
#include <errno.h>
#include <string.h>

static int
user_emit(struct log_output *output, struct log_handler *handler)
{
    user_output_ctx *ctx = NULL;
    log_buf_t *buf       = NULL;
    size_t len;
    int write_len;

    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    ctx = (user_output_ctx *)output->ctx;
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
        if (write_len != len) {
            ERROR_LOG("user cb failed: %d != %lu\n", write_len, len);
        }
    }

    return write_len;
}

static void
user_ctx_dump(struct log_output *output)
{
    if (output) {
        printf("type: %s\n", output->type_name);
        user_output_ctx *ctx = (user_output_ctx *)output->ctx;
        if (ctx) {
            printf("callback: %p\n", ctx->cb);
            printf("param: %p\n", ctx->param);
        }
        dump_statstic(output);
    }
}

static int
user_ctx_init(struct log_output *output, va_list ap)
{
    user_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (output->ctx == NULL) {
        output->ctx = (user_output_ctx *)calloc(1, sizeof(user_output_ctx));
        if (!output->ctx) {
            ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
            goto failed;
        }
    }

    ctx = (user_output_ctx *)output->ctx;
    log_user_callback cb = va_arg(ap, log_user_callback);
    if (cb) {
        ctx->cb = cb;
    }

    void *param = va_arg(ap, void *);
    if (param) {
        ctx->param = param;
    }

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

struct log_output *
user_output_create(void)
{
    struct log_output *output = NULL;
    output = (struct log_output *)calloc(1, sizeof(struct log_output));
    if (!output) {
        ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
        return NULL;
    }

    output->type       = LOG_OUTTYPE_USER;
    output->type_name  = "user";
    output->emit       = user_emit;
    output->ctx_init   = user_ctx_init;
    output->ctx_uninit = user_ctx_uninit;
    output->dump       = user_ctx_dump;

    return output;
}
