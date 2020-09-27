/*
 * argv_split.c - argv_split from kernel
 *
 * Date   : 2020/09/28
 */
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static int count_argc(const char *str)
{
    int count = 0;
    bool was_space;

    for (was_space = true; *str; str++) {
        if (isspace(*str)) {
            was_space = true;
        } else if (was_space) {
            was_space = false;
            count++;
        }
    }

    return count;
}

char **argv_split(const char *str, int *argcp)
{
    char *argv_str;
    bool was_space;
    char **argv, **argv_ret;
    int argc;

    argv_str = strdup(str);
    if (!argv_str) {
        return NULL;
    }

    argc = count_argc(argv_str);
    argv = (char **)(malloc((argc+2) * sizeof(*argv)));
    if (!argv) {
        return NULL;
    }

    *argv = argv_str;
    argv_ret = ++argv;
    for (was_space = true; *argv_str; argv_str++) {
        if (isspace(*argv_str)) {
            was_space = true;
            *argv_str = 0;
        } else if (was_space) {
            was_space = false;
            *argv++ = argv_str;
        }
    }
    *argv = NULL;

    if (argcp)
        *argcp = argc;
    return argv_ret;
}
