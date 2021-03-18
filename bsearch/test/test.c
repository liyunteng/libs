/*
 * test.c - test
 *
 * Date   : 2020/09/28
 */

#include "bsearch.h"
#include "macro.h"
#include <stdio.h>


static int
cmp(const void *key, const void *elt)
{
    return *(int *)key - *(int *)elt;
}

int bsearch_test(void)
{
    int array[128];
    int i;
    for (i = 0; i < ARRAY_SIZE(array); i++) {
        array[i] = i;
    }
    int keys[] = {1, 64, 27, 126, 128};
    int *ret   = NULL;

    for (i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        ret = bsearch(&keys[i], array, ARRAY_SIZE(array), sizeof(int), cmp);
        if (ret != NULL) {
            printf("%d found\n", keys[i]);
        } else {
            printf("%d not found\n", keys[i]);
        }
    }

    return 0;
}

int
main(void)
{
    bsearch_test();
    return 0;
}
