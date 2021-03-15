/*
 * log_simple.c - log simple
 *
 * Date   : 2021/03/14
 */
#include "log.h"
#include <stdlib.h>

#define DEFAULT_LOG_FILE_SIZE 4 * 1024 * 1024
#define DEFAULT_LOG_BAKUP_NUM 4
#define DEFAULT_LOG_DIR "logs"

typedef struct {
    log_format_t *fmt;
    log_output_t *output[2];
    log_rule_t *rule[2];
    log_handler_t *handler;
    int enable_stdout;
} log_ctx_t;

static log_ctx_t g_log_ctx = {NULL, {NULL, NULL}, {NULL, NULL}, NULL, 0};

int
log_simple_init(const char *ident, int level)
{

    if (g_log_ctx.fmt || g_log_ctx.output[0] || g_log_ctx.output[1]
        || g_log_ctx.handler) {
        log_simple_uninit();
    }

    g_log_ctx.fmt = log_format_create("%d.%ms [%5.5V] %m%n");
    if (!g_log_ctx.fmt) {
        goto failed;
    }

    g_log_ctx.output[0] = log_output_create(LOG_OUTTYPE_FILE,
                                            DEFAULT_LOG_DIR,
                                            ident,
                                            DEFAULT_LOG_FILE_SIZE,
                                            DEFAULT_LOG_BAKUP_NUM);
    if (!g_log_ctx.output[0]) {
        goto failed;
    }

    g_log_ctx.enable_stdout = 0;
    if (g_log_ctx.enable_stdout) {
        g_log_ctx.output[1] = log_output_create(LOG_OUTTYPE_STDOUT);
        if (!g_log_ctx.output[1]) {
            goto failed;
        }
    }

    g_log_ctx.handler = log_handler_create(ident);
    if (!g_log_ctx.handler) {
        goto failed;
    }

    g_log_ctx.rule[0] = log_bind(g_log_ctx.handler, level, -1, g_log_ctx.fmt,
                                 g_log_ctx.output[0]);
    if (!g_log_ctx.rule[0]) {
        goto failed;
    }

    if (g_log_ctx.enable_stdout) {
        g_log_ctx.rule[1] = log_bind(g_log_ctx.handler, level, -1,
                                     g_log_ctx.fmt, g_log_ctx.output[1]);
        if (!g_log_ctx.rule[1]) {
            goto failed;
        }
    }

    log_handler_set_default(g_log_ctx.handler);
    return 0;

failed:
    log_simple_uninit();
    return -1;
}

void
log_simple_uninit(void)
{
    if (g_log_ctx.fmt) {
        log_format_destroy(g_log_ctx.fmt);
        g_log_ctx.fmt = NULL;
    }
    if (g_log_ctx.output[0]) {
        log_output_destroy(g_log_ctx.output[0]);
        g_log_ctx.output[0] = NULL;
    }
    if (g_log_ctx.output[1]) {
        log_output_destroy(g_log_ctx.output[1]);
        g_log_ctx.output[1] = NULL;
    }
    if (g_log_ctx.handler) {
        log_handler_destroy(g_log_ctx.handler);
        g_log_ctx.handler = NULL;
    }
    g_log_ctx.rule[0]       = NULL;
    g_log_ctx.rule[1]       = NULL;
    g_log_ctx.enable_stdout = 0;
}

int
log_simple_set_level(int level)
{
    if (!g_log_ctx.handler || !g_log_ctx.rule[0]) {
        return -1;
    }

    log_set_level(g_log_ctx.handler, g_log_ctx.rule[0], level, -1);
    if (g_log_ctx.enable_stdout && g_log_ctx.rule[1]) {
        log_set_level(g_log_ctx.handler, g_log_ctx.rule[1], level, -1);
    }
    return 0;
}
