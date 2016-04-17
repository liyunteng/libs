/*
 * types.h -- types
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/04/17 20:31:00
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
# if defined __GNUG__ &&						\
	(__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8))
#  define NULL (__null)
# else
#  if !defined(__cplusplus)
#   define NULL ((void*)0)
#  else
#   define NULL (0)
#  endif
# endif
#endif

#ifdef __CHECKER__
# define __force		 __attribute__((force))
#else
# define __force
#endif

#ifndef __always_inline
#define __always_inline	inline __attribute__((always_inline))
#endif


#define BITS_PER_LONG 64
typedef _Bool bool;

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

typedef unsigned char  __u8;
typedef unsigned short __u16;
typedef unsigned int   __u32;
typedef unsigned long  __u64;

#define barrier()	 __asm__ __volatile__("":::"memory")
static __always_inline void __write_once_size(volatile void *p, void *res, int size)
{
	switch (size) {
	case 1: *(volatile __u8 *)p = *(__u8 *)res; break;
	case 2: *(volatile __u16 *)p = *(__u16 *)res; break;
	case 4: *(volatile __u32 *)p = *(__u32 *)res; break;
	case 8: *(volatile __u64 *)p = *(__u64 *)res; break;
	default:
		barrier();
		__builtin_memcpy((void *)p, (const void *)res, size);
		barrier();
	}
}

#define WRITE_ONCE(x, val)                                              \
	({                                                              \
		union { typeof(x) __val; char __c[1]; } __u =           \
								{ .__val = (__force typeof(x)) (val) };	\
		__write_once_size(&(x), __u.__c, sizeof(x));            \
		__u.__val;                                              \
	})

#define __READ_ONCE_SIZE						\
	({								\
		switch (size) {						\
		case 1: *(__u8 *)res = *(volatile __u8 *)p; break;	\
		case 2: *(__u16 *)res = *(volatile __u16 *)p; break;	\
		case 4: *(__u32 *)res = *(volatile __u32 *)p; break;	\
		case 8: *(__u64 *)res = *(volatile __u64 *)p; break;	\
		default:						\
			barrier();					\
			__builtin_memcpy((void *)res, (const void *)p, size); \
			barrier();					\
		}							\
	})

static __always_inline
void __read_once_size(const volatile void *p, void *res, int size)
{
	__READ_ONCE_SIZE;
}

static __always_inline
void __read_once_size_nocheck(const volatile void *p, void *res, int size)
{
	__READ_ONCE_SIZE;
}

#define __READ_ONCE(x, check)						\
	({								\
		union { typeof(x) __val; char __c[1]; } __u;		\
		if (check)						\
			__read_once_size(&(x), __u.__c, sizeof(x));	\
		else							\
			__read_once_size_nocheck(&(x), __u.__c, sizeof(x)); \
		__u.__val;						\
	})
#define READ_ONCE(x) __READ_ONCE(x, 1)


#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({				\
			const typeof( ((type *)0)->member ) *__mptr = (ptr); \
			(type *)( (char *)__mptr - offsetof(type, member) );})
#endif
