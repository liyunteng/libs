/*
 * proc.h - proc
 *
 * Date   : 2020/04/29
 */
#ifndef __PROC_H__
#define __PROC_H__

#ifndef bool
#define bool int
#endif // bool

#ifndef true
#define true 1
#endif // true

#ifndef false
#define false 0
#endif // false

bool wait_proc_exit(const char *procname, int timeout_sec);

bool wait_procs_exit(const char **procs, int timeout_sec);

#endif
