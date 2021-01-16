/*
 * buf.c - buf
 *
 * Date   : 2021/01/16
 */
#include "buf.h"
#include "log_priv.h"
#include <errno.h>
#include <string.h>

#define LOG_INT32_LEN (sizeof("-2147483648") - 1)
#define LOG_INT64_LEN (sizeof("-9223372036854775808") - 1)
#define LOG_MAX_UINT32_VALUE ((uint32_t)0xffffffff)
#define LOG_MAX_INT32_VALUE ((uint32_t)0x7fffffff)

void
buf_destroy(log_buf_t *buf)
{
    if (buf->start) {
        free(buf->start);
    }
    free(buf);
}

log_buf_t *
buf_create(size_t size_min, size_t size_max)
{
    log_buf_t *buf = NULL;

    if (size_min == 0) {
        ERROR_LOG("size_min == 0\n");
        return NULL;
    }

    if (size_max != 0 && size_max < size_min) {
        ERROR_LOG("size_max[%lu] < size_max[%lu]\n", size_max, size_min);
        return NULL;
    }

    buf = calloc(1, sizeof(log_buf_t));
    if (!buf) {
        ERROR_LOG("calloc failed:(%s)\n", strerror(errno));
        return NULL;
    }

    strcpy(buf->truncate_str, "  (truncated)");
    buf->truncate_str_len = strlen(buf->truncate_str);

    buf->size_min  = size_min;
    buf->size_max  = size_max;
    buf->size_real = buf->size_min;

    buf->start = calloc(1, buf->size_real);
    if (!buf->start) {
        ERROR_LOG("calloc failed:(%s)\n", strerror(errno));
        goto failed;
    }

    buf->tail       = buf->start;
    buf->end_plus_1 = buf->start + buf->size_real;
    buf->end        = buf->end_plus_1 - 1;

    return buf;

failed:
    if (buf) {
        buf_destroy(buf);
    }
    return NULL;
}

static void
buf_truncate(log_buf_t *buf)
{
    char *p;
    size_t len;

    if ((buf->truncate_str)[0] == '\0')
        return;
    p = (buf->tail - buf->truncate_str_len);
    if (p < buf->start)
        p = buf->start;
    len = buf->tail - p;

    memcpy(p, buf->truncate_str, len);
    return;
}

/* return 0:	success
 * return <0:	fail, set size_real to -1;
 * return >0:	by conf limit, can't extend size
 * increment must > 0
 */
static int
buf_resize(log_buf_t *buf, size_t increment)
{
    int ret         = 0;
    size_t new_size = 0;
    size_t len      = 0;
    char *p         = NULL;

    if (buf->size_max != 0 && buf->size_real >= buf->size_max) {
        DEBUG_LOG("buf->size_real[%lu] >= buf->size_max[%lu]\n", buf->size_real,
                  buf->size_max);
        return 1;
    }

    if (buf->size_max == 0) {
        /* unlimit */
        new_size = buf->size_real + 1.5 * increment;
    } else {
        /* limited */
        if (buf->size_real + increment <= buf->size_max) {
            new_size = buf->size_real + increment;
        } else {
            new_size = buf->size_max;
            ret      = 1;
        }
    }

    len = buf->tail - buf->start;
    p   = realloc(buf->start, new_size);
    if (!p) {
        ERROR_LOG("realloc failed(%s)\n", strerror(errno));
        free(buf->start);
        buf->start      = NULL;
        buf->tail       = NULL;
        buf->end        = NULL;
        buf->end_plus_1 = NULL;
        return -1;
    } else {
        buf->start      = p;
        buf->tail       = p + len;
        buf->end_plus_1 = buf->start + new_size;
        buf->end        = buf->end_plus_1 - 1;
        buf->size_real  = new_size;
    }

    return ret;
}

