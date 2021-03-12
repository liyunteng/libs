/*
 * avl_internal.c - avl_internal
 *
 * Author : liyunteng <liyunteng@streamocean.com>
 * Date   : 2019/09/03
 *
 * Copyright (C) 2019 StreamOcean, Inc.
 * All rights reserved.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "avl_internal.h"

/*
 * avl_balance_right_internal
 *
 * Called when we've just deleted an element from the right hand subtree,
 * and we may want to balance by moving nodes to the left.
 */
void
avl_balance_right_internal(avl_tree_node_type_td **element,
                           bool *balancing_needed)
{
    avl_tree_node_type_td *left, *right;

    switch ((*element)->balance) {
    case RIGHT_HEAVY:
        (*element)->balance = BALANCED;
        break;
    case BALANCED:
        (*element)->balance = LEFT_HEAVY;
        *balancing_needed   = FALSE;
        break;
    case LEFT_HEAVY:
        left = (*element)->left;
        /*
         * Single rotation.
         */
        if (left->balance <= BALANCED) { /* BALANCED or LEFT_HEAVY */
            (*element)->left = left->right;
            left->right      = *element;
            if (left->balance == BALANCED) {
                /* (*element)->balance = LEFT_HEAVY; */
                left->balance     = RIGHT_HEAVY;
                *balancing_needed = FALSE;
            } else { /* left->balanced == LEFT_HEAVY */
                (*element)->balance = BALANCED;
                left->balance       = BALANCED;
            }
            *element = left;
        } else { /* RIGHT_HEAVY */

            /*
             * Double rotation.
             */
            right            = left->right;
            left->right      = right->left;
            right->left      = left;
            (*element)->left = right->right;
            right->right     = *element;
            (*element)->balance =
                (right->balance == LEFT_HEAVY) ? RIGHT_HEAVY : BALANCED;
            left->balance =
                (right->balance == RIGHT_HEAVY) ? LEFT_HEAVY : BALANCED;
            *element       = right;
            right->balance = BALANCED;
        }
        break;
    }
}

/*
 * avl_balance_left_internal
 *
 * Called when we've just deleted an element from the left subtree, and we
 * may want to balance by moving nodes to the left.
 */
void
avl_balance_left_internal(avl_tree_node_type_td **element,
                          bool *balancing_needed)
{

    avl_tree_node_type_td *left, *right;

    switch ((*element)->balance) {
    case LEFT_HEAVY:
        (*element)->balance = BALANCED;
        break;
    case BALANCED:
        (*element)->balance = RIGHT_HEAVY;
        *balancing_needed   = FALSE;
        break;
    case RIGHT_HEAVY:
        right = (*element)->right;
        if (right->balance >= BALANCED) { /* BALANCED or RIGHT_HEAVY */
            /*
             * Single rotation.
             */
            (*element)->right = right->left;
            right->left       = *element;
            if (right->balance == BALANCED) {
                /* (*element)->balance = RIGHT_HEAVY; */
                right->balance    = LEFT_HEAVY;
                *balancing_needed = FALSE;
            } else { /* right->balance == RIGHT_HEAVY */
                (*element)->balance = BALANCED;
                right->balance      = BALANCED;
            }
            *element = right;
        } else { /* LEFT_HEAVY */

            /*
             * Double rotation.
             */
            left              = right->left;
            right->left       = left->right;
            left->right       = right;
            (*element)->right = left->left;
            left->left        = *element;
            (*element)->balance =
                (left->balance == RIGHT_HEAVY) ? LEFT_HEAVY : BALANCED;
            right->balance =
                (left->balance == LEFT_HEAVY) ? RIGHT_HEAVY : BALANCED;
            *element      = left;
            left->balance = BALANCED;
        }
        break;
    }
}

/*
 * avl_delete_replacement_internal
 *
 * We've deleted one node, and it has both left and right children.  Scroll
 * down and find the maximum child less than the deleted node.  Return the
 * node that we extracted.
 */
