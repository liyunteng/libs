/*
 * bst_test.c -- bst test
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/04/19 10:44:53
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#include "bst.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

int
main(void)
{

    /* datatype a[] = {2, 2, 1, 0, 7, 4, 3, 3, 6, 8, 5, 9}; */
    int i;
    Node *root = NULL;
    int ret    = 0;
    datatype a[4096];
    srandom(time(NULL));
    for (i = 0; i < (int)ARRAY_SIZE(a); i++) {
        a[i] = random() % 100;
    }

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
