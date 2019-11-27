/*
 * log2.h -- Interger base 2 logarithm calculation
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/04/17 17:15:53
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef LOG2_H
#define LOG2_H
#include "types.h"

/*
 * deal with unrepresentable constant logarithms
 */
extern __attribute__((const, noreturn)) int ____ilog2_NaN(void);

static __always_inline int
fls(int x)
{
    int r = 32;

    if (!x)
        return 0;
    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

static __always_inline unsigned long
__fls(unsigned long word)
{
    int num = BITS_PER_LONG - 1;

#if BITS_PER_LONG == 64
    if (!(word & (~0ul << 32))) {
        num -= 32;
        word <<= 32;
    }
#endif
    if (!(word & (~0ul << (BITS_PER_LONG - 16)))) {
        num -= 16;
        word <<= 16;
    }
    if (!(word & (~0ul << (BITS_PER_LONG - 8)))) {
        num -= 8;
        word <<= 8;
    }
    if (!(word & (~0ul << (BITS_PER_LONG - 4)))) {
        num -= 4;
        word <<= 4;
    }
    if (!(word & (~0ul << (BITS_PER_LONG - 2)))) {
        num -= 2;
        word <<= 2;
    }
    if (!(word & (~0ul << (BITS_PER_LONG - 1))))
        num -= 1;
    return num;
}

#if BITS_PER_LONG == 32
static __always_inline int
fls64(u64 x)
{
    u32 h = x >> 32;
    if (h)
        return fls(h) + 32;
    return fls(x);
}
#elif BITS_PER_LONG == 64
static __always_inline int
fls64(u64 x)
{
    if (x == 0)
        return 0;
    return __fls(x) + 1;
}
#endif

static inline __attribute__((const)) int
__ilog2_u32(u32 n)
{
    return fls(n) - 1;
}
static inline __attribute__((const)) int
__ilog2_u64(u64 n)
{
    return fls64(n) - 1;
}

/**
 * ilog2 - log of base 2 of 32-bit or a 64-bit unsigned value
 * @n - parameter
 *
 * constant-capable log of base 2 calculation
 * - this can be used to initialise global variables from constant data, hence
 *   the massive ternary operator construction
 *
 * selects the appropriately-sized optimised version depending on sizeof(n)
 */
#define ilog2(n)                                                               \
    (__builtin_constant_p(n) ?                                                 \
         ((n) < 1 ?                                                            \
              ____ilog2_NaN() :                                                \
              (n) & (1ULL << 63) ?                                             \
              63 :                                                             \
              (n) & (1ULL << 62) ?                                             \
              62 :                                                             \
              (n) & (1ULL << 61) ?                                             \
              61 :                                                             \
              (n) & (1ULL << 60) ?                                             \
              60 :                                                             \
              (n) & (1ULL << 59) ?                                             \
              59 :                                                             \
              (n) & (1ULL << 58) ?                                             \
              58 :                                                             \
              (n) & (1ULL << 57) ?                                             \
              57 :                                                             \
              (n) & (1ULL << 56) ?                                             \
              56 :                                                             \
              (n) & (1ULL << 55) ?                                             \
              55 :                                                             \
              (n) & (1ULL << 54) ?                                             \
              54 :                                                             \
              (n) & (1ULL << 53) ?                                             \
              53 :                                                             \
              (n) & (1ULL << 52) ?                                             \
              52 :                                                             \
              (n) & (1ULL << 51) ?                                             \
              51 :                                                             \
              (n) & (1ULL << 50) ?                                             \
              50 :                                                             \
              (n) & (1ULL << 49) ?                                             \
              49 :                                                             \
              (n) & (1ULL << 48) ?                                             \
              48 :                                                             \
              (n) & (1ULL << 47) ?                                             \
              47 :                                                             \
              (n) & (1ULL << 46) ?                                             \
              46 :                                                             \
              (n) & (1ULL << 45) ?                                             \
              45 :                                                             \
              (n) & (1ULL << 44) ?                                             \
              44 :                                                             \
              (n) & (1ULL << 43) ?                                             \
              43 :                                                             \
              (n) & (1ULL << 42) ?                                             \
              42 :                                                             \
              (n) & (1ULL << 41) ?                                             \
              41 :                                                             \
              (n) & (1ULL << 40) ?                                             \
              40 :                                                             \
              (n) & (1ULL << 39) ?                                             \
              39 :                                                             \
              (n) & (1ULL << 38) ?                                             \
              38 :                                                             \
              (n) & (1ULL << 37) ?                                             \
              37 :                                                             \
              (n) & (1ULL << 36) ?                                             \
              36 :                                                             \
              (n) & (1ULL << 35) ?                                             \
              35 :                                                             \
              (n) & (1ULL << 34) ?                                             \
              34 :                                                             \
              (n) & (1ULL << 33) ?                                             \
              33 :                                                             \
              (n) & (1ULL << 32) ?                                             \
              32 :                                                             \
              (n) & (1ULL << 31) ?                                             \
              31 :                                                             \
              (n) & (1ULL << 30) ?                                             \
              30 :                                                             \
              (n) & (1ULL << 29) ?                                             \
              29 :                                                             \
              (n) & (1ULL << 28) ?                                             \
              28 :                                                             \
              (n) & (1ULL << 27) ?                                             \
              27 :                                                             \
              (n) & (1ULL << 26) ?                                             \
              26 :                                                             \
              (n) & (1ULL << 25) ?                                             \
              25 :                                                             \
              (n) & (1ULL << 24) ?                                             \
              24 :                                                             \
              (n) & (1ULL << 23) ?                                             \
              23 :                                                             \
              (n) & (1ULL << 22) ?                                             \
              22 :                                                             \
              (n) & (1ULL << 21) ?                                             \
              21 :                                                             \
              (n) & (1ULL << 20) ?                                             \
              20 :                                                             \
              (n) & (1ULL << 19) ?                                             \
              19 :                                                             \
              (n) & (1ULL << 18) ?                                             \
              18 :                                                             \
              (n) & (1ULL << 17) ?                                             \
              17 :                                                             \
              (n) & (1ULL << 16) ?                                             \
              16 :                                                             \
              (n) & (1ULL << 15) ?                                             \
              15 :                                                             \
              (n) & (1ULL << 14) ?                                             \
              14 :                                                             \
              (n) & (1ULL << 13) ?                                             \
              13 :                                                             \
              (n) & (1ULL << 12) ?                                             \
              12 :                                                             \
              (n) & (1ULL << 11) ?                                             \
              11 :                                                             \
              (n) & (1ULL << 10) ?                                             \
              10 :                                                             \
              (n) & (1ULL << 9) ?                                              \
              9 :                                                              \
              (n) & (1ULL << 8) ?                                              \
              8 :                                                              \
              (n) & (1ULL << 7) ?                                              \
              7 :                                                              \
              (n) & (1ULL << 6) ?                                              \
              6 :                                                              \
              (n) & (1ULL << 5) ?                                              \
              5 :                                                              \
              (n) & (1ULL << 4) ?                                              \
              4 :                                                              \
              (n) & (1ULL << 3) ?                                              \
              3 :                                                              \
              (n) & (1ULL << 2) ?                                              \
              2 :                                                              \
              (n) & (1ULL << 1) ? 1 :                                          \
                                  (n) & (1ULL << 0) ? 0 : ____ilog2_NaN()) :   \
         (sizeof(n) <= 4) ? __ilog2_u32(n) : __ilog2_u64(n))

#endif
