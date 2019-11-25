/*
 * avl.c -- avl
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/06/26 01:04:33
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
#include <inttypes.h>
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "avl.h"
#include "avl_internal.h"

#define DEBUG(fmt, args...)
#define ERR(fmt, args...)

#define SUCCESS 0
#define default_malloc_fn malloc
#define default_free_fn free

void
default_cleanup_fn(void *mem_ptr)
{
    (void)mem_ptr;
    return;
}

int
default_sanity_fn(void *obj)
{
    (void)obj;
    return 0;
}

/*
 *  edt: * * struct avl_tree
 *   Structure used to hold info about an avl tree.
 *
 *  Element: root
 *   Pointer to the root node of the tree.
 *
 *  Element: free_fn
 *   Memory free function for the avl tree to call
 *
 *  Element: compare_fn
 *   Compare function for the avl tree to call
 *
 *  Element: sanity_fn
 *   Object sanity function for the avl tree to call
 *
 *  Element: mutex
 *   Variable specifying the tree mutex. This is set by default.
 *
 *  Element: node_cnt
 *   Variable containing the number nodes in the tree.
 */
struct avl_tree {
    avl_tree_node_type_td *   root;
    avl_tree_malloc_fn        malloc_fn;
    avl_tree_free_fn          free_fn;
    avl_tree_cleanup_fn       cleanup_fn;
    avl_tree_compare_fn       compare_fn;
    avl_tree_object_sanity_fn sanity_fn;
    pthread_mutex_t           mutex;
    avl_tree_options_td       options;
    uint32_t                  node_cnt;
};

/*
 * edt: * * avl_tree_compare_internal
 *
 * This is the comparison function used by the AVL tree for inserting and
 * deleting nodes.
 *
 * Argument: node1
 * IN - this specifies the first node to compare
 *
 * Argument: node2
 * IN - this specifies the second node to compare
 *
 * Argument: ctx
 * IN - this specifies the context passed by the client
 *
 */
static avl_tree_compare_code_td
avl_tree_compare_internal(avl_tree_node_type_td *node1, avl_tree_node_type_td *node2, void *ctx)
{
    avl_tree_h_td local_ctx = (avl_tree_h_td)ctx;

    return local_ctx->compare_fn(node1->object->data, node2->object->data);
}

/*
 * edt: * * avl_tree_walk_internal
 *
 * This is the walk function used by the AVL tree for walking over all the nodes
 * and calling clients walk_fn for each node
 *
 * Argument: node
 * IN - this specifies the node which is being walked over.
 *
 * Argument: internal_ctx
 * IN - this specifies internal context
 *
 * Argument: ctx
 * IN - this specifies the context passed by the client
 *
 */
static avl_tree_walk_code_td
avl_tree_walk_internal(avl_tree_node_type_td *node, void *internal_ctx, void *ctx)
{
    avl_tree_walk_fn walk_fn = internal_ctx;

    return ((walk_fn)(node->object->data, ctx));
}

/*
 * edt: * * avl_tree_lock
 *
 * This is the function to lock the tree.
 *
 * Argument: mutex
 * IN - this specifies the mutex which is to be locked.
 *
 */
int
avl_tree_lock(pthread_mutex_t *mutex, avl_tree_options_td options)
{
    int rc = 0;

    if ((options & AVL_TREE_OPTION_NOT_THREADSAFE) != AVL_TREE_OPTION_NOT_THREADSAFE) {
        if (pthread_mutex_lock(mutex) != 0) {
            rc = -1;
        }
    }

    return rc;
}

/*
 * edt: * * avl_tree_unlock
 *
 * This is the function to unlock the tree.
 *
 * Argument: mutex
 * IN - this specifies the mutex which is to be unlocked.
 *
 */
int
avl_tree_unlock(pthread_mutex_t *mutex, avl_tree_options_td options)
{
    int rc = 0;

    if ((options & AVL_TREE_OPTION_NOT_THREADSAFE) != AVL_TREE_OPTION_NOT_THREADSAFE) {
        if (pthread_mutex_unlock(mutex) != 0) {
            rc = -1;
        }
    }

    return rc;
}

