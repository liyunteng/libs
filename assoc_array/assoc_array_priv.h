/*
 * assoc_array_priv.h -- assoc array priv
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/04/17 18:10:17
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

#ifndef ASSOC_ARRAY_PRIV_H
#define ASSOC_ARRAY_PRIV_H
#include "log2.h"

#define ASSOC_ARRAY_FAN_OUT 16 /* Number of slots per node */
#define ASSOC_ARRAY_FAN_MASK (ASSOC_ARRAY_FAN_OUT - 1)
#define ASSOC_ARRAY_LEVEL_STEP (ilog2(ASSOC_ARRAY_FAN_OUT))
#define ASSOC_ARRAY_LEVEL_STEP_MASK (ASSOC_ARRAY_LEVLE_SETUP - 1)
#define ASSOC_ARRAY_KEY_CHUNK_MASK (ASSOC_ARRAY_KEY_CHUNK_SIZE - 1)
#define ASSOC_ARRAY_KEY_CHUNK_SHIFT (ilog2(BITS_PER_LONG))

/*
 * Undefined type representing a pointer with type information in the bottom
 * two bits.
 */
struct assoc_array_ptr;

/* An N-way node in the tree.
 * Each slot contains one of four things:
 *	(1) Nothing (NULL).
 *	(2) A leaf object (pointer types 0).
 *	(3) A next-level node (ponter type 1, subtype 0).
 *	(4) A shortcut (pointer type 1, subtype 1).
 *
 * The tree is optimised for search-by-ID, but permits reasonable
 * iteration also.
 *
 * The tree is navigated by constructing an index key consisting of an
 * array of segments, where each segment is ilog2(ASSOC_ARRAY_FAN_OUT)
 * bits in size.
 *
 * The segments correspod to levels of the tree (the first segment is
 * used at level 0, the second at level 1, etc.).
 */
struct assoc_array_node {
    struct assoc_array_ptr *back_pointer;
    u8 parent_slot;
    struct assoc_array_ptr *slots[ASSOC_ARRAY_FAN_OUT];
    unsigned long nr_leaves_on_branch;
};

/*
 * A shortcut through the index space out to where a collection of
 * nodes/levels with the same IDs live.
 */
struct assoc_array_shortcut {
    struct assoc_array_ptr *back_pointer;
    int parent_slot;
    int skip_to_level;
    struct assoc_array_ptr *next_node;
    unsigned long index_key[];
};

struct assoc_array_edit {
    struct rcu_head rcu;
    struct assoc_array *array;
    const struct assoc_array_ops *ops;
    const struct assoc_array_ops *ops_for_excised_subtree;
    struct assoc_array_ptr *leaf;
    struct assoc_array_ptr **leaf_p;
    struct assoc_array_ptr *dead_leaf;
    struct assoc_array_ptr *new_meta[3];
    struct assoc_array_ptr *excised_meta[1];
    struct assoc_array_ptr *excised_subtree;
    struct assoc_array_ptr **set_backpointers[ASSOC_ARRAY_FAN_OUT];
    struct assoc_array_ptr *set_backpointers_to;
    struct assoc_array_ptr *adjust_count_on;
    long adjust_count_by;
    struct {
        struct assoc_array_ptr **ptr;
        struct assoc_array_ptr *to;
    } set[2];
    struct {
        u8 *p;
        u8 to;
    } set_parent_slot[1];
    u8 segment_cache[ASSOC_ARRAY_FAN_OUT + 1];
};

#define ASSOC_ARRAY_PTR_TYPE_MASK 0x1UL
#define ASSOC_ARRAY_PTR_LEAF_TYPE 0x0UL
#define ASSOC_ARRAY_PTR_META_TYPE 0x1UL
#define ASSOC_ARRAY_PTR_SUBTYPE_MASK 0x2UL
#define ASSOC_ARRAY_PTR_NODE_SUBTYPE 0x0UL
#define ASSOC_ARRAY_PTR_SHORTCUT_SUBTYPE 0x2UL

static inline bool
assoc_array_ptr_is_meta(const struct assoc_array_ptr *x)
{
    return (unsigned long)x & ASSOC_ARRAY_PTR_TYPE_MASK;
}

static inline bool
assoc_array_ptr_is_leaf(const struct assoc_array_ptr *x)
{
    return !assoc_array_ptr_is_meta(x);
}

static inline bool
assoc_array_ptr_is_shortcut(const struct assoc_array_ptr *x)
{
    return (unsigned long)x & ASSOC_ARRAY_PTR_SUBTYPE_MASK;
}

static inline bool
assoc_array_ptr_is_node(const struct assoc_array_ptr *x)
{
    return !assoc_array_ptr_is_shortcut(x);
}

static inline void *
assoc_array_ptr_to_leaf(const struct assoc_array_ptr *x)
{
    return (void *)((unsigned long)x & ~ASSOC_ARRAY_PTR_TYPE_MASK);
}

static inline unsigned long
__assoc_array_ptr_to_meta(const struct assoc_array_ptr *x)
{
    return (unsigned long)x
           & ~(ASSOC_ARRAY_PTR_SUBTYPE_MASK | ASSOC_ARRAY_PTR_TYPE_MASK);
}

static inline struct assoc_array_node *
assoc_array_ptr_to_node(const struct assoc_array_ptr *x)
{
    return (struct assoc_array_node *)__assoc_array_ptr_to_meta(x);
}

static inline struct assoc_array_shortcut *
assoc_array_ptr_to_shortcut(const struct assoc_array_ptr *x)
{
    return (struct assoc_array_shortcut *)__assoc_array_ptr_to_meta(x);
}

static inline struct assoc_array_ptr *
__assoc_array_x_to_ptr(const void *p, unsigned long t)
{
    return (struct assoc_array_ptr *)((unsigned long)p | t);
}

static inline struct assoc_array_ptr *
assoc_array_leaf_to_ptr(const void *p)
{
    return __assoc_array_x_to_ptr(p, ASSOC_ARRAY_PTR_LEAF_TYPE);
}

static inline struct assoc_array_ptr *
assoc_array_node_to_ptr(const struct assoc_array_node *p)
{
    return __assoc_array_x_to_ptr(p, ASSOC_ARRAY_PTR_META_TYPE
                                         | ASSOC_ARRAY_PTR_NODE_SUBTYPE);
}

static inline struct assoc_array_ptr *
assoc_array_shortcut_to_ptr(const struct assoc_array_shortcut *p)
{
    return __assoc_array_x_to_ptr(p, ASSOC_ARRAY_PTR_META_TYPE
                                         | ASSOC_ARRAY_PTR_SHORTCUT_SUBTYPE);
}
#endif
