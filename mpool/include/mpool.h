/*
 * mpool.h -- memory pool
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/06/06 00:52:00
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

#ifndef MPOOL_H
#define MPOOL_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define MPOOL_DBG

#define MPOOL_FLAG_ALLOC 0x01U
#define MPOOL_FLAG_READY 0x02U

#define ALIGN_SIZE 8
typedef int32_t atomic_t;

typedef struct {
    void *ctx;
    atomic_t ref;
    //For debug
#ifdef MPOOL_DBG
    void *get;
    void *put;
#endif
    uint8_t data[0];
} mpool_item_t;

typedef volatile uint32_t vuint32_t;

typedef struct {
    size_t size;   //size of a mpool_item_t, align to 8 bytes
    size_t msize;  //size of a member
    uint32_t max;
    uint32_t flag;
    atomic_t ref;
    vuint32_t head;
    vuint32_t tail;
    mpool_item_t **queue;
    uint8_t *pool;
} mpool_ctx_t;

/*
 *  Init a cache in system memory
 *  Parameter: ctx         context of memory cache to init.
 *             msize       size of member.
 *             total_size  the size of total memory
 *             pool        a piece of memory to use. If NULL, mpool will alloc memory by itself.
 * Return: 0 on success, -1 on error.
 */
mpool_ctx_t *mpool_init(size_t msize, size_t total_size, void *pool);

/*
 * Initialize a memory by the number of member.
 * Argument
 *   @nmem:  how many memebers do the pool store.
 *   @msize: the size of a member
 * Return
 *   A pointer to mpool_ctx_t. NULL if init failed.
 */
mpool_ctx_t *mpool_calloc(size_t nmem, size_t msize);

/*
 *  Cleanup
 */
void mpool_cleanup(mpool_ctx_t *ctx);

void *__mpool_get(mpool_ctx_t *ctx, const char *, int);
void __mpool_put(void *ptr, const char *, int);

#define mpool_get(ctx) __mpool_get(ctx, __FILE__, __LINE__)
#define mpool_put(ptr) __mpool_put(ptr, __FILE__, __LINE__)

/*
 *  Increace / decrease ref count.
 */
void __mpool_ref_inc(void *ptr, const char *, int);
void mpool_ref_dec(void *ptr);

#define mpool_ref_inc(ptr) __mpool_ref_inc(ptr, __FILE__, __LINE__)

/*
 *  Get index for a piece of memory
 */
int mpool_get_idx(mpool_ctx_t *ctx, void *ptr);
void *mpool_get_by_idx(mpool_ctx_t *ctx, int idx);

/*
 * Test if mpool is empty
 */
static inline int
mpool_empty(mpool_ctx_t *ctx)
{
    return ctx->tail - ctx->head >= ctx->max;
}

static inline int
mpool_full(mpool_ctx_t *ctx)
{
    //mpool is full means queue is empty
    return ctx->tail == ctx->head;
}

static inline int
mpool_size(mpool_ctx_t *ctx)
{
    return ctx->max;
}

static inline int
mpool_count(mpool_ctx_t *ctx)
{
    return ctx->max + ctx->head - ctx->tail;
}

#ifdef __cplusplus
}
#endif
#endif