/****************************************/
/* implementation of avl.h functions    */
/****************************************/

int
avl_tree_init(avl_tree_compare_fn compare_fn, avl_tree_options_td options, avl_tree_h_td *avl_tree)
{
    int  rc           = 0;
    bool malloced_mem = FALSE;

    DEBUG("%s: INIT request - options:%d", __FUNCTION__, options);

    if (compare_fn == NULL || avl_tree == NULL) {
        rc = -1;
    }

    if (rc == 0) {
        /*
         * Allocate memory for the avl tree structure.
         */
        *avl_tree = malloc(sizeof(struct avl_tree));
        if (*avl_tree == NULL) {
            rc = -1;
        } else {
            malloced_mem = TRUE;
        }
    }

    if (rc == 0) {
        /*
         * Fill in the values in the avl tree struct
         */
        (*avl_tree)->malloc_fn  = default_malloc_fn;
        (*avl_tree)->free_fn    = default_free_fn;
        (*avl_tree)->cleanup_fn = default_cleanup_fn;
        (*avl_tree)->compare_fn = compare_fn;
        (*avl_tree)->sanity_fn  = default_sanity_fn;
        (*avl_tree)->root       = NULL;
        (*avl_tree)->options    = options;
        (*avl_tree)->node_cnt   = 0;
        if ((options & AVL_TREE_OPTION_NOT_THREADSAFE) != AVL_TREE_OPTION_NOT_THREADSAFE) {
            if (pthread_mutex_init(&((*avl_tree)->mutex), NULL) != 0) {
                rc = -1;
            }
        }
    }

    if (rc != 0) {
        /*
         * Free up memory
         */
        if (malloced_mem) {
            free(*avl_tree);
            *avl_tree = NULL;
        }
        ERR("%s: INIT request - FAIL retcode: %d", __FUNCTION__, rc);
    } else {
        DEBUG("%s: INIT request - SUCCESS tree handle: %p", __FUNCTION__, *avl_tree);
    }

    return rc;
}

int
avl_tree_init_v2(avl_tree_compare_fn compare_fn, avl_tree_malloc_fn malloc_fn,
                 avl_tree_free_fn free_fn, avl_tree_options_td options, avl_tree_h_td *avl_tree)
{
    int  rc           = 0;
    bool malloced_mem = FALSE;

    DEBUG("%s: INIT request - options:%d", __FUNCTION__, options);

    if (compare_fn == NULL || malloc_fn == NULL || free_fn == NULL || avl_tree == NULL) {
        rc = -1;
    }

    if (rc == 0) {
        /*
         * Allocate memory for the avl tree structure.
         */
        if (*avl_tree == NULL) {
            *avl_tree = malloc_fn(sizeof(struct avl_tree));
        }
        if (*avl_tree == NULL) {
            rc = -1;
        } else {
            malloced_mem = TRUE;
        }
    }

    if (rc == 0) {
        /*
         * Fill in the values in the avl tree struct
         */
        (*avl_tree)->malloc_fn  = malloc_fn;
        (*avl_tree)->free_fn    = free_fn;
        (*avl_tree)->cleanup_fn = default_cleanup_fn;
        (*avl_tree)->compare_fn = compare_fn;
        (*avl_tree)->sanity_fn  = default_sanity_fn;
        (*avl_tree)->root       = NULL;
        (*avl_tree)->options    = options;
        (*avl_tree)->node_cnt   = 0;
        if (malloced_mem
            && (options & AVL_TREE_OPTION_NOT_THREADSAFE) != AVL_TREE_OPTION_NOT_THREADSAFE) {
            if (pthread_mutex_init(&((*avl_tree)->mutex), NULL) != 0) {
                rc = -1;
            }
        }
    }

    if (rc != SUCCESS) {
        /*
         * Free up memory
         */
        if (malloced_mem) {
            free_fn(*avl_tree);
            *avl_tree = NULL;
        }
        ERR("%s: INIT request - FAIL retcode: %d", __FUNCTION__, rc);
    } else {
        DEBUG("%s: INIT request - SUCCESS tree handle: %p", __FUNCTION__, *avl_tree);
    }

    return rc;
}

