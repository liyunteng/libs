/*
 * csignal.c - csignal
 *
 * Date   : 2020/04/25
 */
#include "daemon.h"
#include <signal.h>
#include <stdio.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

int
csignals_to_catch(__sighandler_t signal_handler)
{
    int sig;
    for (sig = 1; sig <= 64; sig++) {
        /* these signals should not be catched */
        if (sig == SIGKILL || sig == SIGSEGV || sig == SIGSTOP ||
            sig == SIGPROF || sig == 32 || sig == 33 || sig == 64) {
            continue;
        }

        /* don't catch SIGBUS and SIGFPE, just produce coredump */
        if (sig == SIGBUS || sig == SIGFPE) {
            continue;
        }

        /* ignore these signals */
        if (sig == SIGWINCH) {
            if (signal(sig, SIG_IGN) == SIG_ERR) {
                return (-1);
            }
            continue;
        }

        if (signal(sig, signal_handler) == SIG_ERR) {
            /* printf("signo=%d, %s\n", sig, strsignal(sig)); */
            return(-1);
        }
    }

    return (0);

    /* setup SIGABRT handler */
    if (signal(SIGABRT, signal_handler) == SIG_ERR) {
        return -1;
    }

    /* setup SIGINT handler */
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        return -1;
    }

    /* setup SIGSEGV handler */
    if (signal(SIGSEGV, signal_handler) == SIG_ERR) {
        return -1;
    }

    /* setup SIGTERM handler */
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        return -1;
    }

    /* setup SIGHUP handler */
    if (signal(SIGHUP, signal_handler) == SIG_ERR) {
        return -1;
    }

    /* ignore SIGCHLD handler */
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        return -1;
    }

    return 0;
}
