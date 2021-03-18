/*
 * test.c - test
 *
 * Date   : 2021/03/18
 */

#include "bst.h"
#include "macro.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

int bst_test(void)
{
    int i;
    Node *root = NULL;
    int ret    = 0;

#if 1
    datatype a[] = {2, 2, 1, 0, 7, 4, 3, 3, 6, 8, 5, 9};
#else
    datatype a[4096];
    srandom(time(NULL));
    for (i = 0; i < (int)ARRAY_SIZE(a); i++) {
        a[i] = random() % 100;
    }
#endif

    bst_create(&root, a, ARRAY_SIZE(a));
    printf("datas:\n");
    for (i = 0; i < (int)ARRAY_SIZE(a); i++) {
        printf("%d ", a[i]);
    }

    ret = bst_delete(root, 0);
    assert(ret == 0);
    ret = bst_delete(root, 9);
    assert(ret == 0);
    ret = bst_delete(root, 3);
    assert(ret == 0);
    ret = bst_delete(root, 2);
    assert(ret == 0);

    printf("\npre:\n");
    bst_pre_iterate(root);
    printf("\nin:\n");
    bst_in_iterate(root);
    printf("\npost:\n");
    bst_post_iterate(root);

    Node *s = bst_search(root, 6);
    if (s) {
        printf("\nfind\n");
    } else {
        printf("\nnot find\n");
    }

    printf("min: %d, max: %d\n", bst_min(root)->data, bst_max(root)->data);

    bst_destroy(&root);
    if (root == NULL) {
        printf("NULL\n");
    }

    return 0;
}

int
main(void)
{

    bst_test();
    return 0;
}