int
avl_tree_init_fns(avl_tree_malloc_fn malloc_fn, avl_tree_free_fn free_fn,
                  avl_tree_cleanup_fn cleanup_fn, avl_tree_object_sanity_fn sanity_fn,
                  avl_tree_h_td avl_tree)
{
    int rc = 0;

    DEBUG("%s: init_fns request", __FUNCTION__);

    rc = avl_tree_lock(&(avl_tree->mutex), avl_tree->options);

    /*
     * Fill in the values in the avl tree struct
     */
    if (rc == 0) {
        if (malloc_fn) {
            avl_tree->malloc_fn = malloc_fn;
        }
        if (free_fn) {
            avl_tree->free_fn = free_fn;
        }
        if (cleanup_fn) {
            avl_tree->cleanup_fn = cleanup_fn;
        }
        if (sanity_fn) {
            avl_tree->sanity_fn = sanity_fn;
        }
        rc = avl_tree_unlock(&(avl_tree->mutex), avl_tree->options);
    }

    if (rc != SUCCESS) {
        ERR("%s: INIT_FNS request - FAIL tree handle:%p "
            " %d",
            __FUNCTION__, avl_tree, rc);
    } else {
        DEBUG("%s: INIT_FNS request - SUCCESS tree handle:%p", __FUNCTION__, avl_tree);
    }

    return rc;
}

int
avl_tree_allocate_object(avl_tree_h_td avl_tree, void **object, uint32_t size)
{
    uint32_t            len;
    int                 rc           = 0;
    avl_tree_object_td *local_object = NULL;

    DEBUG("%s: ALLOCATE request - tree handle:%p size:%d", __FUNCTION__, avl_tree, size);

    if (avl_tree == NULL || object == NULL) {
        rc = -1;
    }

    len = size + sizeof(avl_tree_object_td);
    if (rc == 0) {
        *object = avl_tree->malloc_fn(len);
        if (*object == NULL) {
            rc = -1;
        }
    }

    if (rc == 0) {
        /*
         * Insert Magic number #
         */
        local_object                = *object;
        local_object->magic_num     = AVL_MAGIC_NUM;
        local_object->internal_data = 0;

        *object = local_object->data;
    }

    if (rc == 0) {
        DEBUG("%s: ALLOCATE request - SUCCESS tree handle: %p"
              " object:%p size:%d",
              __FUNCTION__, avl_tree, *object, size);
    } else {
        ERR("%s: ALLOCATE request - FAIL tree handle:%p"
            " object:%p size:%d retcode:%d",
            __FUNCTION__, avl_tree, object ? *object : 0, size, rc);
    }

    return rc;
}

int
avl_tree_free_object(avl_tree_h_td avl_tree, void *object)
{
    int                 rc           = 0;
    avl_tree_object_td *local_object = NULL;

    DEBUG("%s: FREE request - tree handle:%p object:%p", __FUNCTION__, avl_tree, object);

    if (avl_tree == NULL || object == NULL) {
        rc = -1;
    }

    AVL_TREE_CHECK_MAGIC_NUM(object);

    if (rc == 0) {
        if (((avl_tree_object_td *)local_object)->internal_data != 0) {
            rc = -1;
        }
    }

    if (rc == 0) {
        DEBUG("%s: FREE request - tree handle:0%p object:%p"
              " tree_count: %d",
              __FUNCTION__, avl_tree, object, local_object->internal_data);
        avl_tree->cleanup_fn(object);
        object = (avl_tree_object_td *)object - 1;
        avl_tree->free_fn(object);
    }

    if (rc == 0) {
        DEBUG("%s: FREE request - SUCCESS tree handle:%p", __FUNCTION__, avl_tree);
    } else {
        ERR("%s: FREE request - FAIL tree handle:%p object:%p"
            " retcode:%d",
            __FUNCTION__, avl_tree, object, rc);
    }

    return rc;
}

