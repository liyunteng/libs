/*
 * mpool.c - mpool
 *
 * Date   : 2020/04/30
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpool.h"

//Enable this macro to detect memory leak issues.
#ifdef MPOOL_DBG

#    define LOC_MAX_FILE_LEN 16
typedef struct _loc_t {
    char file[LOC_MAX_FILE_LEN];
    int line;
    struct _loc_t *next;
} loc_t;

static loc_t *
loc_init(const char *file, int line)
{
    loc_t *loc = (loc_t *)malloc(sizeof(loc_t));
    strncpy(loc->file, file, LOC_MAX_FILE_LEN);
    loc->file[LOC_MAX_FILE_LEN - 1] = '\0';
    loc->line                       = line;
    loc->next                       = NULL;
    return loc;
}

static void *
loc_list_insert(void *list, loc_t *record)
{
    if (list == NULL) {
        return record;
    } else {
        loc_t *p = (loc_t *)list;
        for (; p->next != NULL; p = p->next);  //seek to the last record
        p->next = record;
        return list;
    }
}

static void
loc_list_free(void *list)
{
    loc_t *p = (loc_t *)list;
    loc_t *q = NULL;
    for (; p != NULL; p = q) {
        q = p->next;
        free(p);
        p = NULL;
    }
}

static void
loc_list_dump(mpool_item_t *item)
{
    fprintf(stderr, "================ Dump block %p\n", item);
    fprintf(stderr, "Get list:\n");
    loc_t *p = (loc_t *)item->get;
    for (; p != NULL; p = p->next) {
        fprintf(stderr, "\t%s:%d\n", p->file, p->line);
    }
    fprintf(stderr, "Put list:\n");
    for (p = (loc_t *)item->put; p != NULL; p = p->next) {
        fprintf(stderr, "\t%s:%d\n", p->file, p->line);
    }
    fprintf(stderr, "================ Dump block %p finished!\n\n", item);
}
#endif

mpool_ctx_t *
mpool_init(size_t msize, size_t total_size, void *pool)
{
    mpool_ctx_t *ctx = NULL;
    size_t size = (sizeof(mpool_item_t) + msize + ALIGN_SIZE - 1) / ALIGN_SIZE
                  * ALIGN_SIZE;
    if (msize == 0
        || total_size < sizeof(mpool_ctx_t) + sizeof(void *) + size) {
        //total_size must large enough to count 1 item at least.
        return NULL;
    }

    if (pool) {
        ctx = (mpool_ctx_t *)pool;
    } else {
        ctx = (mpool_ctx_t *)malloc(total_size);
        memset(ctx, 0, total_size);
        if (!ctx) {
            return NULL;
        }
    }

    if (ctx->flag & MPOOL_FLAG_READY) {
        atomic_inc(&ctx->ref);
        return ctx;
    } else {
        if (atomic_cmpxchg(&ctx->ref, 0, 1) != 0) {  //lock
            //loop to wait someone else init ctx
            while (!(ctx->flag & MPOOL_FLAG_READY))
                ;
            atomic_inc(&ctx->ref);
            return ctx;
        }
    }

    if (ctx == pool) {
        ctx->flag |= MPOOL_FLAG_ALLOC;
    }

    ctx->size  = size;
    ctx->msize = msize;
    ctx->max =
        (total_size - sizeof(mpool_ctx_t)) / (sizeof(void *) + ctx->size);
    ctx->head  = 0;
    ctx->tail  = ctx->max;
    ctx->queue = (mpool_item_t **)((uint8_t *)ctx + sizeof(mpool_ctx_t));
    ctx->pool  = (uint8_t *)ctx->queue + sizeof(void *) * ctx->max;

    int i = 0;
    for (; i < ctx->max; i++) {
        ctx->queue[i] = (mpool_item_t *)(ctx->pool + i * ctx->size);
    }

    ctx->flag |= MPOOL_FLAG_READY;
    return ctx;
}

/*
 * Initialize a memory by the number of member.
 * Argument
 *   @nmem:  how many memebers do the pool store.
 *   @msize: the size of a member
 * Return
 *   A pointer to mpool_ctx_t. NULL if init failed.
 */
mpool_ctx_t *
mpool_calloc(size_t nmem, size_t msize)
{
    if (msize <= 0 || nmem <= 0) {
        return NULL;
    }

    //aligned size of a member
    size_t size = (sizeof(mpool_item_t) + msize + ALIGN_SIZE - 1) / ALIGN_SIZE
                  * ALIGN_SIZE;

    size_t total_size =
        sizeof(mpool_ctx_t) + sizeof(void *) * nmem + size * nmem;
    return mpool_init(msize, total_size, NULL);
}

