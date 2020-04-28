/*
 * ringtab.h - ringtab
 *
 * Date   : 2020/04/27
 */
#ifndef RINGTAB_H
#define RINGTAB_H
#include <stdint.h>

#define RINGTAB_ITEM_NONE -1

/*
 * Can be used in user space and kernel.
 *
 * Note, this module does not provide mutual exclusion in case when
 * there is more than one producer to or consumer from the ring table.
 * However it works fine when there is one producer and one consumer.
 */
typedef struct {
    int16_t n_items;
    int16_t item_size;
    int16_t head;   /* dequeue by consumer */
    int16_t tail;   /* enqueue by producer */
    int16_t dirty;  /* the 1st dirty item */
    int16_t in_use; /* the item in use by consumer */
    struct {
        uint32_t full : 1; /* handle wrap around */
        uint32_t rsvd : 31;
    } flags;
    char data[0];
} ringtab_hdr_t;

#define RINGTAB_STRUCT(name, n_items, item_size)        \
    struct name {                                       \
        ringtab_hdr_t hdr;                              \
        char          data[(n_items) * (item_size)];    \
    }

typedef void (*ringtab_item_release_fn) (void *ctx, void *item);

/******************************************
 * Inline APIs for primitive definitions. *
 ******************************************/

/*
 * Initialization.
 */
void
ringtab_init (ringtab_hdr_t *rtab,
              int16_t n_items,
              int16_t item_size);


/*
 * The number of items in the table
 */
int16_t
ringtab_items_used(ringtab_hdr_t *rtab);


/*
 * The number of empty spots in the table
 */
int16_t
ringtab_items_left(ringtab_hdr_t *rtab);


/*
 * Producer enqueue item at tail.
 * Return TRUE if success; FALSE, otherwise.
 * Note, producer calls this function to get the item pointer to write to.
 */
void *
ringtab_put_item (ringtab_hdr_t  *rtab);


/*
 * Consumer locate the item at head.
 * It can be called multiple times consecutively and get the same result.
 */
void *
ringtab_use_item (ringtab_hdr_t  *rtab);

/*
 * Consumer locate the item at in_use+1.
 * It can be called multiple times consecutively and get the same result.
 * There is no mark for the item being peeked. Unlike the in-use item,
 * when cleanup, the peeked item is released as well.
 * For now, this is up to caller to maintain the serialization of peek
 * and clean.
 */
void *
ringtab_peek_next (ringtab_hdr_t  *rtab);

/*
 * Producer locate the last item at tail.
 * It can be called multiple times consecutively and get the same result.
 */
void *
ringtab_peek_last (ringtab_hdr_t  *rtab);


/*
 * Consumer set dirty pointer to in_use if it's not set yet;
 * then dequeue from head.
 */
void
ringtab_done_item (ringtab_hdr_t *rtab);


/*
 * Producer release the dirty item.
 * All items between dirty->head need to clean.
 */
void *
ringtab_get_dirty (ringtab_hdr_t *rtab);


/*
 * Consumer dequeue item from head.
 */
void *
ringtab_get_item (ringtab_hdr_t  *rtab);


/****************************************
 * A refill mechanism based on ring table.
 ****************************************/
/*
 * Producer clean up all dirty items and return new item pointer to write to.
 */
void *
ringtab_produce_item (ringtab_hdr_t           *rtab,
                      ringtab_item_release_fn  release_fn,
                      void                    *rel_ctx);

/*
 * Conumer marks the current in-use item done and use the next one.
 */
void *
ringtab_consume_item (ringtab_hdr_t  *rtab);

/*
 * Producer clean up all dirty items.
 */
void
ringtab_cleanup (ringtab_hdr_t           *rtab,
                 ringtab_item_release_fn  release_fn,
                 void                    *rel_ctx);

/*
 * Producer clean up all dirty items and not-in-use items.
 */
void
ringtab_cleanup_all (ringtab_hdr_t           *rtab,
                     ringtab_item_release_fn  release_fn,
                     void                    *rel_ctx);


#endif
