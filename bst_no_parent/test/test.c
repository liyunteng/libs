/*
 * test.c - test
 *
 * Date   : 2021/03/18
 */
#include "bst.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int bst_test(void)
{
    struct bst_node *root = NULL;
    datatype arr[]        = {2, 1, 7, 4, 3, 6, 5, 8};

    /*
     * datatype arr[4096];
     * srandom(time(NULL));
     * for (int i = 0; i < (int)ARRAY_SIZE(arr); i++) {
     *	arr[i] = random() % 100;
     * }
     */
    root = bst_create(&root, arr, ARRAY_SIZE(arr));
    for (int i = 0; i < (int)ARRAY_SIZE(arr); i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
    int ret = 0;
    ret     = bst_delete(&root, 4);
    assert(ret == 0);
    ret = bst_delete(&root, 5);
    assert(ret == 0);
    ret = bst_delete(&root, 7);
    assert(ret == 0);
    ret = bst_delete(&root, 8);
    assert(ret == 0);

    printf("pre:\n");
    bst_pre_iterate(root);
    /*
     * printf("\nin:\n");
     * bst_in_iterate(root);
     * printf("\npost:\n");
     * bst_post_iterate(root);
     */

    printf("\nmin: %d, max: %d\n", bst_min(root)->data, bst_max(root)->data);

    struct bst_node *n = bst_search(root, 5);
    if (n) {
        printf("%d found.\n", n->data);
    } else {
        printf("not found.\n");
    }

    return 0;
}

int
main(void)
{
    bst_test();
    return 0;
}
