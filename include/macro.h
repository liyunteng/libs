/*
 * macro.h - macro
 *
 * Date   : 2021/03/16
 */
#ifndef __MACRO_H__
#define __MACRO_H__

// TODO(XXXXX)
// FIXME(AAAAAAAAA)
#ifndef TODO
//#pragma GCC warning "ABC"
#define __STRINGFY1__(x) #x
#define __STRINGFY__(x) __STRINGFY1__(x)
#define __PRAGMA_MESSAGE__(x) _Pragma(__STRINGFY1__(message(x)))
/* #define __PRAGMA_MESSAGE__(x) _Pragma(__STRINGFY1__(GCC warning x)) */
/* #define __PRAGMA_MESSAGE__(x) _Pragma(__STRINGFY1__(GCC error x)) */

#if _MSC_VER
#warning _MCS_VER
#define TODO(msg) __PRAGMA_MESSAGE__(__STRINGFY1__(TODO: msg))
#define FIXME(msg) __PRAGMA_MESSAGE__(__STRINGFY1__(FIXME : msg))
#elif __GNUC__
#define TODO(msg) __PRAGMA_MESSAGE__(__STRINGFY__(TODO: msg))
#define FIXME(msg) __PRAGMA_MESSAGE__(__STRINGFY__(FIXME: msg))
#else
#define TODO(msg)
#define FIXME(msg)
#endif
#endif


// DLL_EXPORT_API / DLL_IMPORT_API
#ifndef DLL_EXPORT_API
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define DLL_EXPORT_API __declspec(dllexport)
    #define DLL_IMPORT_API __declspec(dllimport)
#else
    #if __GNUC__ >= 4
        #define DLL_EXPORT_API __attribute__((visibility("default")))
        #define DLL_IMPORT_API
    #else
        #define DLL_EXPORT_API
        #define DLL_IMPORT_API
    #endif
#endif
#endif


// deprecated
#ifndef DEPRECATED_API
#if defined(_MSC_VER)
#define DEPRECATED_API __declspec(deprecated)
#elif __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC__MMINOR__ >= 1)
#define DEPRECATED_API __attribute__((deprecated))
#else
#define DEPRECATED_API
#endif
#endif

// inline
#ifndef inline
#if !defined(__cplusplus)
    #if _MSC_VER && _MSC_VER < 1900  // VS2015
        #define inline __inline
    #elif __GNUC__ && !__GNUC_STDC_INLINE__ && !__GNUC_GNUC_INLINE__
        #define inline static __inline__  // __attribute__((unused))
        //#define inline static __inlin__ __attribute__((always_inline))
    #else
        //#define inline static inline
    #endif
#endif
#endif


// NULL
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



// barrier
#ifndef barrier
#define barrier() __asm__ __volatile__("" ::: "memory")
#endif


// offsetof
#ifndef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE, MEMBER) __compiler_offset(TYPE, MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
#endif
#endif

// container_of
#ifndef container_of
#define container_of(ptr, type, member)                                        \
    ({                                                                         \
        const typeof(((type *)0)->member) *__mptr = (ptr);                     \
        (type *)((char *)__mptr - offsetof(type, member));                     \
    })
#endif

// ARRAY_SIZE
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(name) (size_t)((sizeof(name)) / (sizeof(name[0])))
#endif

// MIN
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

// MAX
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

// FLOOR(num/demo): FLOOR(34/10) = 3
#ifndef FLOOR
#define FLOOR(num, deno) ( (num)/ (deno) )
#endif

// CEIL(num/demo): CEIL(34/10) = CEIL((34 + 10 - 1) / 10) = 4
#ifndef CEIL
#define CEIL(num, deno) ( ((num) + (deno) - 1) / (deno) )
#endif

// BOUND(value, 2, 20) => [2 ~ 20]
#ifndef BOUND
#define BOUND(v, min, max) MIN(MAX(v, min), max)
#endif

// ALIGN
#ifndef ALIGN
#define ALIGN(v, a) (((v)+(a)-1)&~((a)-1))
#endif

// bool
#ifndef bool
#if defined(_Bool)
#define bool _Bool
#else
#define bool unsigned char
#endif
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef true
#define true 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef false
#define false 0
#endif

#endif
