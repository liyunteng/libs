/*
 * log_priv.h - log_priv
 *
 * Date   : 2021/01/15
 */
#ifndef LOG_PRIV_H
#define LOG_PRIV_H

#include "log.h"
#include "list.h"
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>

#ifndef BOOL
#define BOOL int8_t
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define COLOR_NORMAL "\033[0;00m"
#define COLOR_EMERG "\033[5;7;31m"
#define COLOR_ALERT "\033[5;7;35m"
#define COLOR_FATAL "\033[5;7;33m"
#define COLOR_ERROR "\033[1;31m"
#define COLOR_WARNING "\033[1;35m"
#define COLOR_NOTICE "\033[1;34m"
#define COLOR_INFO "\033[1;37m"
#define COLOR_DEBUG "\033[0;32m"
#define COLOR_VERBOSE "\033[0;00m"

#define DEFAULT_SOCKADDR "127.0.0.1"
#define DEFAULT_SOCKPORT 12345
#define DEFAULT_FILEPATH "."
#define DEFAULT_FILENAME "test"
#define DEFAULT_BAKUP 0
#define DEFAULT_FILESIZE 4 * 1024 * 1024
#define DEFAULT_TIME_FORMAT "%F %T"
#define DEFAULT_FORMAT "%d.%ms %c:%p [%V] %F:%U(%L) %m%n"

#define BUFFER_MIN 1024 * 4
#define BUFFER_MAX 1024 * 1024 * 4

#define DEBUG_LOG(fmt, ...)
/* #define DEBUG_LOG(fmt, ...)                                                    \
 *     fprintf(stdout, "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__) */
#define ERROR_LOG(fmt, ...)                                                    \
    fprintf(stderr, "%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

typedef int(*formater)(char *buf, size_t len, char *fmt, ...);
typedef struct  {
    struct list_head formater_entry;
    formater formater;
    char *mode;
    char *key;
} log_formater_t;

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
