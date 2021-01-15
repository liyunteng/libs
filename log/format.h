/*
 * format.h - format
 *
 * Date   : 2021/01/15
 */
#ifndef FORMAT_H
#define FORMAT_H
#include "log_priv.h"
#include <stdint.h>

typedef struct log_formater log_formater_t;
typedef int (*formater)(const log_formater_t *f, char *buf, size_t len,
                        const log_handler_t *handler,
                        const LOG_LEVEL_E level,
                        const char *file,
                        const char *func,
                        const long line,
                        const char *fmt,
                        va_list ap);
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