avl_tree_node_type_td *
avl_delete_replacement_internal(avl_tree_node_type_td **link,
                                bool *balancing_needed)
{
    avl_tree_node_type_td *retnode;

    if ((*link)->right) {
        retnode =
            avl_delete_replacement_internal(&(*link)->right, balancing_needed);
        if (*balancing_needed)
            avl_balance_right_internal(link, balancing_needed);
    } else {
        retnode           = *link;
        *link             = (*link)->left;
        *balancing_needed = TRUE;
    }
    return (retnode);
}

/*
 * avl_walk_internal
 *
 * This routine will walk the avl tree, calling the specified procedure
 * at each node.  The walk is done in order.
 *
 * If the passed procedure returns AVL_STOP_WALK - the walk will stop.  Returns
 * AVL_CONT_WALK if the walk completed successfully.
 *
 */
avl_tree_walk_code_td
avl_walk_internal(avl_tree_node_type_td *element, avl_walker_type_internal proc,
                  void *internal_ctx, void *paramptr)
{
    avl_tree_walk_code_td res;

    if (NULL == element) {
        return (AVL_CONT_WALK);
    }

    if (element->left) {
        res = avl_walk_internal(element->left, proc, internal_ctx, paramptr);
        if (res != AVL_CONT_WALK) {
            return (res);
        }
    }

    res = (*proc)(element, internal_ctx, paramptr);
    if (res != AVL_CONT_WALK) {
        return (res);
    }

    if (element->right) {
        res = avl_walk_internal(element->right, proc, internal_ctx, paramptr);
        if (res != AVL_CONT_WALK) {
            return (res);
        }
    }

    return (AVL_CONT_WALK);
}

/*
 * avl_search_internal
 *
 * Search the tree using the comparison function and return the matching
 * node.  If no matching node exists, then return NULL.  Goal is a dummy
 * node which the caller must allocate and search with the desired search
 * value.
 */
void *
avl_search_internal(avl_tree_node_type_td *top, avl_tree_node_type_td *goal,
                    avl_compare_type_internal compare_func, void *ctx)
{
    while (TRUE) {
        if (!top)
            break;

        switch ((*compare_func)(top, goal, ctx)) {
        case AVL_TREE_EQ: /* Found it */
            return (top);
        case AVL_TREE_LT:     /* top < goal */
            top = top->right; /* Search right */
            break;
        case AVL_TREE_GT:    /* top > goal */
            top = top->left; /* Search left */
            break;
        default: /* bogus case */
            return (NULL);
        }
    }
    return (NULL);
}

/*
 * avl_get_next_internal_v2
 *
 * Guts of avl_get_next, and avl_get_greater_equal
 */
static avl_tree_node_type_td *
avl_get_next_internal_v2(avl_tree_node_type_td *top,
                         avl_tree_node_type_td *element,
                         avl_compare_type_internal compare_func, void *ctx,
                         bool equal_match, bool *is_equal)
{
    avl_tree_node_type_td *subtree;

    if (!top)
        return (NULL);

    switch ((*compare_func)(top, element, ctx)) {

    default: /* bogus case */
        return (NULL);

    case AVL_TREE_EQ: /* Found it */
        if (equal_match) {
            *is_equal = TRUE;
            return (top);
        } else { /* Return the least member of the */
            *is_equal = FALSE;
            return (avl_tree_get_first(top->right)); /* right subtree */
        }
    case AVL_TREE_LT: /* top < element */
        /*
         * If top < element, then there can be nothing greater than element
         * in the left subtree, AND top is uninteresting.
         */
        return (avl_get_next_internal_v2(top->right, element, compare_func, ctx,
                                         equal_match, is_equal));

    case AVL_TREE_GT: /* top > element */
        /*
         * If top > element, then the right subtree is uninteresting.
         * Get the next least next element from the left subtree.  If it's
         * non-NULL, then it's less than top, so return that.  Otherwise,
         * return top.
         */
        subtree   = avl_get_next_internal_v2(top->left, element, compare_func,
                                           ctx, equal_match, is_equal);
        *is_equal = FALSE;
        return (subtree ? subtree : top);
    }
}