int
buf_vprintf(log_buf_t *buf, const char *fmt, va_list args)
{
    va_list ap;
    size_t size_left;
    int nwrite;

    if (!buf->start) {
        ERROR_LOG("buf->start is NULL\n");
        return -1;
    }

    va_copy(ap, args);
    size_left = buf->end_plus_1 - buf->tail;
    nwrite    = vsnprintf(buf->tail, size_left, fmt, ap);
    if (nwrite >= 0 && nwrite < size_left) {
        buf->tail += nwrite;
        /* *(buf->tail) = '\0'; */
        return 0;
    } else if (nwrite < 0) {
        ERROR_LOG("vsnprintf failed(%s)\n", strerror(errno));
        return -1;
    } else if (nwrite >= size_left) {
        int ret;
        ret = buf_resize(buf, nwrite - size_left + 1);
        if (ret > 0) {
            DEBUG_LOG("resize limited, trucated\n");
            va_copy(ap, args);
            size_left = buf->end_plus_1 - buf->tail;
            vsnprintf(buf->tail, size_left, fmt, ap);
            buf->tail += size_left - 1;
            /* *(buf->tail) = '\0'; */
            buf_truncate(buf);
            return 1;
        } else if (ret < 0) {
            ERROR_LOG("buf_resize failed\n");
            return -1;
        } else {
            va_copy(ap, args);
            size_left = buf->end_plus_1 - buf->tail;
            nwrite    = vsnprintf(buf->tail, size_left, fmt, ap);
            if (nwrite < 0) {
                ERROR_LOG("vsnprintf failed(%s)\n", strerror(errno));
                return -1;
            } else {
                buf->tail += nwrite;
                /* *(buf->tail) = '\0'; */
                return 0;
            }
        }
    }
    return 0;
}

/* if width > num_len, 0 padding, else output num */
int
buf_printf_dec32(log_buf_t *buf, uint32_t ui32, int width)
{
    unsigned char *p;
    char *q;
    unsigned char tmp[LOG_INT32_LEN + 1] = {0};
    size_t num_len, zero_len, out_len;

    if (!buf->start) {
        ERROR_LOG("buf->start is NULL\n");
        return -1;
    }

    p = tmp + LOG_INT32_LEN;
    do {
        *--p = (unsigned char)(ui32 % 10 + '0');
    } while (ui32 /= 10);

    /* zero or space padding */
    num_len = (tmp + LOG_INT32_LEN) - p;

    if (width > num_len) {
        zero_len = width - num_len;
        out_len  = width;
    } else {
        zero_len = 0;
        out_len  = num_len;
    }

    if ((q = buf->tail + out_len) > buf->end) {
        int ret;
        ret = buf_resize(buf, out_len - (buf->end - buf->tail));
        if (ret > 0) {
            size_t len_left;
            DEBUG_LOG("resize limited, output\n");
            len_left = buf->end - buf->tail;
            if (len_left <= zero_len) {
                zero_len = len_left;
                num_len  = 0;
            } else if (len_left > zero_len) {
                /* zero_len not changed */
                num_len = len_left - zero_len;
            }

            if (zero_len)
                memset(buf->tail, '0', zero_len);
            memcpy(buf->tail + zero_len, p, num_len);
            /* *(buf->tail) = '\0'; */
            buf_truncate(buf);
            return 1;
        } else if (ret < 0) {
            ERROR_LOG("buf_resize failed\n");
            return -1;
        } else {
            q = buf->tail + out_len;
        }
    }

    if (zero_len)
        memset(buf->tail, '0', zero_len);
    memcpy(buf->tail + zero_len, p, num_len);
    buf->tail = q;
    /* *(buf->tail) = '\0'; */

    return 0;
}