int
avl_tree_insert(avl_tree_h_td avl_tree, void *object, void **existing_object)
{
    int                    rc             = SUCCESS;
    avl_tree_node_type_td *new_node       = NULL;
    bool                   balance_needed = TRUE;
    avl_tree_node_type_td *new_node_ptr   = NULL;
    avl_tree_object_td *   local_object   = NULL;
    bool                   is_new_node    = FALSE;

    DEBUG("%s: INSERT request - tree handle:%p object:%p", __FUNCTION__, avl_tree, object);

    if (avl_tree == NULL || object == NULL) {
        rc = -1;
    }

    AVL_TREE_CHECK_MAGIC_NUM(object);

    if (rc == 0) {
        DEBUG("%s: INSERT request - tree handle:%p object:%p"
              " tree_count: %d",
              __FUNCTION__, avl_tree, object, local_object->internal_data);
        rc = avl_tree->sanity_fn(object);
    }

    if (rc == 0) {
        /*
         * Allocate memory for tree node
         */
        new_node = avl_tree->malloc_fn(sizeof(avl_tree_node_type_td));
        if (new_node == NULL) {
            rc = -1;
        }
    }

    if (rc == 0) {
        memset(new_node, 0, sizeof(avl_tree_node_type_td));
        new_node->object = local_object;
        rc               = avl_tree_lock(&(avl_tree->mutex), avl_tree->options);
    }

    if (rc == 0) {
        new_node_ptr = avl_insert_internal(&avl_tree->root, new_node, &balance_needed,
                                           avl_tree_compare_internal, avl_tree, &is_new_node);

        if (is_new_node == TRUE) {
            avl_tree->node_cnt++;
        }

        rc = avl_tree_unlock(&(avl_tree->mutex), avl_tree->options);

        if (is_new_node == FALSE) {
            /*
             * The node could not be inserted, clean up and return error
             */
            rc = -1;
            if (existing_object) {
                local_object     = new_node_ptr->object;
                *existing_object = local_object->data;
            }
        }
    }

    if (rc == 0) {
        local_object = (avl_tree_object_td *)object - 1;
        AVL_TREE_INCREMENT_TREE_COUNTER(local_object);
    }

    if (rc != 0) {
        /*
         * Clean up
         */
        if (new_node != NULL) {
            avl_tree->free_fn(new_node);
        }
        ERR("%s: INSERT request - FAIL tree handle:%p object:%p"
            " %d",
            __FUNCTION__, avl_tree, object, rc);
    } else {
        DEBUG("%s: INSERT request - SUCCESS tree handle:%p"
              "  object:%p tree_count: %d",
              __FUNCTION__, avl_tree, object, local_object->internal_data);
    }

    return rc;
}

