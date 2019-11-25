/*
 * rbtree_augmented.h -- Red Black Trees
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/04/17 21:19:22
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

#ifndef RBTREE_AUGMENTED_H
#define RBTREE_AUGMENTED_H

/*
 * red-black trees properties:  http://en.wikipedia.org/wiki/Rbtree
 *
 *  1) A node is either red or black
 *  2) The root is black
 *  3) All leaves (NULL) are black
 *  4) Both children of every red node are black
 *  5) Every simple path from root to leaves contains the same number
 *     of black nodes.
 *
 *  4 and 5 give the O(log n) guarantee, since 4 implies you cannot have two
 *  consecutive red nodes in a path and every red node is therefore followed by
 *  a black. So if B is the number of black nodes on every simple path (as per
 *  5), then the longest possible path due to 4 is 2B.
 *
 *  We shall indicate color with case, where black nodes are uppercase and red
 *  nodes will be lowercase. Unknown color nodes shall be drawn as red within
 *  parentheses and have some accompanying text comment.
 */

/*
 * Notes on lockless lookups:
 *
 * All stores to the tree structure (rb_left and rb_right) must be done using
 * WRITE_ONCE(). And we must not inadvertently cause (temporary) loops in the
 * tree structure as seen in program order.
 *
 * These two requirements will allow lockless iteration of the tree -- not
 * correct iteration mind you, tree rotations are not atomic so a lookup might
 * miss entire subtrees.
 *
 * But they do guarantee that any such traversal will only see valid elements
 * and that it will indeed complete -- does not get stuck in a loop.
 *
 * It also guarantees that if the lookup returns an element it is the 'correct'
 * one. But not returning an element does _NOT_ mean it's not present.
 *
 * NOTE:
 *
 * Stores to __rb_parent_color are not important for simple lookups so those
 * are left undone as of now. Nor did I check for loops involving parent
 * pointers.
 */

#include "rbtree.h"

/*
 * Please note - only struct rb_augment_callbacks and the prototypes
 * for rb_insert_augmented() and rb_erase_augmented() are intended to
 * be public. The rest are implementation details you are not expected
 * to depend on.
 *
 */
struct rb_augment_callbacks {
    void (*propagate)(struct rb_node *node, struct rb_node *stop);
    void (*copy)(struct rb_node *old, struct rb_node *new);
    void (*rotate)(struct rb_node *old, struct rb_node *new);
};

extern void __rb_insert_augmented(struct rb_node *node, struct rb_root *root,
                                  void (*augment_rotate)(struct rb_node *old, struct rb_node *new));

static inline void
rb_insert_augmented(struct rb_node *node, struct rb_root *root,
                    const struct rb_augment_callbacks *augment)
{
    __rb_insert_augmented(node, root, augment->rotate);
}

