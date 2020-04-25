/*
 * backtrace.c - backtrace
 *
 * Date   : 2020/04/26
 */
#include <stdio.h>
#include <execinfo.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define BT_MAX_LEVEL 128

int
backtrace_symbolic (char *buf, int bufsize)
{
    int j, nptrs;
    char **strings;
    void *buffer[BT_MAX_LEVEL];
    int nleft = bufsize;
    int n;

    if (!buf || bufsize <= 30) {
        return -1;
    }
    memset(buf, 0, bufsize);
    nptrs = backtrace(buffer, BT_MAX_LEVEL);
    n = snprintf(buf + (bufsize - nleft), nleft, "backtrace() return %d address:\n", nptrs);
    nleft -= n;

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        perror("backtrace_symbols");
        return -1;
    }
    for (j = 0; j < nptrs; j++) {
        if (nleft < (int)strlen(strings[j]) + 8) {
            break;
        }
        n = snprintf(buf + (bufsize - nleft), nleft, "  [%02d] %s\n", nptrs - j - 1, strings[j]);
        nleft -= n;
    }

    free(strings);
    buf[bufsize - nleft - 1] = '\0';
    return j;
}
