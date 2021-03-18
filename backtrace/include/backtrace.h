/*
 * backtrace.h - backtrace
 *
 * Date   : 2020/04/26
 */
#ifndef __BACKTRACE_H__
#define __BACKTRACE_H__

/* add link option -rdynamic */
int backtrace_symbolic (char *buf, int bufsize);

#endif
