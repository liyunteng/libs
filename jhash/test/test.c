/*
 * test.c - test
 *
 * Date   : 2021/03/19
 */
#include "jhash.h"
#include "macro.h"
#include <string.h>
#include <stdio.h>

int
jhash_test(void)
{
    int i;
    uint32_t r;
    const char *keys[] = {
        "",
        "abc",
        "0123456789",
        "0123456789abcdef",
        "abcdefghijklmnopqrstuvwxyz",
    };

    for (i = 0; i < ARRAY_SIZE(keys); i++) {
        r = jhash(keys[i], strlen(keys[i]), 0);
        printf("0x%x\n", r);
    }

    r = jhash_3words(0x01, 0x02, 0x03, 0);
    printf("0x%x\n", r);

    r = jhash_2words(0x01, 0x02, 0);
    printf("0x%x\n", r);

    r = jhash_1word(0x01, 0);
    printf("0x%x\n", r);

    return 0;
}

int main(void)
{
    jhash_test();
    return 0;
}
