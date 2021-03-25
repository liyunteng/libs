/*
 * syslog_output.c - syslog_output
 *
 * Date   : 2021/01/17
 */
#include "syslog_output.h"
#include "log.h"

#include <errno.h>
#include <string.h>
#include <syslog.h>

#define DEFAULT_SYSLOG_IDENT "default"

typedef struct {
    char *ident;
    int options;
    int facility;
} syslog_output_ctx;


static void
syslog_ctx_dump(struct log_output *output)
{
    if (output) {
        printf("type: %s\n", output->priv->type_name);
        syslog_output_ctx *ctx = (syslog_output_ctx *)output->ctx;
        if (ctx) {
            printf("ident:    %s\n", ctx->ident);
            printf("options:  0x%x\n", ctx->options);
            printf("facility: 0x%x\n", ctx->facility);
        }
        dump_statstic(output);
    }
}

static int
syslog_ctx_init(struct log_output *output, va_list ap)
{
    syslog_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return -1;
    }

    if (output->ctx == NULL) {
        output->ctx = (syslog_output_ctx *)calloc(1, sizeof(syslog_output_ctx));
        if (!output->ctx) {
            ERROR_LOG("calloc failed: (%s)\n", strerror(errno));
            return -1;
        }
    }

    ctx = (syslog_output_ctx *)output->ctx;

    char *ident = va_arg(ap, char *);
    if (ident && strlen(ident) > 0) {
        ctx->ident = strdup(ident);
    } else {
        ctx->ident = strdup(DEFAULT_SYSLOG_IDENT);
    }
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
    syslog_output_ctx *ctx = NULL;
    if (!output) {
        ERROR_LOG("output is NULL\n");
        return;
    }

    ctx = (syslog_output_ctx *)output->ctx;
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
