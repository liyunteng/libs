/*
 * assoc_array.h -- Generic associative array implementation
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/04/17 17:10:17
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

#ifndef ASSOC_ARRAY_H
#define ASSOC_ARRAY_H
#include "types.h"

#define ASSOC_ARRAY_KEY_CHUNK_SIZE BITS_PER_LONG

/*
 * Generic associative array.
 */
struct assoc_array {
    struct assoc_array_ptr *root; /* The node at the root of the tree */
    unsigned long           nr_leaves_on_tree;
};

struct assoc_array_ops {
    /* Method to get a chunk of an index key from caller-supplied data */
    unsigned long (*get_key_chunk)(const void *index_key, int level);

    /* Metohd to get a piece of an objects' index key */
    unsigned long (*get_object_key_chunk)(const void *object, int level);

    /* Is this the object we're looking for? */
    bool (*compare_object)(const void *object, const void *index_key);

    /*
     * How different is an object from an index key, to a bit
     * position in their key? (or -1 if they're the same)
     */
    int (*diff_objects)(const void *object, const void *index_key);

    /* Method to free an object. */
    void (*free_object)(void *object);
};

/*
 * Access and manipulation functions.
 */
struct assoc_array_edit;

static inline void
assoc_array_init(struct assoc_array *array)
{
    array->root              = NULL;
    array->nr_leaves_on_tree = 0;
}

extern int   assoc_array_iterate(const struct assoc_array *array,
                                 int (*iterator)(const void *object, void *iterator_data),
                                 void *iterator_data);
extern void *assoc_array_find(const struct assoc_array *array, const struct assoc_array_ops *ops,
                              const void *index_key);
extern void  assoc_array_destroy(struct assoc_array *array, const struct assoc_array_ops *ops);
extern struct assoc_array_edit *assoc_array_insert(struct assoc_array *          array,
                                                   const struct assoc_array_ops *ops,
                                                   const void *index_key, void *object);
extern void assoc_array_insert_set_object(struct assoc_array_edit *edit, void *object);
extern struct assoc_array_edit *assoc_array_delete(struct assoc_array *          array,
                                                   const struct assoc_array_ops *ops,
                                                   const void *                  index_key);
extern struct assoc_array_edit *assoc_array_clear(struct assoc_array *          array,
                                                  const struct assoc_array_ops *ops);
extern void                     assoc_array_apply_edit(struct assoc_array_edit *edit);
extern void                     assoc_array_cancel_edit(struct assoc_array_edit *edit);
extern int assoc_array_gc(struct assoc_array *array, const struct assoc_array_ops *  ops,
                          bool (*iterator)(void *object, void *iterator_data), void *iterator_data);
#endif
