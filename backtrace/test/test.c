/*
 * test.c - test
 *
 * Date   : 2020/04/26
 */

#include <stdio.h>
#include <string.h>
#include "backtrace.h"

void
myfunc3()
{
    char buf[102400];
    int n;
    n = backtrace_symbolic(buf, sizeof(buf));
    printf("%d:%d\n", n, (int)strlen(buf));
    printf("%s\n", buf);
}

static void   /* "static" means don't export the symbol... */
myfunc2(void)
{
    myfunc3();
}

void
myfunc(int ncalls)
{
    if (ncalls > 1)
        myfunc(ncalls - 1);
    else
        myfunc2();
}

int main(void)
{
    myfunc(200);
    return 0;
}