/*
 * avl_get_next_v2
 *
 * Get the least node greater than the value passed in 'element'.  Yes,
 * that's right SNMP fans, it's the all powerful GET NEXT operator!  Note
 * that 'element' can be a real node or a dummy node (i.e., not really in
 * the tree).
 *
 * Return values: NULL if there is no such element, or the next element.
 */
void *
avl_get_next_v2(avl_tree_node_type_td *top, avl_tree_node_type_td *current,
                avl_compare_type_internal compare_func, void *ctx)
{
    bool is_equal;

    return (avl_get_next_internal_v2(top, current, compare_func, ctx, FALSE,
                                     &is_equal));
}

/*
 * avl_get_greater_equal_v2
 *
 * You might have thought that mib implementors really want to get the next
 * element but since the MIB engine take care of that for us by incrementing
 * the index what we really want is the element whose index is greater
 * than/equal (GE) the input argument.
 *
 * Return values: NULL if there's no such element, or the next GE element
 */
void *
avl_get_greater_equal_v2(avl_tree_node_type_td *top,
                         avl_tree_node_type_td *element,
                         avl_compare_type_internal compare_func, void *ctx,
                         bool *is_equal)
{
    return (avl_get_next_internal_v2(top, element, compare_func, ctx, TRUE,
                                     is_equal));
}

/*
 * avl_get_prev_internal_v2
 *
 * Guts of avl_get_prev, and avl_get_smaller_equal
 */
static avl_tree_node_type_td *
avl_get_prev_internal_v2(avl_tree_node_type_td *top,
                         avl_tree_node_type_td *element,
                         avl_compare_type_internal compare_func, void *ctx,
                         bool equal_match, bool *is_equal)
{
    avl_tree_node_type_td *subtree;

    if (!top)
        return (NULL);

    switch ((*compare_func)(top, element, ctx)) {

    default: /* bogus case */
        return (NULL);

    case AVL_TREE_EQ: /* Found it */
        if (equal_match) {
            *is_equal = TRUE;
            return (top);
        } else { /* Return the greatest member of the */
            *is_equal = FALSE;
            return (avl_tree_get_last(top->left)); /* left subtree */
        }
    case AVL_TREE_LT: /* top < element */
        /*
         * If top < element, then there can be nothing greater than element
         * in the left subtree, AND top is uninteresting.
         */
        subtree   = avl_get_prev_internal_v2(top->right, element, compare_func,
                                           ctx, equal_match, is_equal);
        *is_equal = FALSE;
        return (subtree ? subtree : top);

    case AVL_TREE_GT: /* top > element */
        /*
         * If top > element, then the right subtree is uninteresting.
         */
        return (avl_get_prev_internal_v2(top->left, element, compare_func, ctx,
                                         equal_match, is_equal));
    }
}

/*
 * avl_get_prev_v2
 *
 * Get the geatest node smaller than the value passed in 'element'.
 * This function is analogous to GET PREV operator!  Note
 * that 'element' can be a real node or a dummy node (i.e., not really in
 * the tree).
 *
 * Return values: NULL if there is no such element, or the next element.
 */
void *
avl_get_prev_v2(avl_tree_node_type_td *top, avl_tree_node_type_td *current,
                avl_compare_type_internal compare_func, void *ctx)
{
    bool is_equal;

    return (avl_get_prev_internal_v2(top, current, compare_func, ctx, FALSE,
                                     &is_equal));
}

/*
 * avl_get_smaller_equal_v2
 *
 * This function returns element whose index is less than/equal (GE)
 * the input argument.
 *
 * Return values: NULL if there's no such element, or the prev LE element
 */
void *
avl_get_smaller_equal_v2(avl_tree_node_type_td *top,
                         avl_tree_node_type_td *element,
                         avl_compare_type_internal compare_func, void *ctx,
                         bool *is_equal)
{
    return (avl_get_prev_internal_v2(top, element, compare_func, ctx, TRUE,
                                     is_equal));
}