int
avl_tree_delete(avl_tree_h_td avl_tree, void *object, void **deleted_object)
{
    int                    rc             = 0;
    bool                   balance_needed = TRUE;
    avl_tree_node_type_td *del_node_ptr   = NULL;
    avl_tree_node_type_td  target_node;
    avl_tree_object_td *   local_object = NULL;

    DEBUG("%s: DELETE request - tree handle:%p object:%p", __FUNCTION__, avl_tree, object);

    if (avl_tree == NULL || object == NULL) {
        rc = -1;
        DEBUG("%s: rc = AVL_TREE_ERROR_INVARG", __FUNCTION__);
    }

    AVL_TREE_CHECK_MAGIC_NUM(object);

    if (rc == 0) {
        DEBUG("%s: DELETE request - tree handle:%p object:%p"
              " tree_count: %d",
              __FUNCTION__, avl_tree, object, local_object->internal_data);
    }

    if (rc == 0) {
        rc = avl_tree->sanity_fn(object);
        DEBUG("%s: rc = %d", __FUNCTION__, avl_tree->sanity_fn(object));
    }

    if (rc == 0) {
        DEBUG("%s: rc = SUCCESS", __FUNCTION__);
        memset(&target_node, 0, sizeof(avl_tree_node_type_td));
        target_node.object = local_object;
        rc                 = avl_tree_lock(&(avl_tree->mutex), avl_tree->options);
    }

    if (rc == 0) {
        DEBUG("%s: rc = SUCCESS after avl_tree_lock", __FUNCTION__);
        del_node_ptr = avl_delete_internal(&avl_tree->root, &target_node, &balance_needed,
                                           avl_tree_compare_internal, avl_tree);

        if (del_node_ptr != NULL) {
            DEBUG("%s: del_node_ptr != NULL", __FUNCTION__);
            avl_tree->node_cnt--;
        }

        rc = avl_tree_unlock(&(avl_tree->mutex), avl_tree->options);

        if (del_node_ptr == NULL) {
            /*
             * The node could not be found
             */
            rc = -1;
            DEBUG("%s: del_node_ptr == NULL, rc == AVL_TREE_ERROR_NOTFOUND", __FUNCTION__);
        }
    }

    AVL_TREE_DECREMENT_TREE_COUNTER(del_node_ptr->object);

    if (rc == 0) {
        DEBUG("%s: rc = SUCCESS after avl_tree_unlock", __FUNCTION__);
        if (deleted_object == NULL) {
            rc = avl_tree_free_object(avl_tree, (void *)(del_node_ptr->object->data));
        } else {
            *deleted_object = del_node_ptr->object->data;
        }
        /*
         * Remove the tree node
         */
        avl_tree->free_fn(del_node_ptr);
    }

    if (rc == 0) {
        DEBUG("%s: DELETE request - SUCCESS tree handle:%p"
              "  object:%p tree_count: %d",
              __FUNCTION__, avl_tree, object, local_object->internal_data);
    } else {
        ERR("%s: DELETE request - FAIL tree handle:%p object:%p"
            " %d",
            __FUNCTION__, avl_tree, object, rc);
    }

    return rc;
}

int
avl_tree_search(avl_tree_h_td avl_tree, avl_tree_search_options_td options, void *compare_object,
                void **found_object, avl_tree_search_options_td *result)
{
    int                    rc                = 0;
    avl_tree_node_type_td *searched_node_ptr = NULL;
    avl_tree_node_type_td  target_node;
    bool                   is_equal     = FALSE;
    avl_tree_object_td *   local_object = NULL;

    DEBUG("%s: SEARCH request - tree handle:%p options:%d"
          " compare object:%p",
          __FUNCTION__, avl_tree, options, compare_object);

    if (avl_tree == NULL || found_object == NULL) {
        rc = -1;
    }

    if ((options != AVL_TREE_OPTION_MIN) && (options != AVL_TREE_OPTION_MAX)) {
        if (compare_object == NULL) {
            rc = -1;
        }
        if (rc == SUCCESS) {
            rc = avl_tree->sanity_fn(compare_object);
        }
        AVL_TREE_CHECK_MAGIC_NUM(compare_object);
    }

    if (rc == 0) {
        target_node.object = local_object;
        rc                 = avl_tree_lock(&(avl_tree->mutex), avl_tree->options);
    }

    if (rc == 0) {
        if (options == AVL_TREE_OPTION_EQ) {
            searched_node_ptr = avl_search_internal(avl_tree->root, &target_node,
                                                    avl_tree_compare_internal, avl_tree);
        } else if (options == AVL_TREE_OPTION_LT) {
            searched_node_ptr =
                avl_get_prev_v2(avl_tree->root, &target_node, avl_tree_compare_internal, avl_tree);
        } else if (options == AVL_TREE_OPTION_LE) {
            searched_node_ptr = avl_get_smaller_equal_v2(
                avl_tree->root, &target_node, avl_tree_compare_internal, avl_tree, &is_equal);
            if (searched_node_ptr) {
                if (result) {
                    if (is_equal) {
                        *result = AVL_TREE_OPTION_EQ;
                    } else {
                        *result = AVL_TREE_OPTION_LT;
                    }
                }
            }
        } else if (options == AVL_TREE_OPTION_GT) {
            searched_node_ptr =
                avl_get_next_v2(avl_tree->root, &target_node, avl_tree_compare_internal, avl_tree);
        } else if (options == AVL_TREE_OPTION_GE) {
            searched_node_ptr = avl_get_greater_equal_v2(
                avl_tree->root, &target_node, avl_tree_compare_internal, avl_tree, &is_equal);
            if (searched_node_ptr) {
                if (result) {
                    if (is_equal) {
                        *result = AVL_TREE_OPTION_EQ;
                    } else {
                        *result = AVL_TREE_OPTION_GT;
                    }
                }
            }
        } else if (options == AVL_TREE_OPTION_MIN) {
            searched_node_ptr = avl_tree_get_first(avl_tree->root);
        } else if (options == AVL_TREE_OPTION_MAX) {
            searched_node_ptr = avl_tree_get_last(avl_tree->root);
        } else {
            rc = -1;
        }

        rc = avl_tree_unlock(&(avl_tree->mutex), avl_tree->options);

        if (searched_node_ptr == NULL) {
            /*
             * The node could not be found
             */
            rc = -1;
        }
    }

    if (rc == 0) {
        *found_object = searched_node_ptr->object->data;
    }

    if (rc == 0) {
        DEBUG("%s: SEARCH request - SUCCESS tree handle:%p options:%d"
              " compare object:%p",
              __FUNCTION__, avl_tree, options, compare_object);
    } else {
        ERR("%s: SEARCH request - FAIL tree handle:%p options:%d"
            " compare object:%p retcode:%d",
            __FUNCTION__, avl_tree, options, compare_object, rc);
    }

    return rc;
}

