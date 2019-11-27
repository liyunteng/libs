/*
 * bst.h --
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/04/20 06:44:01
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
#include "types.h"

typedef int datatype;
struct bst_node {
    struct bst_node *left;
    struct bst_node *right;
    datatype data;
};

extern struct bst_node *bst_insert(struct bst_node **root, datatype data);
extern struct bst_node *bst_create(struct bst_node **root, datatype a[],
                                   int len);
extern void bst_pre_iterate(struct bst_node *root);
extern void bst_in_iterate(struct bst_node *root);
extern void bst_post_iterate(struct bst_node *root);
extern struct bst_node *bst_min(struct bst_node *root);
extern struct bst_node *bst_max(struct bst_node *root);
extern struct bst_node *bst_search(struct bst_node *root, datatype key);
extern int bst_delete(struct bst_node **root, datatype key);

#endif
