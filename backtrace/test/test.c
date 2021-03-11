/*
 * test.c - test
 *
 * Date   : 2020/04/26
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include "backtrace.h"

void catch_signal(int signo)
{
    char buf[10240];
    int n;
    n = backtrace_symbolic(buf, sizeof(buf));
    printf("catch signal: (%d)%s\n", signo, strsignal(signo));

    printf("%d:%d\n", n, (int)strlen(buf));
    printf("%s\n", buf);
    exit(-1);
}

int
myfunc3(int n)
{
    printf("%s\n", __PRETTY_FUNCTION__);

    int i = 2;
    i = i / (n - 1);
    return i;
}

static int   /* "static" means don't export the symbol... */
myfunc2(int n)
{
    printf("%s\n", __PRETTY_FUNCTION__);

    int r = 0;
    r = myfunc3(n);
    printf("%d\n", r);
    return r;
}

void
myfunc(int n)
{
    printf("%s\n", __PRETTY_FUNCTION__);

    if (n > 1)
        myfunc(n - 1);
    else
        myfunc2(n);
}

int main(void)
{
    signal(SIGILL, catch_signal);
    signal(SIGFPE, catch_signal);
    myfunc(2);

    return 0;
}
