/*
 * test.c - test
 *
 * Date   : 2021/03/14
 */
#include "uuid.h"
#include <stdio.h>

int uuid_test(void)
{
    int i = 0;
    char uuid[37] = {0};
    for (i = 0; i < 1000; i++) {
        uuid_generate_simple(uuid);
        printf("%d: %s\n", i, uuid);
    }
    return 0;
}

int main(void)
{
    uuid_test();
    return 0;
}