#define RB_DECLARE_CALLBACKS(rbstatic, rbname, rbstruct, rbfield, rbtype, rbaugmented, rbcompute) \
    static inline void rbname##_propagate(struct rb_node *rb, struct rb_node *stop)               \
    {                                                                                             \
        while (rb != stop) {                                                                      \
            rbstruct *node      = rb_entry(rb, rbstruct, rbfield);                                \
            rbtype    augmented = rbcompute(node);                                                \
            if (node->rbaugmented == augmented)                                                   \
                break;                                                                            \
            node->rbaugmented = augmented;                                                        \
            rb                = rb_parent(&node->rbfield);                                        \
        }                                                                                         \
    }                                                                                             \
    static inline void rbname##_copy(struct rb_node *rb_old, struct rb_node *rb_new)              \
    {                                                                                             \
        rbstruct *old    = rb_entry(rb_old, rbstruct, rbfield);                                   \
        rbstruct *new    = rb_entry(rb_new, rbstruct, rbfield);                                   \
        new->rbaugmented = old->rbaugmented;                                                      \
    }                                                                                             \
    static void rbname##_rotate(struct rb_node *rb_old, struct rb_node *rb_new)                   \
    {                                                                                             \
        rbstruct *old    = rb_entry(rb_old, rbstruct, rbfield);                                   \
        rbstruct *new    = rb_entry(rb_new, rbstruct, rbfield);                                   \
        new->rbaugmented = old->rbaugmented;                                                      \
        old->rbaugmented = rbcompute(old);                                                        \
    }                                                                                             \
    rbstatic const struct rb_augment_callbacks rbname = {rbname##_propagate, rbname##_copy,       \
                                                         rbname##_rotate};

#define RB_RED 0
#define RB_BLACK 1

#define __rb_parent(pc) ((struct rb_node *)(pc & ~3))

#define __rb_color(pc) ((pc)&1)
#define __rb_is_black(pc) __rb_color(pc)
#define __rb_is_red(pc) (!__rb_color(pc))
#define rb_color(rb) __rb_color((rb)->__rb_parent_color)
#define rb_is_red(rb) __rb_is_red((rb)->__rb_parent_color)
#define rb_is_black(rb) __rb_is_black((rb)->__rb_parent_color)

static inline void
rb_set_parent(struct rb_node *rb, struct rb_node *p)
{
    rb->__rb_parent_color = rb_color(rb) | (unsigned long)p;
}

static inline void
rb_set_parent_color(struct rb_node *rb, struct rb_node *p, int color)
{
    rb->__rb_parent_color = (unsigned long)p | color;
}

static inline void
__rb_change_child(struct rb_node *old, struct rb_node *new, struct rb_node *parent,
                  struct rb_root *root)
{
    if (parent) {
        if (parent->rb_left == old)
            WRITE_ONCE(parent->rb_left, new);
        else
            WRITE_ONCE(parent->rb_right, new);
    } else
        WRITE_ONCE(root->rb_node, new);
}

extern void __rb_erase_color(struct rb_node *parent, struct rb_root *root,
                             void (*augment_rotate)(struct rb_node *old, struct rb_node *new));

static __always_inline struct rb_node *
__rb_erase_augmented(struct rb_node *node, struct rb_root *root,
                     const struct rb_augment_callbacks *augment)
{
    struct rb_node *child = node->rb_right;
    struct rb_node *tmp   = node->rb_left;
    struct rb_node *parent, *rebalance;
    unsigned long   pc;

    if (!tmp) {
        pc     = node->__rb_parent_color;
        parent = __rb_parent(pc);
        __rb_change_child(node, child, parent, root);
        if (child) {
            child->__rb_parent_color = pc;
            rebalance                = NULL;
        } else
            rebalance = __rb_is_black(pc) ? parent : NULL;
        tmp = parent;
    } else if (!child) {
        tmp->__rb_parent_color = pc = node->__rb_parent_color;
        parent                      = __rb_parent(pc);
        __rb_change_child(node, tmp, parent, root);
        rebalance = NULL;
        tmp       = parent;
    } else {
        struct rb_node *successor = child, *child2;

        tmp = child->rb_left;
        if (!tmp) {
            parent = successor;
            child2 = successor->rb_right;

            augment->copy(node, successor);
        } else {
            do {
                parent    = successor;
                successor = tmp;
                tmp       = tmp->rb_left;
            } while (tmp);
            child2 = successor->rb_right;
            WRITE_ONCE(parent->rb_left, child2);
            WRITE_ONCE(successor->rb_right, child);
            rb_set_parent(child, successor);

            augment->copy(node, successor);
            augment->propagate(parent, successor);
        }

        tmp = node->rb_left;
        WRITE_ONCE(successor->rb_left, tmp);
        rb_set_parent(tmp, successor);

        pc  = node->__rb_parent_color;
        tmp = __rb_parent(pc);
        __rb_change_child(node, successor, tmp, root);

        if (child2) {
            successor->__rb_parent_color = pc;
            rb_set_parent_color(child2, parent, RB_BLACK);
            rebalance = NULL;
        } else {
            unsigned long pc2            = successor->__rb_parent_color;
            successor->__rb_parent_color = pc;
            rebalance                    = __rb_is_black(pc2) ? parent : NULL;
        }
        tmp = successor;
    }

    augment->propagate(tmp, NULL);
    return rebalance;
}

static __always_inline void
rb_erase_augmented(struct rb_node *node, struct rb_root *root,
                   const struct rb_augment_callbacks *augment)
{
    struct rb_node *rebalance = __rb_erase_augmented(node, root, augment);
    if (rebalance)
        __rb_erase_color(rebalance, root, augment->rotate);
}

#endif
