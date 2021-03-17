/*
 * test.c - test
 *
 * Date   : 2021/03/17
 */

#include "macro.h"
#include "mpool.h"
#include "thread.h"
#include "system.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>


typedef struct {
    char str[120];
    int id;
} item_t;

int
mpool_test(void)
{
    mpool_ctx_t *ctx;
    item_t *p;
    item_t *queue[32] = {0};
    int i;
    ctx = mpool_calloc(16, sizeof(item_t));
    assert(ctx);

    printf(
        "================================GET================================"
        "\n");
    for (i = 0; i < 16; i++) {
        p = (item_t *)mpool_get(ctx);
        if (!p)
            break;
        queue[i] = p;
        printf("get idx: %d %p  size: %d count: %d head: %d tail: %d\n", i, p,
               mpool_size(ctx), mpool_count(ctx), ctx->head, ctx->tail);
    }

    printf(
        "================================PUT================================"
        "\n");
    for (i = 0; i < 16; i++) {
        p = queue[i];
        if (!p)
            break;
        p->id = i;
        mpool_put(p);
        printf("put idx: %d %p  size: %d count: %d head: %d tail: %d\n", i, p,
               mpool_size(ctx), mpool_count(ctx), ctx->head, ctx->tail);
    }

    mpool_put(p);
    for (i = 0; i < 16; i++) {
        p = mpool_get_by_idx(ctx, i);
        printf("%d\n", p->id);
    }


    mpool_cleanup(ctx);
    return 0;
}

int
mpool_benchmark(void)
{
    mpool_ctx_t *ctx = NULL;
    item_t *p        = NULL;
    int i, j;

    ctx = mpool_init(sizeof(item_t), 4 * 1024 * 1024, NULL);
    assert(ctx);

    printf("size: %d\n", mpool_size(ctx));
    printf("count: %d\n", mpool_count(ctx));
    for (j = 0; j < 200000; j++) {
        for (i = 0; i < mpool_size(ctx); i++) {
            p     = (item_t *)mpool_get(ctx);
            p->id = i;
            snprintf(p->str, sizeof(p->str), "This is %d", i);
            assert(p);
        }

        /* printf("get idx: %d %p  size: %d count: %d head: %d tail: %d\n", i,
         * p, mpool_size(ctx), mpool_count(ctx), ctx->head, ctx->tail); */

        for (i = 0; i < mpool_size(ctx); i++) {
            p = mpool_get_by_idx(ctx, i);
            assert(p);
            mpool_put(p);
        }
        /* printf("put idx: %d %p  size: %d count: %d head: %d tail: %d\n", i,
         * p, mpool_size(ctx), mpool_count(ctx), ctx->head, ctx->tail); */
    }

    printf("idx: %d %p  size: %d count: %d head: %d tail: %d\n", i, p,
           mpool_size(ctx), mpool_count(ctx), ctx->head, ctx->tail);
    mpool_cleanup(ctx);
    return 0;
}

#define N_THREAD 3

int run(void *arg)
{
    mpool_ctx_t *ctx = (mpool_ctx_t *)arg;
    int i;
    item_t *p = NULL;

    for (i = 0; i < 10000; i++) {
        p = mpool_get(ctx);
        if (p) {
            p->id = mpool_get_idx(ctx, p);
            snprintf(p->str, sizeof(p->str), "this is 0x%lx", pthread_self());
            mpool_put(p);
        }
    }
}

int
mpool_thread(void)
{
    pthread_t tids[N_THREAD];
    int i;
    mpool_ctx_t *ctx = NULL;

    ctx = mpool_calloc(32, sizeof(item_t));
    assert(ctx);


    for (i = 0; i < N_THREAD; i++) {
        thread_create(&tids[i], run, (void *)ctx);
    }

    for (i = 0; i < N_THREAD; i++) {
        thread_destroy(tids[i]);
    }

    for (i = 0; i < 32; i++) {
        item_t *p = mpool_get_by_idx(ctx, i);
        if (p) {
            printf("[%d] id: %d str: %s\n", i, p->id, p->str);
        }
    }
    printf("size: %d count: %d head: %d tail: %d\n",
           mpool_size(ctx), mpool_count(ctx), ctx->head, ctx->tail);
    mpool_cleanup(ctx);
    return 0;
}
int
main(int argc, char *argv[])
{
    /* mpool_test(); */
    /* mpool_benchmark(); */
    mpool_thread();
    return 0;
}