void
mpool_cleanup(mpool_ctx_t *ctx)
{
    if (ctx) {
        if (atomic_dec_and_test(&(ctx->ref))) {
            if (ctx->flag & MPOOL_FLAG_ALLOC) {
                free(ctx);
            } else {
                ctx->flag &= ~MPOOL_FLAG_READY;  //clear the ready flag
            }
        }
    }
}

static inline void
mpool_enqueue(mpool_ctx_t *ctx, mpool_item_t *ptr)
{
    while (1) {
#ifdef MPOOL_DBG
        if (mpool_empty(ctx)) {
            fprintf(stderr, "Try to put a package in to an empty pool!\n");
            loc_list_dump(ptr);
            return;
        }
#endif
        uint32_t idx = ctx->tail;
        if (cmpxchg(&ctx->tail, idx, idx + 1) == idx) {
            ctx->queue[idx % ctx->max] = ptr;
            return;
        }
    }
}

static inline mpool_item_t *
mpool_dequeue(mpool_ctx_t *ctx)
{

    while (1) {
        if (mpool_full(ctx)) {
            return NULL;
        }

        uint32_t idx      = ctx->head;
        mpool_item_t *ptr = NULL;
        if (cmpxchg(&ctx->head, idx, idx + 1) == idx) {
            ptr = ctx->queue[idx % ctx->max];
            return ptr;
        }
    }
}

void *
__mpool_get(mpool_ctx_t *ctx, const char *file, int line)
{
    mpool_item_t *ptr = NULL;
    void *ret         = NULL;

    if (!ctx) {
        return NULL;
    }

    ptr = mpool_dequeue(ctx);
    if (ptr) {
        if (atomic_cmpxchg(&ptr->ref, 0, 1) == 0) {
#ifdef MPOOL_DBG
            loc_list_free(ptr->get);
            loc_list_free(ptr->put);
            loc_t *loc = loc_init(file, line);
            ptr->get   = loc_list_insert(ptr->get, loc);
#endif
            ptr->ctx = ctx;
            memset(ptr->data, 0, ctx->msize);
            ret = (void *)ptr->data;
        } else {
#ifdef MPOOL_DBG
            fprintf(stderr, "Un-expected package in queue!\n");
            loc_list_dump(ptr);
#endif
        }
    }

    return ret;
}

void
__mpool_put(void *p, const char *file, int line)
{
#ifdef MPOOL_DBG
    mpool_item_t *ptr = container_of(p, mpool_item_t, data);
    loc_t *loc        = loc_init(file, line);
    ptr->put          = loc_list_insert(ptr->put, loc);
#endif

    mpool_ref_dec(p);

#ifdef MPOOL_DBG
    if (atomic_read(&ptr->ref) < 0) {
        fprintf(stderr, "ATTENTION: double free on %s:%d!\n", file, line);
        loc_list_dump(ptr);
    }
#endif
}

void
__mpool_ref_inc(void *p, const char *file, int line)
{
    if (p) {
        mpool_item_t *ptr = container_of(p, mpool_item_t, data);
        atomic_inc(&ptr->ref);
#ifdef MPOOL_DBG
        loc_t *loc = loc_init(file, line);
        ptr->get   = loc_list_insert(ptr->get, loc);
        if (atomic_read(&ptr->ref) <= 1) {
            fprintf(stderr, "ref inc on a free memory on %s:%d\n", file, line);
            loc_list_dump(ptr);
        }
#endif
    }
}

void
mpool_ref_dec(void *p)
{
    if (p) {
        mpool_item_t *ptr = container_of(p, mpool_item_t, data);
        if (atomic_dec_and_test(&(ptr->ref))) {
            mpool_enqueue(ptr->ctx, ptr);
#ifdef MPOOL_DBG
//            loc_list_dump(ptr);
#endif
        }
    }
}

int
mpool_get_idx(mpool_ctx_t *ctx, void *p)
{
    mpool_item_t *ptr = container_of(p, mpool_item_t, data);
    int64_t l         = (int64_t)((uint8_t *)ptr - ctx->pool);
    if (l < 0 || l % ctx->size != 0) {
        return -EINVAL;
    }
    int idx = (int)(l / ctx->size);
    if (idx >= ctx->max) {
        return -EINVAL;
    }
    return idx;
}

void *
mpool_get_by_idx(mpool_ctx_t *ctx, int idx)
{
    if (!ctx || idx < 0 || idx >= ctx->max) {
        return NULL;
    }
    mpool_item_t *ptr = (mpool_item_t *)(ctx->pool + idx * ctx->size);
    return ptr->data;
}