uint32_t
avl_tree_count(avl_tree_h_td avl_tree)
{
    if (avl_tree == NULL) {
        return (0);
    } else {
        return (avl_tree->node_cnt);
    }
}

/*
 * This avl_tree_walk() CANNOT do deletion while walking.
 */
int
avl_tree_walk(avl_tree_h_td avl_tree, avl_tree_walk_fn walk_fn, void *ctx,
              avl_tree_walk_code_td *result)
{
    int rc = 0;

    DEBUG("%s: WALK request - tree handle:%p walk fn:%p ctx:%p", __FUNCTION__, avl_tree, walk_fn,
          ctx);

    if (avl_tree == NULL || walk_fn == NULL || result == NULL) {
        rc = -1;
    }

    *result = AVL_CONT_WALK;

    if (avl_tree->root == NULL) {
        return (0);
    }

    if (rc == 0) {
        rc = avl_tree_lock(&(avl_tree->mutex), avl_tree->options);
    }

    if (rc == 0) {
        *result = avl_walk_internal(avl_tree->root, avl_tree_walk_internal, (void *)walk_fn, ctx);
        rc      = avl_tree_unlock(&(avl_tree->mutex), avl_tree->options);
    }

    if (rc == 0) {
        DEBUG("%s: WALK request - SUCCESS tree handle:%p walk fn:%p"
              " ctx:%p, result:%d",
              __FUNCTION__, avl_tree, walk_fn, ctx, *result);
    } else {
        ERR("%s: WALK request - FAILURE tree handle:%p walk fn:%p"
            " ctx:%p, result:%d, retcode:%d",
            __FUNCTION__, avl_tree, walk_fn, ctx, result ? *result : 0, rc);
    }

    return rc;
}

/*
 * This avl_tree_walk_safe() CAN do deletion while walking.
 */
