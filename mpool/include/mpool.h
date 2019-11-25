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

#include "atomic_x86_64.h"
#include "types.h"
#include <stdint.h>

#define MPOOL_FLAG_ALLOC 0x01U
#define MPOOL_FLAG_READY 0x02U

#define ALIGN_SIZE 8

typedef volatile uint32_t vuint32_t;

typedef struct mpool_item_s mpool_item_t;
typedef struct mpool_ctx_s  mpool_ctx_t;
struct mpool_ctx_s {
    size_t         size;  /* mpool_item_t size */
    size_t         msize; /* mpool_item_t.data size */
    uint32_t       max;
    uint32_t       flag;
    atomic_t       ref;
    vuint32_t      head;
    vuint32_t      tail;
    mpool_item_t **queue;
    uint8_t *      pool;
};

struct mpool_item_s {
    mpool_ctx_t *ctx;
    atomic_t     ref;
    uint8_t      data[0];
};

mpool_ctx_t *mpool_init(size_t size, size_t total_size, void *pool);
mpool_ctx_t *mpool_callc(size_t nmem, size_t msize);
void         mpool_cleanup(mpool_ctx_t *ctx);

void *__mpool_get(mpool_ctx_t *ctx, const char *file, int line);
void  __mpool_put(void *ptr, const char *file, int line);

#define mpool_get(ctx) __mpool_get(ctx, __FILE__, __LINE__)
#define mpool_put(ptr) __mpool_put(ptr, __FILE__, __LINE__)

inline void __mpool_ref_inc(void *ptr, const char *file, int line);
inline void __mpool_ref_dec(void *ptr, const char *file, int line);

#define mpool_ref_inc(ptr) __mpool_ref_inc(ptr, __FILE__, __LINE__)
#define mpool_ref_dec(ptr) __mpool_ref_dec(ptr, __FILE__, __LINE__)

int   mpool_get_idx(mpool_ctx_t *ctx, void *ptr);
void *mpool_get_by_idx(mpool_ctx_t *ctx, int idx);

static inline int
mpool_empty(mpool_ctx_t *ctx)
{
    return ctx->tail == ctx->head;
}

static inline int
mpool_full(mpool_ctx_t *ctx)
{
    return ctx->tail - ctx->head >= ctx->max;
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
