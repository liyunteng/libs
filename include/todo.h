/*
 * todo.h - todo
 *
 * Date   : 2021/03/16
 */
#ifndef __TODO_H__
#define __TODO_H__

// TODO(XXXXX)
// FIXME(AAAAAAAAA)

//#pragma GCC warning "ABC"
#define __STRINGFY1__(x) #x
#define __STRINGFY__(x) __STRINGFY1__(x)
#define __PRAGMA_MESSAGE__(x) _Pragma(__STRINGFY1__(message(x)))
/* #define __PRAGMA_MESSAGE__(x) _Pragma(__STRINGFY1__(GCC warning x)) */
/* #define __PRAGMA_MESSAGE__(x) _Pragma(__STRINGFY1__(GCC error x)) */

#if _MSC_VER
#define TODO(msg) __PRAGMA_MESSAGE__(__STRINGFY1__(TODO: msg))
#define FIXME(msg) __PRAGMA_MESSAGE__(__STRINGFY1__(FIXME: msg))
#elif __GNUC__
#define TODO(msg) __PRAGMA_MESSAGE__(__STRINGFY__(TODO: msg))
#define FIXME(msg) __PRAGMA_MESSAGE__(__STRINGFY__(FIXME: msg))
#else
#define TODO(msg) TODO msg
#define FIXME(msg) FIXME msg
#endif


#endif
