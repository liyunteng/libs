/*
 * deprecated.h - deprecated
 *
 * Date   : 2021/03/16
 */
#ifndef __DEPRECATED_H__
#define __DEPRECATED_H__

#if defined(_MSC_VER)
#define deprecated __declspec(deprecated)
#elif __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC__MMINOR__ >= 1)
#define deprecated __attribute__((deprecated))
#else
#define deprecated
#endif


#endif
