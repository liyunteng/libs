/*
 * ringtab.c - ringtab
 *
 * Date   : 2020/04/27
 */

#include <string.h>
#include "ringtab.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/****************************************
 * Inline APIs for primitive definitions.
 ****************************************/
/*
 * Initialization.
 */
void
ringtab_init(ringtab_hdr_t *rtab,
             int16_t n_items,
             int16_t item_size)
{
    memset(rtab, 0, (sizeof(*rtab) + item_size * n_items));
    rtab->n_items = n_items;
    rtab->item_size = item_size;
    rtab->head = 0;
    rtab->tail = 0;
    rtab->dirty = RINGTAB_ITEM_NONE;
    rtab->in_use = RINGTAB_ITEM_NONE;
    rtab->flags.full = FALSE;
}

/*
 * The number of items in the table
 */
int16_t
ringtab_items_used(ringtab_hdr_t *rtab)
{
    int16_t diff;

    if (rtab->flags.full) {
        return (rtab->n_items);
    }

    diff = rtab->tail - rtab->head;
    return ((diff >= 0) ? diff : rtab->n_items + diff);
}

/*
 * The number of empty spots in the table
 */
int16_t
ringtab_items_left(ringtab_hdr_t *rtab)
{
    return (rtab->n_items - ringtab_items_used(rtab));
}

/*
 * Producer enqueue item at tail.
 * Note, producer calls this function to get the item pointer to write to.
 */
void *
ringtab_put_item(ringtab_hdr_t *rtab)
{
    void *item;

    if (ringtab_items_used(rtab) >= rtab->n_items) {
        return (NULL);          /* full */
    }

    /* Export data. */
    item = rtab->data + rtab->tail * rtab->item_size;

    /* Update counters. */
    rtab->tail++;
    rtab->tail %= rtab->n_items;
    if (rtab->tail == rtab->head) {
        rtab->flags.full = TRUE;
    }

    return (item);
}

/*
 * Consumer locate the item at head.
 * It can be called multiple times consecutively and get the same result.
 */
void *
ringtab_use_item(ringtab_hdr_t *rtab)
{
    if (ringtab_items_used(rtab) == 0) {
        return(NULL);           /* empty */
    }

    rtab->in_use = rtab->head;
    return ((void *)(rtab->data + rtab->in_use * rtab->item_size));
}

/*
 * Consumer locate the item at in_use+1.
 * It can be called multiple times consecutively and get the same result.
 * There is no mark for the item being peeked. Unlike the in-use item,
 * when cleanup, the peeked item is released as well.
 * For now, this is up to caller to maintain the serialization of peek
 * and clean.
 */
void *
ringtab_peek_next(ringtab_hdr_t *rtab)
{
    int16_t peek_item;

    /* It has to have at least 2 left to see the next item. */
    if (ringtab_items_used(rtab) <= 1) {
        return NULL;
    }

    if (rtab->in_use == RINGTAB_ITEM_NONE) {
        peek_item = rtab->head; /* in case we peek before use */
    } else {
        peek_item = rtab->in_use + 1; /* normal case */
        peek_item %= rtab->n_items;
    }

    return ((void *)(rtab->data + peek_item * rtab->item_size));
}


/*
 * Producer locate the last item at tail.
 * It can be called multiple times consecutively and get the same result.
 */
void *
ringtab_peek_last(ringtab_hdr_t *rtab)
{
    int16_t tail;

    tail = (rtab->tail == 0) ? rtab->n_items : rtab->tail;
    tail --;
    return ((void *)(rtab->data + tail * rtab->item_size));
}


/*
 * This function is called when the current in-use item is finished using.
 * Consumer set dirty pointer to in_use if it's not set yet;
 * then dequeue from head.
 */
void
ringtab_done_item(ringtab_hdr_t *rtab)
{
    if (rtab->dirty == RINGTAB_ITEM_NONE) {
        rtab->dirty = rtab->in_use;
    }

    rtab->in_use ++;
    rtab->in_use %= rtab->n_items;

    rtab->head = rtab->in_use;

    if (rtab->flags.full == TRUE) {
        rtab->flags.full = FALSE;
    }
}

/*
 * Producer release the dirty item.
 * All items between dirty->head need to clean.
 */
void *
ringtab_get_dirty(ringtab_hdr_t *rtab)
{
    void *item;

    if (rtab->dirty == RINGTAB_ITEM_NONE) {
        return NULL;
    }

    /*
     * Normally, this does not happen.
     * If no new items and all the rest are dirty,
     * use_item() returns NULL. Caller is expected to retry by itself.
     *
     * However, in case an item is in-use, it is not treated dirty,
     * hence, skipped. This happens when
     *   get_item() or use_item(), followed by cleanup().
     */
    if (rtab->dirty == rtab->in_use) {
        return (NULL);
    }

    item = (void *)(rtab->data + rtab->dirty * rtab->item_size);

    rtab->dirty ++;
    rtab->dirty %= rtab->n_items;

    if (rtab->dirty == rtab->head) {
        rtab->dirty = RINGTAB_ITEM_NONE;
    }

    return item;
}

/*
 * Consumer dequeue item from head.
 */
void *
ringtab_get_item(ringtab_hdr_t *rtab)
{
    void *item;

    item = ringtab_use_item(rtab);
    if (item == NULL) {
        return NULL;
    }

    rtab->head++;
    rtab->head %= rtab->n_items;
    if (rtab->flags.full == TRUE) {
        rtab->flags.full = FALSE;
    }

    return item;
}

/****************************************
 * A refill mechanism based on ring table.
 ****************************************/
/*
 * Producer clean up all dirty items and return new item pointer to write to.
 */
void *
ringtab_produce_item(ringtab_hdr_t *rtab,
                     ringtab_item_release_fn release_fn,
                     void *rel_ctx)
{
    void *item;

    ringtab_cleanup(rtab, release_fn, rel_ctx);
    item = ringtab_put_item(rtab);
    return (item);
}

/*
 * Conumer marks the current in-use item done and use the next one.
 */
void *
ringtab_consume_item(ringtab_hdr_t *rtab)
{
    void *item;

    ringtab_done_item(rtab);
    item = ringtab_use_item(rtab);
    return item;
}

/*
 * Producer clean up all dirty items.
 */
void
ringtab_cleanup(ringtab_hdr_t *rtab,
                ringtab_item_release_fn release_fn,
                void *rel_ctx)
{
    void *item;
    while ((item = ringtab_get_dirty(rtab)) != NULL) {
        if (release_fn != NULL) release_fn(rel_ctx, item);
    }
}

/*
 * Producer clean up all dirty items and not-in-use items.
 */
void
ringtab_cleanup_all(ringtab_hdr_t *rtab,
                    ringtab_item_release_fn release_fn,
                    void *rel_ctx)
{
    void *item;

    /* release used items */
    ringtab_cleanup(rtab, release_fn, rel_ctx);

    /* release left not-in-use items */
    while ((item = ringtab_get_item(rtab)) != NULL) {
        if (release_fn != NULL) release_fn(rel_ctx, item);
    }

    ringtab_init(rtab, rtab->n_items, rtab->item_size);
}
