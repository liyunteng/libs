/*
 * test.c - test
 *
 * Date   : 2020/09/28
 */

#include "argv_split.h"
#include <stdio.h>


int main(void)
{
    const char *str = "-a -b -c -d --ef --gh xyz eee";
    int i = 0;

    char **argv = NULL;
    int argc = 0;
    printf("%s\n", str);
    argv = argv_split(str, &argc);
    if (argv) {
        for (i = 0; i < argc; i++) {
            printf("[%d] %s\n", i, argv[i]);
        }
    }
    return 0;
}