int
buf_printf_dec64(log_buf_t *buf, uint64_t ui64, int width)
{
    unsigned char *p;
    char *q;
    unsigned char tmp[LOG_INT64_LEN + 1];
    size_t num_len, zero_len, out_len;
    uint32_t ui32;

    if (!buf->start) {
        ERROR_LOG("buf->start is NULL\n");
        return -1;
    }

    p = tmp + LOG_INT64_LEN;
    if (ui64 <= LOG_MAX_UINT32_VALUE) {
        /*
         * To divide 64-bit numbers and to find remainders
         * on the x86 platform gcc and icc call the libc functions
         * [u]divdi3() and [u]moddi3(), they call another function
         * in its turn.  On FreeBSD it is the qdivrem() function,
         * its source code is about 170 lines of the code.
         * The glibc counterpart is about 150 lines of the code.
         *
         * For 32-bit numbers and some divisors gcc and icc use
         * a inlined multiplication and shifts.  For example,
         * unsigned "i32 / 10" is compiled to
         *
         *     (i32 * 0xCCCCCCCD) >> 35
         */

        ui32 = (uint32_t)ui64;

        do {
            *--p = (unsigned char)(ui32 % 10 + '0');
        } while (ui32 /= 10);

    } else {
        do {
            *--p = (unsigned char)(ui64 % 10 + '0');
        } while (ui64 /= 10);
    }


    /* zero or space padding */
    num_len = (tmp + LOG_INT64_LEN) - p;

    if (width > num_len) {
        zero_len = width - num_len;
        out_len  = width;
    } else {
        zero_len = 0;
        out_len  = num_len;
    }

    if ((q = buf->tail + out_len) > buf->end) {
        int rc;
        rc = buf_resize(buf, out_len - (buf->end - buf->tail));
        if (rc > 0) {
            size_t len_left;
            DEBUG_LOG("resize limited, output\n");
            len_left = buf->end - buf->tail;
            if (len_left <= zero_len) {
                zero_len = len_left;
                num_len  = 0;
            } else if (len_left > zero_len) {
                /* zero_len not changed */
                num_len = len_left - zero_len;
            }
            if (zero_len)
                memset(buf->tail, '0', zero_len);
            memcpy(buf->tail + zero_len, p, num_len);
            buf->tail += len_left;
            //*(a_buf->tail) = '\0';
            buf_truncate(buf);
            return 1;
        } else if (rc < 0) {
            ERROR_LOG("buf_resize failed\n");
            return -1;
        } else {
            q = buf->tail + out_len; /* re-calculate p*/
        }
    }

    if (zero_len)
        memset(buf->tail, '0', zero_len);
    memcpy(buf->tail + zero_len, p, num_len);
    buf->tail = q;
    //*(a_buf->tail) = '\0';
    return 0;
}

int
buf_printf_hex(log_buf_t *buf, uint32_t ui32, int width)
{
    unsigned char *p;
    char *q;
    unsigned char tmp[LOG_INT32_LEN + 1];
    size_t num_len, zero_len, out_len;
    static unsigned char hex[] = "0123456789abcdef";
    // static unsigned char   HEX[] = "0123456789ABCDEF";

    if (!buf->start) {
        ERROR_LOG("buf->start is NULL\n");
        return -1;
    }

    p = tmp + LOG_INT32_LEN;
    do {
        /* the "(uint32_t)" cast disables the BCC's warning */
        *--p = hex[(uint32_t)(ui32 & 0xf)];
    } while (ui32 >>= 4);

    /* zero or space padding */
    num_len = (tmp + LOG_INT32_LEN) - p;

    if (width > num_len) {
        zero_len = width - num_len;
        out_len  = width;
    } else {
        zero_len = 0;
        out_len  = num_len;
    }

    if ((q = buf->tail + out_len) > buf->end) {
        int ret;
        ret = buf_resize(buf, out_len - (buf->end - buf->tail));
        if (ret > 0) {
            size_t len_left;
            DEBUG_LOG("resize limited, output\n");
            len_left = buf->end - buf->tail;
            if (len_left <= zero_len) {
                zero_len = len_left;
                num_len  = 0;
            } else if (len_left > zero_len) {
                /* zero_len not changed */
                num_len = len_left - zero_len;
            }

            if (zero_len)
                memset(buf->tail, '0', zero_len);
            memcpy(buf->tail + zero_len, p, num_len);
            buf->tail += len_left;
            /* *(buf->tail) = '\0'; */
            buf_truncate(buf);
            return 1;
        } else if (ret < 0) {
            ERROR_LOG("buf_resize failed\n");
            return -1;
        } else {
            q = buf->tail + out_len;
        }
    }

    if (zero_len)
        memset(buf->tail, '0', zero_len);
    memcpy(buf->tail + zero_len, p, num_len);
    buf->tail = q;
    /* *(buf->tail) = '\0'; */

    return 0;
}

