/*
 * test_bst.c -- bst test
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/04/20 08:20:30
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bst.h"

int
main(void)
{
    struct bst_node *root  = NULL;
    datatype         arr[] = {2, 1, 7, 4, 3, 6, 5, 8};

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
