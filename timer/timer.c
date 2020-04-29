/*
 * timer.c - timer
 *
 * Date   : 2020/04/30
 */
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mpool.h"
#include "timer.h"

inline uint64_t
get_current_clock(void)
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (uint64_t)tp.tv_sec * 1e3 + (uint64_t)(tp.tv_nsec / 1e6);
}

static void
heap_insert_helper(timer_event_t *heap[], uint32_t idx)
{
    if (idx <= 0) {
        return;
    }
    uint32_t p = (idx - 1) >> 1;
    if (heap[p]->time <= heap[idx]->time) {
        return;
    } else {
        timer_event_t *tmp = heap[p];
        heap[p]            = heap[idx];
        heap[idx]          = tmp;
        heap_insert_helper(heap, p);
    }
}

static void
event_heap_insert(timer_ctx_t *ctx, timer_event_t *evt)
{
    timer_event_t **heap = ctx->heap;
    uint32_t idx         = ctx->nevts++;
    heap[idx]            = evt;
    heap_insert_helper(heap, idx);
}

static void
heap_del_helper(timer_event_t *heap[], uint32_t nevts, uint32_t idx)
{
    if (idx >= nevts) {
        return;
    }

    uint32_t lchild = idx * 2 + 1;
    uint32_t rchild = idx * 2 + 2;
    uint32_t i      = idx;

    if (lchild < nevts && (heap[i]->time > heap[lchild]->time)) {
        i = lchild;
    }

    if (rchild < nevts && (heap[i]->time > heap[rchild]->time)) {
        i = rchild;
    }

    if (i != idx) {
        timer_event_t *tmp = heap[i];
        heap[i]            = heap[idx];
        heap[idx]          = tmp;
        heap_del_helper(heap, nevts, i);
    }
}

static timer_event_t *
event_heap_pop(timer_ctx_t *ctx)
{
    timer_event_t *evt = ctx->heap[0];
    ctx->heap[0]       = ctx->heap[--ctx->nevts];
    heap_del_helper(ctx->heap, ctx->nevts, 0);
    return evt;
}

static void
event_re_enqueue(timer_ctx_t *ctx, timer_event_t *evt)
{
    if (evt->enable && evt->interval > 0) {
        evt->time += evt->interval;
        if (evt->count > 0) {
            evt->count--;
            if (evt->count == 0) {
                evt->interval = 0;
            }
        }
        event_heap_insert(ctx, evt);
    } else {
        mpool_put(evt);
    }
}

void
timer_run(timer_ctx_t *ctx)
{
    while (ctx->nevts > 0 && ctx->heap[0]->time <= get_current_clock()) {
        timer_event_t *evt = event_heap_pop(ctx);
        if (evt->enable && evt->cb) {
            evt->cb(evt->data);
        }
        event_re_enqueue(ctx, evt);
    }
}

timer_ctx_t *
timer_init(timer_ctx_t *ctx)
{
    if (!ctx) {
        return NULL;
    }

    memset(ctx, 0, sizeof(timer_ctx_t));

    //make sure mpool is large enough
    ctx->mpool = mpool_calloc(MAX_EVENT_COUNT, sizeof(timer_event_t));
    //    ctx->mpool = mpool_init(sizeof(timer_event_t),
    //            (sizeof(timer_event_t) + sizeof(void*)) * MAX_EVENT_COUNT + 512,
    //            NULL);

    if (!ctx->mpool) {
        goto error;
    }

    return ctx;

error:
    mpool_cleanup(ctx->mpool);
    return NULL;
}

void
timer_cleanup(timer_ctx_t *ctx)
{

    if (!ctx) {
        return;
    }
    mpool_cleanup(ctx->mpool);
    ctx->mpool = NULL;
}

int
timer_register(timer_ctx_t *ctx, timer_cb_t cb, void *data, uint32_t delay,
               uint32_t interval, uint32_t count)
{
    timer_event_t *evt = NULL;
    if (ctx->nevts >= MAX_EVENT_COUNT) {
        goto err;
    }

    evt = (timer_event_t *)mpool_get(ctx->mpool);
    if (!evt) {
        goto err;
    }
    memset(evt, 0, sizeof(timer_event_t));

    int idx = mpool_get_idx(ctx->mpool, evt);
    if (idx < 0) {
        goto err;
    }

    evt->id       = idx;
    evt->time     = get_current_clock() + delay;
    evt->cb       = cb;
    evt->data     = data;
    evt->interval = interval;
    evt->count    = count;
    evt->enable   = 1;

    event_heap_insert(ctx, evt);

    return idx;

err:
    if (evt) {
        mpool_put(evt);
    }
    return -1;
}

void
timer_cancel(timer_ctx_t *ctx, int id)
{
    timer_event_t *evt = mpool_get_by_idx(ctx->mpool, id);
    if (evt) {
        evt->enable = 0;
    }
}