/*
 * avl_delete_internal
 *
 * Delete the target node from database, and rebalance tree if necessary.
 * Note that the target node may be a dummy node (i.e., not really in the
 * tree).  After extracting the (real) node from the tree, return it.
 * Return NULL if we can't find anything.
 */

void *
avl_delete_internal(avl_tree_node_type_td **top, avl_tree_node_type_td *target,
                    bool *balancing_needed,
                    avl_compare_type_internal compare_func, void *ctx)
{
    avl_tree_node_type_td *del_node;

    /*
     * Did not find node in tree.
     */
    del_node = NULL;
    if (!*top) {
        *balancing_needed = FALSE;
        return (NULL);
    }

    switch ((*compare_func)(target, *top, ctx)) {

    default: /* bogus case */
        *balancing_needed = FALSE;
        del_node          = NULL;
        break;

    case AVL_TREE_LT:

        /*
         * Target is less than top, traverse the left link.
         */
        del_node = avl_delete_internal(&(*top)->left, target, balancing_needed,
                                       compare_func, ctx);
        if (*balancing_needed)
            avl_balance_left_internal(top, balancing_needed);
        break;

    case AVL_TREE_GT:

        /*
         * Target is more than top, traverse the right link.
         */
        del_node = avl_delete_internal(&(*top)->right, target, balancing_needed,
                                       compare_func, ctx);
        if (*balancing_needed)
            avl_balance_right_internal(top, balancing_needed);
        break;

    case AVL_TREE_EQ:

        /*
         * Found entry in tree, delete it.
         */
        del_node = *top;

        if (!del_node->right) {
            *top              = del_node->left;
            *balancing_needed = TRUE;
            del_node->left    = NULL;
            del_node->right   = NULL;
            del_node->balance = BALANCED;
        } else if (!del_node->left) {
            *top              = del_node->right;
            *balancing_needed = TRUE;
            del_node->left    = NULL;
            del_node->right   = NULL;
            del_node->balance = BALANCED;
        } else {
            avl_tree_node_type_td *new_node;

            new_node          = avl_delete_replacement_internal(&del_node->left,
                                                       balancing_needed);
            new_node->left    = del_node->left;
            new_node->right   = del_node->right;
            new_node->balance = del_node->balance;
            *top              = new_node;
            del_node->left    = NULL;
            del_node->right   = NULL;
            del_node->balance = BALANCED;
            if (*balancing_needed)
                avl_balance_left_internal(top, balancing_needed);
        }
    }

    return (del_node);
}

/*
 * avl_insert_internal
 *
 * Given an initialized node, which is NOT in the tree, insert it into the
 * tree.  Return a pointer to the node or NULL if there's a duplicate.
 */
