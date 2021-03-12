/*
 * daemo.h - daemo
 *
 * Date   : 2020/04/25
 */
#ifndef DAEMON_H
#define DAEMON_H

#include <signal.h>
#include <semaphore.h>

#ifdef __APPLE__
typedef void (*__sighandler_t)(int);
#endif

int csignals_to_catch(__sighandler_t signal_handler);

void daemon_sem_wait(sem_t *sem);

int Daemon(void);
#endif
