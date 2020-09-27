/*
 * test.c - test
 *
 * Date   : 2020/09/28
 */

#include "bsearch.h"
#include <stdio.h>


static int
cmp(const void *key, const void *elt)
{
    return *(int *)key - *(int *)elt;
}

int
main(void)
{
    int array[128];
    int i;
    for (i = 0; i < ARRAY_SIZE(array); i++) {
        array[i] = i;
    }
    int key = 1;
    int *ret = NULL;

    ret = bsearch(&key,  array, ARRAY_SIZE(array), sizeof(int), cmp);
    if (ret != NULL) {
        printf("found\n");
    } else {
        printf("not found\n");
    }

    return 0;
}
