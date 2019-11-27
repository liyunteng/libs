/*
 * mpool.c -- memory pool
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/06/06 00:44:00
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpool.h"

#define MAX_FILE_LEN 16

mpool_ctx_t *
mpool_init(size_t msize, size_t total_size, void *pool)
{
    mpool_ctx_t *ctx = NULL;
    size_t size = (sizeof(mpool_item_t) + msize + ALIGN_SIZE - 1) / ALIGN_SIZE
                  * ALIGN_SIZE;
    if (msize == 0
        || total_size < sizeof(mpool_ctx_t) + sizeof(void *) + size) {
        return NULL;
    }

    if (pool) {
        ctx = (mpool_ctx_t *)pool;
    } else {
        ctx = (mpool_ctx_t *)malloc(total_size);
        if (!ctx) {
            return NULL;
        }
        memset(ctx, 0, total_size);
    }

    if (ctx->flag & MPOOL_FLAG_READY) {
        atomic_inc(&ctx->ref);
        return ctx;
    } else {
        if (atomic_cmpxchg(&ctx->ref, 0, 1) != 0) {
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

    uint32_t i = 0;
    for (; i < ctx->max; i++) {
        ctx->queue[i] = (mpool_item_t *)(ctx->pool + i * ctx->size);
    }

    ctx->flag |= MPOOL_FLAG_READY;
    return ctx;
}

mpool_ctx_t *
mpool_callc(size_t nmem, size_t msize)
{
    if (msize <= 0 || nmem <= 0) {
        return NULL;
    }

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
        if (atomic_dec_and_test(&ctx->ref)) {
            if (ctx->flag & MPOOL_FLAG_ALLOC) {
                free(ctx);
            } else {
                ctx->flag &= ~MPOOL_FLAG_READY;
            }
        }
    }
}

static inline int
mpool_enqueue(mpool_ctx_t *ctx, mpool_item_t *ptr)
{
    while (1) {
        if (mpool_full(ctx)) {
            return -EAGAIN;
        }

        uint32_t idx = ctx->tail;
        if (cmpxchg(&ctx->tail, idx, idx + 1) == idx) {
            ctx->queue[idx % ctx->max] = ptr;
            return 0;
        }
    }
}

static inline mpool_item_t *
mpool_dequeue(mpool_ctx_t *ctx)
{
    while (1) {
        if (mpool_empty(ctx)) {
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
            ptr->ctx = ctx;
            memset(ptr->data, 0, ctx->msize);
            ret = (void *)ptr->data;
        } else {
        }
    }

    return ret;
}

void
__mpool_put(void *p, const char *file, int line)
{
    mpool_ref_dec(p);
}

inline void
__mpool_ref_inc(void *p, const char *file, int line)
{
    if (p) {
        mpool_item_t *ptr = container_of(p, mpool_item_t, data);
        atomic_inc(&ptr->ref);
    }
}

inline void
__mpool_ref_dec(void *p, const char *file, int line)
{
    if (p) {
        mpool_item_t *ptr = container_of(p, mpool_item_t, data);
        if (atomic_dec_and_test(&ptr->ref)) {
            mpool_enqueue(ptr->ctx, ptr);
        }
    }
}

int
mpool_get_idx(mpool_ctx_t *ctx, void *p)
{
    if (p == NULL || ctx == NULL) {
        return -EINVAL;
    }

    mpool_item_t *ptr = container_of(p, mpool_item_t, data);
    int64_t l         = (int64_t)((uint8_t *)ptr - ctx->pool);
    if (l < 0 || l % ctx->size != 0) {
        return -EINVAL;
    }

    int idx = (int)(l / ctx->size);
    if ((uint32_t)idx >= ctx->max) {
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
