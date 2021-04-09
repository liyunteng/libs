/*
 * simple_log.c - simple wrapper
 *
 * Date   : 2021/03/14
 */
#include "simple_log.h"

#include <stdlib.h>
#include <string.h>

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
    int level;
    char *dir;
    char *log_name;
} simple_log_ctx_t;

static simple_log_ctx_t g_simple_log_ctx = {
    NULL, {{NULL, NULL, NULL}, {NULL, NULL, NULL}}, -1, NULL, NULL};

static int
simple_log_start(int which)
{
    if (!g_simple_log_ctx.rules[which].fmt) {
        if (which == SIMPLE_LOG_STDOUT) {
            g_simple_log_ctx.rules[which].fmt =
                log_format_create("%d.%ms [%C%-5.5V%R] %m%n");
        } else if (which == SIMPLE_LOG_FILE) {
            g_simple_log_ctx.rules[which].fmt =
                log_format_create("%d.%ms [%-5.5V] %m%n");
        }
        if (!g_simple_log_ctx.rules[which].fmt) {
            return -1;
        }
    }


    if (!g_simple_log_ctx.rules[which].output) {
        if (which == SIMPLE_LOG_STDOUT) {
            g_simple_log_ctx.rules[which].output =
                log_output_create(LOG_OUTTYPE_STDOUT);
        } else if (which == SIMPLE_LOG_FILE) {
            g_simple_log_ctx.rules[which].output =
                log_output_create(LOG_OUTTYPE_FILE,
                                  g_simple_log_ctx.dir,
                                  g_simple_log_ctx.log_name,
                                  0,
                                  DEFAULT_LOG_FILE_SIZE,
                                  DEFAULT_LOG_BAKUP_NUM);
        }

        if (!g_simple_log_ctx.rules[which].output) {
            return -1;
        }
    }

    if (!g_simple_log_ctx.rules[which].rule) {
        g_simple_log_ctx.rules[which].rule = log_rule_create(
            g_simple_log_ctx.handler, g_simple_log_ctx.rules[which].fmt,
            g_simple_log_ctx.rules[which].output, g_simple_log_ctx.level, -1);
        if (!g_simple_log_ctx.rules[which].rule) {
            return -1;
        }
    }

    return 0;
}

static void
simple_log_stop(int which)
{
    if (g_simple_log_ctx.rules[which].rule) {
        log_rule_destroy(g_simple_log_ctx.rules[which].rule);
        g_simple_log_ctx.rules[which].rule = NULL;
    }
    if (g_simple_log_ctx.rules[which].output) {
        log_output_destroy(g_simple_log_ctx.rules[which].output);
        g_simple_log_ctx.rules[which].output = NULL;
    }
    if (g_simple_log_ctx.rules[which].fmt) {
        log_format_destroy(g_simple_log_ctx.rules[which].fmt);
        g_simple_log_ctx.rules[which].fmt = NULL;
    }
}

int
simple_log_init(const char *dir, const char *filename, int level)
{

    if (g_simple_log_ctx.handler) {
        simple_log_uninit();
    }

    g_simple_log_ctx.dir      = strdup(dir);
    g_simple_log_ctx.log_name = strdup(filename);
    g_simple_log_ctx.level    = level;
    g_simple_log_ctx.handler  = log_handler_create("default");
    if (!g_simple_log_ctx.handler) {
        goto failed;
    }
    simple_log_start(SIMPLE_LOG_STDOUT);
    simple_log_start(SIMPLE_LOG_FILE);

    log_handler_set_default(g_simple_log_ctx.handler);
    return 0;

failed:
    simple_log_uninit();
    return -1;
}

void
simple_log_uninit(void)
{
    simple_log_stop(SIMPLE_LOG_STDOUT);
    simple_log_stop(SIMPLE_LOG_FILE);


    if (g_simple_log_ctx.dir) {
        free(g_simple_log_ctx.dir);
        g_simple_log_ctx.dir = NULL;
    }

    if (g_simple_log_ctx.log_name) {
        free(g_simple_log_ctx.log_name);
        g_simple_log_ctx.log_name = NULL;
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
    g_simple_log_ctx.level = level;

    return 0;
}

void
simple_log_enable_stdout(int enable)
{
    if (enable) {
        simple_log_start(SIMPLE_LOG_STDOUT);
    } else {
        simple_log_stop(SIMPLE_LOG_STDOUT);
    }
}

void
simple_log_enable_file(int enable)
{
    if (enable) {
        simple_log_start(SIMPLE_LOG_FILE);
    } else {
        simple_log_stop(SIMPLE_LOG_FILE);
    }
}
