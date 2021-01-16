/*
 * format.h - format
 *
 * Date   : 2021/01/15
 */
#ifndef FORMAT_H
#define FORMAT_H
#include "log_priv.h"
#include <stdint.h>


typedef struct log_spec log_spec_t;
typedef int (*write_buf)(log_spec_t *f, log_event_t *e, char *buf, size_t len);
struct log_spec {
    struct list_head spec_entry;

    write_buf write_buf;
    char *str;
    int len;
    char time_fmt[64];


    size_t min_width;
    size_t max_width;
    int left_fill_zeros;
    int left_adjust;
    char print_fmt[64];
};

void event_update(log_event_t *e, log_handler_t *handler, log_rule_t *rule,
                  LOG_LEVEL_E level, const char *file, const char *func,
                  long line, const char *fmt, va_list ap);

log_spec_t * spec_create(char *pstart, char **pnext);
#endif
