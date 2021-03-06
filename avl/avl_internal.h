/*
 * avl_internal.h - avl_internal
 *
 * Date   : 2021/03/18
 */
#ifndef __AVL_INTERNAL_H__
#define __AVL_INTERNAL_H__

#include "avl.h"
#include "macro.h"
#include <stdint.h>
#include <pthread.h>

#define AVL_MAGIC_NUM 0xBE3A41C6

typedef uint32_t magic_type_td;

typedef struct avl_tree_object {
    magic_type_td magic_num;
    uint32_t internal_data;
    char data[0];
} avl_tree_object_td;

typedef enum avl_tree_balance_type_ {
    LEFT_HEAVY  = -1,
    BALANCED    = 0,
    RIGHT_HEAVY = 1,
} avl_tree_balance_type_td;

typedef struct avl_tree_node_type_ {
    struct avl_tree_node_type_ *left;
    struct avl_tree_node_type_ *right;
    avl_tree_balance_type_td balance;
    avl_tree_object_td *object;
} avl_tree_node_type_td;

#define AVL_TREE_CHECK_MAGIC_NUM(object)                                       \
    {                                                                          \
        if (rc == 0) {                                                         \
            local_object = (avl_tree_object_td *)object - 1;                   \
            if (local_object->magic_num != AVL_MAGIC_NUM) {                    \
                rc = -1;                                                       \
            }                                                                  \
        }                                                                      \
    }

#define AVL_TREE_INCREMENT_TREE_COUNTER(object)                                \
    {                                                                          \
        if (rc == 0) {                                                         \
            ((avl_tree_object_td *)object)->internal_data++;                   \
        }                                                                      \
    }

#define AVL_TREE_DECREMENT_TREE_COUNTER(object)                                \
    {                                                                          \
        if (rc == 0) {                                                         \
            ((avl_tree_object_td *)object)->internal_data--;                   \
            assert((int)((avl_tree_object_td *)object)->internal_data >= 0);   \
        }                                                                      \
    }

typedef avl_tree_walk_code_td (*avl_walker_type_internal)(
    avl_tree_node_type_td *node, void *internal_ctx, void *ctx);

typedef avl_tree_compare_code_td (*avl_compare_type_internal)(
    avl_tree_node_type_td *node_one, avl_tree_node_type_td *node_two,
    void *ctx);

extern avl_tree_walk_code_td avl_walk_internal(avl_tree_node_type_td *avl_node,
                                               avl_walker_type_internal proc,
                                               void *internal_ctx,
                                               void *paramptr);

extern void *avl_get_greater_equal_v2(avl_tree_node_type_td *top,
                                      avl_tree_node_type_td *current,
                                      avl_compare_type_internal compare_func,
                                      void *ctx, bool *is_equal);

extern void *avl_get_smaller_equal_v2(avl_tree_node_type_td *top,
                                      avl_tree_node_type_td *current,
                                      avl_compare_type_internal compare_func,
                                      void *ctx, bool *is_equal);

extern void *avl_get_next_v2(avl_tree_node_type_td *top,
                             avl_tree_node_type_td *current,
                             avl_compare_type_internal compare_func, void *ctx);

extern void *avl_get_prev_v2(avl_tree_node_type_td *top,
                             avl_tree_node_type_td *current,
                             avl_compare_type_internal compare_func, void *ctx);

extern void *avl_insert_internal(avl_tree_node_type_td **top,
                                 avl_tree_node_type_td *new,
                                 bool *balancing_needed,
                                 avl_compare_type_internal compare_func,
                                 void *ctx, bool *is_new_nod);

extern void *avl_delete_internal(avl_tree_node_type_td **top,
                                 avl_tree_node_type_td *target,
                                 bool *balancing_needed,
                                 avl_compare_type_internal compare_func,
                                 void *ctx);

extern void *avl_search_internal(avl_tree_node_type_td *top,
                                 avl_tree_node_type_td *goal,
                                 avl_compare_type_internal compare_func,
                                 void *ctx);

void avl_balance_left(avl_tree_node_type_td **element, bool *balancing_needed);

void avl_balance_right(avl_tree_node_type_td **element, bool *balancing_needed);

avl_tree_node_type_td *avl_delete_replacement(avl_tree_node_type_td **link,
                                              bool *balancing_needed);

const char *avl_balance_string_internal(avl_tree_balance_type_td bal);

extern avl_tree_node_type_td *avl_tree_get_first(avl_tree_node_type_td *top);

extern avl_tree_node_type_td *avl_tree_get_last(avl_tree_node_type_td *top);

extern int avl_tree_lock(pthread_mutex_t *mutex, avl_tree_options_td options);

extern int avl_tree_unlock(pthread_mutex_t *mutex, avl_tree_options_td options);

extern void default_cleanup_fn(void *mem_ptr);

extern int default_sanity_fn(void *obj);

extern void avl_balance_right_internal(avl_tree_node_type_td **element,
                                       bool *balancing_needed);

extern void avl_balance_left_internal(avl_tree_node_type_td **element,
                                      bool *balancing_needed);

extern avl_tree_node_type_td *
avl_delete_replacement_internal(avl_tree_node_type_td **link,
                                bool *balancing_needed);

#endif
