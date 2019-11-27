/*
 * avl.h -- avl
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/06/26 01:01:58
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

#ifndef __AVL_H__
#define __AVL_H__
#ifdef __cplusplus
extern "C" {
}
#endif

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    AVL_TREE_LT,
    AVL_TREE_EQ,
    AVL_TREE_GT,
    AVL_TREE_INVALID
} avl_tree_compare_code_td;

/*
 * edt: * avl_tree_shutdown_code_td
 *
 * AVL_FREE_OBJECTS
 * Free the objects when shutting down the tree.
 *
 * AVL_DONT_FREE_OBJECTS
 * Don't free the objects when shutting down the tree.
 */
typedef enum {
    AVL_FREE_OBJECTS,
    AVL_DONT_FREE_OBJECTS
} avl_tree_shutdown_code_td;

typedef enum {
    AVL_CONT_WALK,
    AVL_STOP_WALK,
} avl_tree_walk_code_td;

#define AVL_TREE_OPTION_DEFAULT 0x00000000
#define AVL_TREE_OPTION_NOT_THREADSAFE 0x00000001

typedef uint32_t avl_tree_options_td;

#define AVL_TREE_OPTION_EQ 0x00000001
#define AVL_TREE_OPTION_LT 0x00000002
#define AVL_TREE_OPTION_LE 0x00000003
#define AVL_TREE_OPTION_GT 0x00000004
#define AVL_TREE_OPTION_GE 0x00000005
#define AVL_TREE_OPTION_MIN 0x00000006
#define AVL_TREE_OPTION_MAX 0x00000007

typedef uint32_t avl_tree_search_options_td;
typedef struct avl_tree *avl_tree_h_td;

typedef void *(*avl_tree_malloc_fn)(size_t size);
typedef void (*avl_tree_free_fn)(void *mem_ptr);
typedef void (*avl_tree_cleanup_fn)(void *mem_ptr);
typedef int (*avl_tree_object_sanity_fn)(void *object);

typedef avl_tree_compare_code_td (*avl_tree_compare_fn)(void *obj_one,
                                                        void *obj_two);
typedef avl_tree_walk_code_td (*avl_tree_walk_fn)(void *object, void *ctx);

int avl_tree_init(avl_tree_compare_fn compare_fn, avl_tree_options_td options,
                  avl_tree_h_td *avl_tree);

int avl_tree_init_v2(avl_tree_compare_fn compare_fn,
                     avl_tree_malloc_fn malloc_fn, avl_tree_free_fn free_fn,
                     avl_tree_options_td options, avl_tree_h_td *avl_tree);
int avl_tree_init_fns(avl_tree_malloc_fn malloc_fn, avl_tree_free_fn free_fn,
                      avl_tree_cleanup_fn cleanup_fn,
                      avl_tree_object_sanity_fn snity_fn,
                      avl_tree_h_td avl_tree);

int avl_tree_allocate_object(avl_tree_h_td avl_tree, void **object,
                             uint32_t size);

int avl_tree_free_object(avl_tree_h_td avl_tree, void *object);

int avl_tree_insert(avl_tree_h_td avl_tree, void *object,
                    void **existing_object);

int avl_tree_delete(avl_tree_h_td avl_tree, void *object,
                    void **deleted_object);

uint32_t avl_tree_count(avl_tree_h_td avl_tree);

int avl_tree_search(avl_tree_h_td avl_tree, avl_tree_search_options_td options,
                    void *compare_object, void **found_object,
                    avl_tree_search_options_td *result);
int avl_tree_walk(avl_tree_h_td avl_tree, avl_tree_walk_fn walk_fn, void *ctx,
                  avl_tree_walk_code_td *result);

int avl_tree_walk_safe(avl_tree_h_td avl_tree, avl_tree_walk_fn walk_fn,
                       void *ctx, avl_tree_walk_code_td *result);

int avl_tree_shutdown(avl_tree_h_td *avl_tree,
                      avl_tree_shutdown_code_td shutdown_code);

void avl_tree_debug(avl_tree_h_td avl_tree);

#ifdef __cplusplus
}
#endif

#endif
