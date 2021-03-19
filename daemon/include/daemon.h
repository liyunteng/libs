/*
 * daemo.h - daemo
 *
 * Date   : 2020/04/25
 */
#ifndef __DAEMON_H__
#define __DAEMON_H__

#include <signal.h>
#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
typedef void (*__sighandler_t)(int);
#endif

int csignals_to_catch(__sighandler_t signal_handler);

void daemon_sem_wait(sem_t *sem);

int Daemon(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
