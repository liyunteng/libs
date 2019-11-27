/*
 * bst.h -- binary search tree
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/04/19 15:58:58
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

#ifndef BST_H
#define BST_H
#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include <stdlib.h>

typedef int datatype;
typedef struct _Node Node;
struct _Node {
    Node *parent;
    Node *lchild;
    Node *rchild;
    datatype data;
};

extern Node *bst_insert(Node **root, datatype data);
extern Node *bst_create(Node **root, datatype arr[], size_t len);
extern void bst_pre_iterate(Node *root);
extern void bst_in_iterate(Node *root);
extern void bst_post_iterate(Node *root);
extern Node *bst_min(Node *root);
extern Node *bst_max(Node *root);
extern Node *bst_search(Node *root, datatype data);
/**
 * bst_delete - delete the Node of data from root
 * @root:     the root of bst
 * @data:     the data to delete
 *
 * RETURN:
 * 0 delet success.
 * -1 data not found.
 * -2 internel error.
 */
extern int bst_delete(Node *root, datatype data);

extern void bst_destroy(Node **root);

#ifdef __cplusplus
}
#endif
#endif
