/*
 * log_priv.h - log_priv
 *
 * Date   : 2021/01/15
 */
#ifndef LOG_PRIV_H
#define LOG_PRIV_H

#include "list.h"
#include "log.h"
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
/* #define DEBUG_LOG(fmt, ...) \ fprintf(stdout, "%s:%d " fmt, __FILE__,
 * __LINE__, ##__VA_ARGS__) */
#define ERROR_LOG(fmt, ...)                                                    \
    fprintf(stderr, "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

struct log_format {
    char format[128];
    BOOL color;
    struct list_head format_entry;
    struct list_head callbacks;
};

typedef int (*log_output_ctx_init_fn)(log_output_t *output, va_list ap);
typedef void (*log_output_ctx_uninit_fn)(log_output_t *output);
typedef int (*log_output_emit_fn)(log_output_t *output, LOG_LEVEL_E level,
                                  char *buf, size_t len);
typedef void (*log_output_dump_fn)(log_output_t *output);
struct log_output {
    enum LOG_OUTTYPE type;
    char *type_name;
    struct list_head output_entry;
    struct {
        struct {
            uint32_t count;
            uint64_t bytes;
        } stats[LOG_VERBOSE + 1];
        uint64_t count_total;
        uint64_t bytes_total;
    } stat;
    void *ctx;
    log_output_ctx_init_fn ctx_init;
    log_output_ctx_uninit_fn ctx_uninit;
    log_output_emit_fn emit;
    log_output_dump_fn dump;
};

typedef struct {
    LOG_LEVEL_E level_begin;
    LOG_LEVEL_E level_end;
    log_output_t *output;
    log_format_t *format;
    struct list_head rule_entry;
    struct list_head rule;
} log_rule_t;

struct log_handler {
    pthread_mutex_t mutex;
    char ident[128];

    char *bufferp;
    size_t buffer_max;
    size_t buffer_min;
    size_t buffer_real;

    struct list_head rules;  // rules
    struct list_head handler_entry;
};


void dump_statstic(log_output_t *output);
#endif
