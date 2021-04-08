/*
 * simple_log.c - simple wrapper
 *
 * Date   : 2021/03/14
 */
#include "simple_log.h"

#include <stdlib.h>

#define DEFAULT_LOG_FILE_SIZE 4 * 1024 * 1024
#define DEFAULT_LOG_BAKUP_NUM 4

enum { SIMPLE_LOG_STDOUT, SIMPLE_LOG_FILE, SIMPLE_LOG_MAX };

typedef struct {
    log_format_t *fmt;
    log_output_t *output;
    log_rule_t *rule;
} simple_log_rule_t;

typedef struct {
    log_handler_t *handler;
    simple_log_rule_t rules[SIMPLE_LOG_MAX];
} simple_log_ctx_t;

static simple_log_ctx_t g_simple_log_ctx = {
    NULL, {{NULL, NULL, NULL}, {NULL, NULL, NULL}}};

int
simple_log_init(const char *dir, const char *filename, int level)
{

    if (g_simple_log_ctx.handler) {
        simple_log_uninit();
    }

    g_simple_log_ctx.handler = log_handler_create("default");
    if (!g_simple_log_ctx.handler) {
        goto failed;
    }

    g_simple_log_ctx.rules[SIMPLE_LOG_STDOUT].fmt =
        log_format_create("%d.%ms [%C%-5.5V%R] %m%n");
    if (!g_simple_log_ctx.rules[SIMPLE_LOG_STDOUT].fmt) {
        goto failed;
    }
    g_simple_log_ctx.rules[SIMPLE_LOG_STDOUT].output =
        log_output_create(LOG_OUTTYPE_STDOUT);
    if (!g_simple_log_ctx.rules[SIMPLE_LOG_STDOUT].output) {
        goto failed;
    }
    g_simple_log_ctx.rules[SIMPLE_LOG_STDOUT].rule = log_rule_create(
        g_simple_log_ctx.handler, g_simple_log_ctx.rules[SIMPLE_LOG_STDOUT].fmt,
        g_simple_log_ctx.rules[SIMPLE_LOG_STDOUT].output, level, -1);
    if (!g_simple_log_ctx.rules[SIMPLE_LOG_STDOUT].rule) {
        goto failed;
    }

    g_simple_log_ctx.rules[SIMPLE_LOG_FILE].fmt =
        log_format_create("%d.%ms [%-5.5V] %m%n");
    if (!g_simple_log_ctx.rules[SIMPLE_LOG_FILE].fmt) {
        goto failed;
    }
    g_simple_log_ctx.rules[SIMPLE_LOG_FILE].output =
        log_output_create(LOG_OUTTYPE_FILE,
                          dir,
                          filename,
                          0,
                          DEFAULT_LOG_FILE_SIZE,
                          DEFAULT_LOG_BAKUP_NUM);
    if (!g_simple_log_ctx.rules[SIMPLE_LOG_FILE].output) {
        goto failed;
    }
    g_simple_log_ctx.rules[SIMPLE_LOG_FILE].rule = log_rule_create(
        g_simple_log_ctx.handler, g_simple_log_ctx.rules[SIMPLE_LOG_FILE].fmt,
        g_simple_log_ctx.rules[SIMPLE_LOG_FILE].output, level, -1);
    if (!g_simple_log_ctx.rules[SIMPLE_LOG_FILE].rule) {
        goto failed;
    }

    log_handler_set_default(g_simple_log_ctx.handler);
    return 0;

failed:
    simple_log_uninit();
    return -1;
}

void
simple_log_uninit(void)
{
    int i;
    for (i = 0; i < SIMPLE_LOG_MAX; i++) {
        if (g_simple_log_ctx.rules[i].rule) {
            log_rule_destroy(g_simple_log_ctx.rules[i].rule);
            g_simple_log_ctx.rules[i].rule = NULL;
        }
        if (g_simple_log_ctx.rules[i].output) {
            log_output_destroy(g_simple_log_ctx.rules[i].output);
            g_simple_log_ctx.rules[i].output = NULL;
        }
        if (g_simple_log_ctx.rules[i].fmt) {
            log_format_destroy(g_simple_log_ctx.rules[i].fmt);
            g_simple_log_ctx.rules[i].fmt = NULL;
        }
    }

    if (g_simple_log_ctx.handler) {
        log_handler_destroy(g_simple_log_ctx.handler);
        g_simple_log_ctx.handler = NULL;
    }
}

int
simple_log_set_level(int level)
{
    int i;
    if (!g_simple_log_ctx.handler) {
        return -1;
    }

    for (i = 0; i < SIMPLE_LOG_MAX; i++) {
        if (g_simple_log_ctx.rules[i].rule && g_simple_log_ctx.rules[i].fmt
            && g_simple_log_ctx.rules[i].output) {
            log_rule_set_level(g_simple_log_ctx.rules[i].rule, level, -1);
        }
    }

    return 0;
}