int
buf_append(log_buf_t *buf, const char *str, size_t str_len)
{
    char *p;

    if (str_len <= 0 || str == NULL) {
        return 0;
    }

    if (!buf->start) {
        ERROR_LOG("buf->start is NULL\n");
        return -1;
    }

    if ((p = buf->tail + str_len) > buf->end) {
        int ret;
        ret = buf_resize(buf, str_len - (buf->end - buf->tail));
        if (ret > 0) {
            size_t len_left;
            DEBUG_LOG("resize limited, output\n");
            len_left = buf->end - buf->tail;
            memcpy(buf->tail, str, len_left);
            buf->tail += len_left;
            /* *(buf->tail) = '\0'; */
            buf_truncate(buf);
            return 1;
        } else if (ret < 0) {
            ERROR_LOG("buf_resize failed\n");
            return -1;
        } else {
            p = buf->tail + str_len;
        }
    }

    memcpy(buf->tail, str, str_len);
    buf->tail = p;
    /* *(buf->tail) = '\0'; */

    return 0;
}

int
buf_adjust_append(log_buf_t *buf, const char *str, size_t str_len,
                  int left_adjust, int zero_pad, size_t in_width,
                  size_t out_width)
{
    size_t append_len = 0;
    size_t source_len = 0;
    size_t space_len  = 0;

    if (str_len <= 0 || str == NULL) {
        return 0;
    }

    if (!buf->start) {
        ERROR_LOG("buf->start is NULL\n");
        return -1;
    }

    /* calculate how many character will be got from str */
    if (out_width == 0 || str_len < out_width) {
        source_len = str_len;
    } else {
        source_len = out_width;
    }

    /* calculate how many character will be output */
    if (in_width == 0 || source_len >= in_width) {
        append_len = source_len;
        space_len  = 0;
    } else {
        append_len = in_width;
        space_len  = in_width - source_len;
    }

    /*  |-----append_len-----------| */
    /*  |-source_len---|-space_len-|  left_adjust */
    /*  |-space_len---|-source_len-|  right_adjust */
    /*  |-(size_real-1)---|           size not enough */

    if (append_len > buf->end - buf->tail) {
        int ret = 0;
        ret     = buf_resize(buf, append_len - (buf->end - buf->tail));
        if (ret > 0) {
            ERROR_LOG("resize limited, output\n");
            append_len = (buf->end - buf->tail);
            if (left_adjust) {
                if (source_len < append_len) {
                    space_len = append_len - source_len;
                } else {
                    source_len = append_len;
                    space_len  = 0;
                }
                if (space_len)
                    memset(buf->tail + source_len, ' ', space_len);
                memcpy(buf->tail, str, source_len);
            } else {
                if (space_len < append_len) {
                    source_len = append_len - space_len;
                } else {
                    space_len  = append_len;
                    source_len = 0;
                }
                if (space_len) {
                    if (zero_pad) {
                        memset(buf->tail, '0', space_len);
                    } else {
                        memset(buf->tail, ' ', space_len);
                    }
                }
                memcpy(buf->tail + space_len, str, source_len);
            }
            buf->tail += append_len;
            /* *(buf->tail) = '\0'; */
            buf_truncate(buf);
            return 1;
        } else if (ret < 0) {
            ERROR_LOG("buf_resize failled\n");
            return -1;
        } else {
        }
    }

    if (left_adjust) {
        if (space_len)
            memset(buf->tail + source_len, ' ', space_len);
        memcpy(buf->tail, str, source_len);
    } else {
        if (space_len) {
            if (zero_pad) {
                memset(buf->tail, '0', space_len);
            } else {
                memset(buf->tail, ' ', space_len);
            }
        }
        memcpy(buf->tail + space_len, str, source_len);
    }
    buf->tail += append_len;
    /* (a_buf->tail) = '\0'; */
    return 0;
}
