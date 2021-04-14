/*
 * syslog_output.c - syslog_output
 *
 * Date   : 2021/01/17
 */
#include "syslog_output.h"

#include <errno.h>
#include <string.h>
#include <syslog.h>

#define DEFAULT_SYSLOG_IDENT "default"

struct syslog_output_ctx {
    char *ident;
    int options;
    int facility;
};

static void
syslog_ctx_dump(struct log_output *output)
{
    if (output) {
        DUMP_LOG("type: %s\n", output->priv->type_name);
        struct syslog_output_ctx *ctx = (struct syslog_output_ctx *)output->ctx;
        if (ctx) {
            DUMP_LOG("ident:    %s\n", ctx->ident);
            DUMP_LOG("options:  0x%x\n", ctx->options);
            DUMP_LOG("facility: 0x%x\n", ctx->facility);
        }
        dump_statstic(output);
    }
}

static int
syslog_ctx_init(struct log_output *output, va_list ap)
{
    struct syslog_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (output->ctx == NULL) {
        output->ctx = (struct syslog_output_ctx *)calloc(
            1, sizeof(struct syslog_output_ctx));
        if (!output->ctx) {
            ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
            return -1;
        }
    }

    ctx = (struct syslog_output_ctx *)output->ctx;

    char *ident = va_arg(ap, char *);
    ctx->ident  = strdup(ident);

    int options  = va_arg(ap, int);
    ctx->options = options;

    int facility  = va_arg(ap, int);
    ctx->facility = facility;

    openlog(ctx->ident, ctx->options, ctx->facility);

    return 0;
}

static void
syslog_ctx_uninit(struct log_output *output)
{
    struct syslog_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return;
    }

    ctx = (struct syslog_output_ctx *)output->ctx;
    if (!ctx) {
        return;
    }

    if (ctx->ident) {
        free(ctx->ident);
        ctx->ident = NULL;
    }

    closelog();
}

static int
syslog_emit(struct log_output *output, struct log_handler *handler)
{
    log_buf_t *buf = NULL;
    size_t len;
    int level;

    (void)output;
    if (!handler) {
        ERROR_LOG("handler is NULL\n");
        return -1;
    }
    buf = handler->event.msg_buf;
    if (!buf) {
        ERROR_LOG("msg_buf is NULL\n");
        return -1;
    }

    len   = buf_len(buf);
    level = handler->event.level;
    if (level > LOG_DEBUG) {
        level = LOG_DEBUG;
    }
    if (level < LOG_EMERG) {
        level = LOG_EMERG;
    }

    syslog(level, "%s", buf->start);
    return len;
}

struct log_output_priv syslog_output_priv = {
    .type       = LOG_OUTTYPE_SYSLOG,
    .type_name  = "syslog",
    .emit       = syslog_emit,
    .ctx_init   = syslog_ctx_init,
    .ctx_uninit = syslog_ctx_uninit,
    .dump       = syslog_ctx_dump,
};