int
avl_tree_walk_safe(avl_tree_h_td avl_tree, avl_tree_walk_fn walk_fn, void *ctx,
                   avl_tree_walk_code_td *result)
{
    avl_tree_node_type_td *a_node;
    avl_tree_node_type_td *node_next;
    int                    rc = 0;

    DEBUG("%s: WALK request - tree handle:%p walk fn:%p ctx:%p", __FUNCTION__, avl_tree, walk_fn,
          ctx);

    if (avl_tree == NULL || walk_fn == NULL || result == NULL) {
        rc = -1;
    }

    *result = AVL_CONT_WALK;

    if (avl_tree->root == NULL) {
        return (0);
    }

    if (rc == 0) {
        rc = avl_tree_lock(&(avl_tree->mutex), avl_tree->options);
    }

    if (rc == 0) {
        a_node = avl_tree_get_first(avl_tree->root);
        for (; ((a_node != NULL) && (*result == AVL_CONT_WALK));) {
            /*
             * Remember the next node first because a_node might be
             * deleted inside the loop.
             */
            node_next =
                avl_get_next_v2(avl_tree->root, a_node, avl_tree_compare_internal, avl_tree);
            *result = avl_tree_walk_internal(a_node, (void *)walk_fn, ctx);
            a_node  = node_next;
        }
        rc = avl_tree_unlock(&(avl_tree->mutex), avl_tree->options);
    }

    if (rc == 0) {
        DEBUG("%s: WALK request - SUCCESS tree handle:%p walk fn:%p"
              " ctx:%p, result:%d",
              __FUNCTION__, avl_tree, walk_fn, ctx, *result);
    } else {
        ERR("%s: WALK request - FAILURE tree handle:%p walk fn:%p"
            " ctx:%p, result:%d, retcode:%d",
            __FUNCTION__, avl_tree, walk_fn, ctx, result ? *result : 0, rc);
    }

    return rc;
}

int
avl_tree_shutdown(avl_tree_h_td *avl_tree, avl_tree_shutdown_code_td shutdown_code)
{
    int                    rc = SUCCESS;
    avl_tree_node_type_td *removed_node;
    bool                   balance_needed = FALSE;

    DEBUG("%s: SHUTDOWN request - tree handle:%p free object:%d", __FUNCTION__, avl_tree,
          shutdown_code);

    if (avl_tree == NULL || *avl_tree == NULL) {
        rc = -1;
    }

    if (rc == SUCCESS) {
        rc = avl_tree_lock(&((*avl_tree)->mutex), (*avl_tree)->options);
    }

    if (rc == 0) {
        while ((*avl_tree)->root != NULL) {
            removed_node =
                avl_delete_internal(&(*avl_tree)->root, (*avl_tree)->root, &balance_needed,
                                    avl_tree_compare_internal, *avl_tree);

            assert(removed_node);

            AVL_TREE_DECREMENT_TREE_COUNTER(removed_node->object);

            if (shutdown_code == AVL_FREE_OBJECTS) {
                rc = avl_tree_free_object(*avl_tree, removed_node->object->data);
            }
            (*avl_tree)->free_fn(removed_node);
        }
        rc = avl_tree_unlock(&((*avl_tree)->mutex), (*avl_tree)->options);
    }

    /*
     * Free the avl structure
     */
    if (rc == 0) {
        if (((*avl_tree)->options & AVL_TREE_OPTION_NOT_THREADSAFE)
            != AVL_TREE_OPTION_NOT_THREADSAFE) {
            if (pthread_mutex_destroy(&((*avl_tree)->mutex)) != 0) {
                rc = -1;
            }
        }
        (*avl_tree)->free_fn(*avl_tree);
        *avl_tree = NULL;
    }

    if (rc == 0) {
        DEBUG("%s: SHUTDOWN request - SUCCESS tree handle:%p"
              " free object:%d\n",
              __FUNCTION__, avl_tree, shutdown_code);
    } else {
        ERR("%s: SHUTDOWN request - FAILURE tree handle:%p"
            " shutdown_code:%d retcode:%d",
            __FUNCTION__, avl_tree, shutdown_code, rc);
    }

    return rc;
}

/*
 * Display an AVL tree via DEBUG.
 * Not memlock protected yet, tho it needs it.
 */
void
avl_tree_debug(avl_tree_h_td avl_tree)
{
    avl_tree_node_type_td *a_node;

    DEBUG("%s: avl_tree=%p, root=%p, first=%p, last=%p", __FUNCTION__, avl_tree, avl_tree->root,
          avl_tree_get_first(avl_tree->root), avl_tree_get_last(avl_tree->root));

    for (a_node = avl_tree_get_first(avl_tree->root); a_node;
         a_node = avl_get_next_v2(avl_tree->root, a_node, avl_tree_compare_internal, avl_tree)) {
        DEBUG("    %p, %p %p %s", a_node, a_node->left, a_node->right,
              avl_balance_string_internal(a_node->balance));
    }
}
