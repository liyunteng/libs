/*
 * buf.h - buf
 *
 * Date   : 2021/01/16
 */
#ifndef BUF_H
#define BUF_H

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    char *start;
    char *tail;
    char *end;
    char *end_plus_1;

    size_t size_min;
    size_t size_max;
    size_t size_real;

    char truncate_str[64];
    size_t truncate_str_len;
} log_buf_t;

log_buf_t *buf_create(size_t min, size_t max);
void buf_destroy(log_buf_t *buf);

int buf_vprintf(log_buf_t *buf, const char *fmt, va_list ap);
int buf_append(log_buf_t *buf, const char *str, size_t str_len);
int buf_adjust_append(log_buf_t *buf, const char *str, size_t str_len,
                      int left_adjust, int zero_pad, size_t in_width,
                      size_t out_width);
int buf_printf_dec32(log_buf_t *buf, uint32_t uii32, int width);
int buf_printf_dec64(log_buf_t *buf, uint64_t ui64, int width);
int buf_printf_hex(log_buf_t *buf, uint32_t ui32, int width);

#define buf_restart(buf)                                                       \
    do {                                                                       \
        buf->tail = buf->start;                                                \
    } while (0)

#define buf_len(buf) (buf->tail - buf->start)
#define buf_left(buf) (buf->end_plus_1 - buf->tail)
#define buf_str(buf) (buf->start)
#define buf_seal(buf)                                                          \
    do {                                                                       \
        *(buf)->tail = '\0';                                                   \
    } while (0)

#endif
