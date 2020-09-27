/*
 * types.h -- types
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/06/05 17:33:06
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

#ifndef TYPES_H
#define TYPES_H

#ifndef NULL
#ifdef __cplusplus
#  if !defined(__MINGW32__) && !defined(_MSC_VER)
#    define NULL __null
#  else
#    define NULL 0
#  endif
#else
#  define NULL ((void*)0)
#endif
#endif

/* #ifndef NULL
 * #    if defined __GNUC__                                                       \
 *         && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8))
 * #        define NULL (__null)
 * #    else
 * #        if !defined(__cplusplus)
 * #            define NULL ((void *)0)
 * #        else
 * #            define NULL (0)
 * #        endif
 * #    endif
 * #endif */

#ifdef __CHECKER__
#    define __force __attribute__((force))
#else
#    define __force
#endif

#ifndef __always_inline
#    define __always_inline inline __attribute__((always_inline))
#endif

#if defined(__x86_64__) && !defined(__ILP32__)
#    define __BITS_PER_LONG 64
#else
#    define __BITS_PER_LONG 32
#endif

#define BITS_PER_LONG __BITS_PER_LONG
typedef _Bool bool;

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;
typedef unsigned long __u64;

#ifndef barrier
#    define barrier() __asm__ __volatile__("" ::: "memory")
#endif

#ifndef WRITE_ONCE
static __always_inline void
__write_once_size(volatile void *p, void *res, int size)
{
    switch (size) {
    case 1:
        *(volatile __u8 *)p = *(__u8 *)res;
        break;
    case 2:
        *(volatile __u16 *)p = *(__u16 *)res;
        break;
    case 4:
        *(volatile __u32 *)p = *(__u32 *)res;
        break;
    case 8:
        *(volatile __u64 *)p = *(__u64 *)res;
        break;
    default:
        barrier();
        __builtin_memcpy((void *)p, (const void *)res, size);
        barrier();
    }
}

#    define WRITE_ONCE(x, val)                                                 \
        ({                                                                     \
            union {                                                            \
                typeof(x) __val;                                               \
                char __c[1];                                                   \
            } __u = {.__val = (__force typeof(x))(val)};                       \
            __write_once_size(&(x), __u.__c, sizeof(x));                       \
            __u.__val;                                                         \
        })
#endif

#ifndef READ_ONCE
#    define __READ_ONCE_SIZE                                                   \
        ({                                                                     \
            switch (size) {                                                    \
            case 1:                                                            \
                *(__u8 *)res = *(volatile __u8 *)p;                            \
                break;                                                         \
            case 2:                                                            \
                *(__u16 *)res = *(volatile __u16 *)p;                          \
                break;                                                         \
            case 4:                                                            \
                *(__u32 *)res = *(volatile __u32 *)p;                          \
                break;                                                         \
            case 8:                                                            \
                *(__u64 *)res = *(volatile __u64 *)p;                          \
                break;                                                         \
            default:                                                           \
                barrier();                                                     \
                __builtin_memcpy((void *)res, (const void *)p, size);          \
                barrier();                                                     \
            }                                                                  \
        })

static __always_inline void
__read_once_size(const volatile void *p, void *res, int size)
{
    __READ_ONCE_SIZE;
}

static __always_inline void
__read_once_size_nocheck(const volatile void *p, void *res, int size)
{
    __READ_ONCE_SIZE;
}

#    define __READ_ONCE(x, check)                                              \
        ({                                                                     \
            union {                                                            \
                typeof(x) __val;                                               \
                char __c[1];                                                   \
            } __u;                                                             \
            if (check)                                                         \
                __read_once_size(&(x), __u.__c, sizeof(x));                    \
            else                                                               \
                __read_once_size_nocheck(&(x), __u.__c, sizeof(x));            \
            __u.__val;                                                         \
        })
#    define READ_ONCE(x) __READ_ONCE(x, 1)
#endif

#ifndef offsetof
#    ifdef __compiler_offsetof
#        define offsetof(TYPE, MEMBER) __compiler_offset(TYPE, MEMBER)
#    else
#        define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
#    endif
#endif

#ifndef container_of
#    define container_of(ptr, type, member)                                    \
        ({                                                                     \
            const typeof(((type *)0)->member) *__mptr = (ptr);                 \
            (type *)((char *)__mptr - offsetof(type, member));                 \
        })
#endif

#ifndef ARRAY_SIZE
#    define ARRAY_SIZE(name) (size_t)((sizeof(name)) / (sizeof(name[0])))
#endif

#ifndef min
#    define min(x, y)                                                          \
        ({                                                                     \
            typeof(x) _min1 = (x);                                             \
            typeof(y) _min2 = (y);                                             \
            (void)(&_min1 == &_min2);                                          \
            _min1 < _min2 ? _min1 : _min2;                                     \
        })
#endif

#ifndef max
#    define max(x, y)                                                          \
        ({                                                                     \
            typeof(x) _max1 = (x);                                             \
            typeof(y) _max2 = (y);                                             \
            (void)(&_max1 == &_max2);                                          \
            _max1 > _max2 ? _max1 : _max2;                                     \
        })
#endif

#ifndef TRUE
#    define TRUE 1
#endif

#ifndef FALSE
#    define FALSE 0
#endif

#endif
