/*
 * inline.h - inline
 *
 * Date   : 2021/03/16
 */
#ifndef __INLINE_H__
#define __INLINE_H__

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
