/*
 * test.c - test
 *
 * Date   : 2020/04/25
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "daemon.h"

void sig_handler (int signo)
{
    fprintf(stderr, "catch signal: %s(%d)\n", strsignal(signo), signo);
}

int main(void)
{
    csignals_to_catch(sig_handler);

    Daemon();

    while (1) {
        sleep(3);
        printf("this is a test\n");
    }
    return 0;
}
