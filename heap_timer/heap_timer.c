/*
 * heap_timer.c - heap_timer
 *
 * Date   : 2021/03/23
 */
#include "heap_timer.h"
#include "system.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>

struct timer_event {
    uint64_t clock;
    timer_cb_t cb;
    void *data;

    uint32_t delay;
    uint32_t interval;
    uint32_t count;
    uint32_t enable;
};

struct heap_timer {
    uint32_t nevts;
    uint32_t max_evts;
    struct timer_event **heap;
};

static void
heap_insert(struct timer_event **heap, uint32_t idx)
{
    assert(heap && *heap);

    if (idx <= 0) {
        return;
    }

    uint32_t p = (idx - 1) >> 1;
    if (heap[p]->clock <= heap[idx]->clock) {
        return;
    } else {
        struct timer_event *tmp = heap[p];
        heap[p]                 = heap[idx];
        heap[idx]               = tmp;
        heap_insert(heap, p);
    }
}

static void
heap_delete(struct timer_event **heap, uint32_t nevts, uint32_t idx)
{
    assert(heap && *heap);
    if (idx >= nevts) {
        return;
    }

    uint32_t lchild = idx * 2 + 1;
    uint32_t rchild = idx * 2 + 2;
    uint32_t i      = idx;

    if (lchild < nevts && (heap[i]->clock > heap[lchild]->clock)) {
        i = lchild;
    }
    if (rchild < nevts && (heap[i]->clock > heap[rchild]->clock)) {
        i = rchild;
    }
    if (i != idx) {
        struct timer_event *tmp = heap[i];
        heap[i]                 = heap[idx];
        heap[idx]               = tmp;
        heap_delete(heap, nevts, i);
    }
}

static void
event_heap_push(struct heap_timer *ctx, struct timer_event *ev)
{
    assert(ctx);
    assert(ev);

    struct timer_event **heap = ctx->heap;
    uint32_t idx              = ctx->nevts++;
    heap[idx]                 = ev;
    assert(idx < ctx->max_evts);
    heap_insert(heap, idx);
}

static struct timer_event *
event_heap_pop(struct heap_timer *ctx)
{
    assert(ctx);
    struct timer_event *ev = ctx->heap[0];
    ctx->heap[0]           = ctx->heap[--ctx->nevts];
    assert(ctx->nevts < ctx->max_evts);
    heap_delete(ctx->heap, ctx->nevts, 0);
    return ev;
}

static void
event_re_enqueue(struct heap_timer *ctx, struct timer_event *ev)
{
    assert(ctx);
    assert(ev);
    if (ev->enable && ev->interval > 0) {
        ev->clock += ev->interval;
        if (ev->count > 0) {
            ev->count--;
            if (ev->count == 0) {
                ev->interval = 0;
            }
        }
        event_heap_push(ctx, ev);
    } else {
        /* free(ev); */
        /* ev = NULL; */
    }
}

struct timer_event *
timer_event_register(struct heap_timer *ctx, timer_cb_t cb, void *data, uint32_t delay, uint32_t interval,
                   uint32_t count)
{
    assert(ctx);
    struct timer_event *ev = NULL;

    if (ctx->nevts >= ctx->max_evts) {
        goto failed;
    }

    ev = (struct timer_event *)calloc(1, sizeof(struct timer_event));
    if (!ev) {
        goto failed;
    }

    ev->cb       = cb;
    ev->data     = data;
    ev->delay    = delay;
    ev->interval = interval;
    ev->count    = count;
    ev->clock    = system_clock() + ev->delay;
    ev->enable = 1;

    event_heap_push(ctx, ev);
    return ev;

failed:
    if (ev) {
        free(ev);
        ev = NULL;
    }
    return NULL;
}

void
timer_event_cancel(struct heap_timer *ctx, struct timer_event *ev)
{
    assert(ctx);
    assert(ev);
    ev->enable = 0;
}

void
heap_timer_run(struct heap_timer *ctx)
{
    assert(ctx);
    while (ctx->nevts > 0 && ctx->heap[0]->clock <= system_clock()) {
        struct timer_event *ev = event_heap_pop(ctx);
        if (ev->enable && ev->cb) {
            ev->cb(ev->data);
        }
        event_re_enqueue(ctx, ev);
    }
}

struct heap_timer *
heap_timer_create(int max_evts)
{
    struct heap_timer *ctx = NULL;

    ctx = (struct heap_timer *)calloc(1, sizeof(struct heap_timer));
    if (!ctx) {
        goto failed;
    }
    ctx->heap =
        (struct timer_event **)calloc(max_evts, sizeof(struct timer_event *));
    if (!ctx->heap) {
        goto failed;
    }

    ctx->max_evts = max_evts;
    ctx->nevts    = 0;

    return ctx;

failed:
    if (ctx) {
        if (ctx->heap) {
            free(ctx->heap);
            ctx->heap = NULL;
        }
        free(ctx);
        ctx = NULL;
    }

    return NULL;
}

void
heap_timer_destroy(struct heap_timer *ctx)
{
    if (ctx) {
        if (ctx->heap) {
            free(ctx->heap);
            ctx->heap = NULL;
        }
        free(ctx);
        ctx = NULL;
    }
}
