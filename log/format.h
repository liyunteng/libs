/*
 * format.h - format
 *
 * Date   : 2021/01/15
 */
#ifndef FORMAT_H
#define FORMAT_H
#include "log_priv.h"
#include <stdint.h>

typedef struct {
    const char *file;
    const char *func;
    const char *fmt;
    va_list ap;
    LOG_LEVEL_E level;
    log_handler_t *handler;
    long line;
} log_argument_t;

typedef struct log_formater log_formater_t;
typedef int (*formater)(log_formater_t *f, log_argument_t*s, char *buf, size_t len);


struct log_formater {
    struct list_head formater_entry;
    formater formater;
    char *mode;
    char *key;

    union {
        uint32_t u32;
        uint64_t u64;
        char *str;
        uint32_t hex;
        char c;
    } u;
};

int format_parse(log_format_t *fmt);
#endif
