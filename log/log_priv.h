/*
 * log_priv.h - log_priv
 *
 * Date   : 2021/01/15
 */
#ifndef __LOG_PRIV_H__
#define __LOG_PRIV_H__

#include "log_buf.h"
#include "log_format.h"
#include "log.h"
#include "list.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef BOOL
#define BOOL int8_t
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DEBUG_LOG(fmt, ...)
// #define DEBUG_LOG(fmt, ...)                                          \
//    fprintf(stdout, "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define ERROR_LOG(fmt, ...)                                             \
    fprintf(stderr, "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)


enum LOG_OPTS {
    LOG_OPT_SET_HANDLER_IDENT, /* log_handler_t *handler, char *ident */
    LOG_OPT_GET_HANDLER_IDENT, /* log_handler_t *handler, char *ident */
};

struct log_format {
    char format[128];
    struct list_head format_entry;
    struct list_head callbacks;
};

typedef int (*log_output_ctx_init_fn)(struct log_output *output, va_list ap);
typedef void (*log_output_ctx_uninit_fn)(struct log_output *output);
typedef int (*log_output_emit_fn)(struct log_output *output,
                                  struct log_handler *handler);
typedef void (*log_output_dump_fn)(struct log_output *output);

struct log_output_priv {
    enum LOG_OUTTYPE type;
    char *type_name;
    log_output_ctx_init_fn ctx_init;
    log_output_ctx_uninit_fn ctx_uninit;
    log_output_emit_fn emit;
    log_output_dump_fn dump;
};

struct log_output {
    void *ctx;
    struct log_output_priv *priv;

    struct list_head output_entry;
    struct {
        struct {
            uint32_t count;
            uint64_t bytes;
        } stats[LOG_VERBOSE + 1];
        uint64_t count_total;
        uint64_t bytes_total;
    } stat;
};

struct log_rule {
    int level_begin;
    int level_end;
    struct log_output *output;
    struct log_format *format;
    struct list_head rule_entry;
    struct list_head rule;
};

struct log_handler {
    pthread_mutex_t mutex;
    char ident[128];

    struct log_event event;

    struct list_head rules;  // rules
    struct list_head handler_entry;
};


void dump_statstic(struct log_output *output);
#endif