void *
avl_insert_internal(avl_tree_node_type_td **top, avl_tree_node_type_td *new,
                    bool *balancing_needed,
                    avl_compare_type_internal compare_func, void *ctx,
                    bool *is_new_node)
{
    avl_tree_node_type_td *left, *right;
    avl_tree_node_type_td *return_node = NULL;

    /* Initialize the search variables */
    if (!*top) {
        /* new element - insert it here */
        *balancing_needed = TRUE;
        *top              = new;
        new->left         = NULL;
        new->right        = NULL;
        new->balance      = BALANCED;
        *is_new_node      = TRUE;
        return (new);
    }

    switch ((*compare_func)(new, *top, ctx)) {

    default: /* bogus case */
        return_node       = NULL;
        *balancing_needed = FALSE;
        break;

    case AVL_TREE_LT:

        /*
         * Walk left.
         */
        return_node = avl_insert_internal(&(*top)->left, new, balancing_needed,
                                          compare_func, ctx, is_new_node);
        if (!*balancing_needed)
            break;

        switch ((*top)->balance) {
        case RIGHT_HEAVY:

            /*
             * Node was right heavy, now is balanced.
             */
            (*top)->balance   = BALANCED;
            *balancing_needed = FALSE;
            break;

        case BALANCED:

            /*
             * Node was balanced, now is left heavy.
             */
            (*top)->balance = LEFT_HEAVY;
            break;

        case LEFT_HEAVY:

            /*
             * Node was left heavy, need to rebalance.
             */
            left = (*top)->left;

            /*
             * Single LL rotation.
             */
            if (left->balance == LEFT_HEAVY) {
                (*top)->left    = left->right;
                left->right     = *top;
                (*top)->balance = BALANCED;
                *top            = left;
            } else {

                /*
                 * Double LR rotation
                 */
                if ((right = left->right) != NULL) {
                    left->right  = right->left;
                    right->left  = left;
                    (*top)->left = right->right;
                    right->right = *top;
                    (*top)->balance =
                        (right->balance == LEFT_HEAVY) ? RIGHT_HEAVY : BALANCED;
                    left->balance =
                        (right->balance == RIGHT_HEAVY) ? LEFT_HEAVY : BALANCED;
                    *top = right;
                }
            }
            (*top)->balance   = BALANCED;
            *balancing_needed = FALSE;
            break;
        }
        break;

    case AVL_TREE_GT:

        /*
         * Walk right.
         */
        return_node = avl_insert_internal(&(*top)->right, new, balancing_needed,
                                          compare_func, ctx, is_new_node);

        if (!*balancing_needed)
            break;

        switch ((*top)->balance) {
        case LEFT_HEAVY:

            /*
             * Node was left heavy, is now balanced.
             */
            (*top)->balance   = BALANCED;
            *balancing_needed = FALSE;
            break;

        case BALANCED:

            /*
             * Node was balanced, is now right heavy.
             */
            (*top)->balance = RIGHT_HEAVY;
            break;

        case RIGHT_HEAVY:

            /*
             * Node was right heavy, now needs rebalancing.
             */
            right = (*top)->right;

            /*
             * Single RR rotation.
             */
            if (right->balance == RIGHT_HEAVY) {
                (*top)->right   = right->left;
                right->left     = *top;
                (*top)->balance = BALANCED;
                *top            = right;
            } else {

                /*
                 * Double RL rotation.
                 */
                if ((left = right->left) != NULL) {
                    right->left   = left->right;
                    left->right   = right;
                    (*top)->right = left->left;
                    left->left    = *top;
                    (*top)->balance =
                        (left->balance == RIGHT_HEAVY) ? LEFT_HEAVY : BALANCED;
                    right->balance =
                        (left->balance == LEFT_HEAVY) ? RIGHT_HEAVY : BALANCED;
                    *top = left;
                }
            }
            (*top)->balance   = BALANCED;
            *balancing_needed = FALSE;
            break;
        }
        break;

    case AVL_TREE_EQ:

        /*
         * Found existing entry.
         */
        return_node       = *top;
        *balancing_needed = FALSE;
        break;
    }
    return (return_node);
}

/*
 * avl_balance_string
 *
 * Return a balance string representing the balance state.
 */
const char *
avl_balance_string_internal(avl_tree_balance_type_td bal)
{
    switch (bal) {
    case LEFT_HEAVY:
        return ("Left heavy");
    case BALANCED:
        return ("Balanced");
    case RIGHT_HEAVY:
        return ("Right heavy");
    default:
        return ("Confused");
    }
}

/*
 * avl_tree_get_first
 *
 * Return the least node in the tree, or NULL if there is no tree.
 */
avl_tree_node_type_td *
avl_tree_get_first(avl_tree_node_type_td *top)
{
    if (!top)
        return (NULL);

    while (top->left) /* Leftmost node is least */
        top = top->left;

    return (top);
}

/*
 * avl_tree_get_last
 *
 * Return the greatest node in the tree, or NULL if there is no tree.
 */
avl_tree_node_type_td *
avl_tree_get_last(avl_tree_node_type_td *top)
{
    if (!top)
        return (NULL);

    while (top->right) /* Rightmost node is greatest */
        top = top->right;

    return (top);
}
