/*
 * proc.h - proc
 *
 * Date   : 2020/04/29
 */
#ifndef __PROC_H__
#define __PROC_H__

int wait_proc_exit(const char *procname, int timeout_sec);

int wait_procs_exit(const char **procs, int timeout_sec);

#endif
