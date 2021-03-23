/*
 * test.c - test
 *
 * Date   : 2021/03/23
 */
#include "heap_timer.h"
#include "system.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#define N 100

void ontime(void *param)
{
    uint64_t *pc = (uint64_t *)param;
    printf("counter: %"PRIu64"  clock: %"PRIu64"\n", *pc, system_clock());
    *pc += 1;
}

void heap_test1()
{
    int i;
    uint64_t counter = 0;
    uint64_t now;
    heap_timer_t *ctx = NULL;
    timer_event_t **ev = NULL;


    ev = (timer_event_t **)calloc(N, sizeof(timer_event_t*));
    assert(ev);
    now = system_clock();
    ctx = heap_timer_create(N);
    assert(ctx);

    for (i = 0; i < N; i++) {
        ev[i] = timer_event_register(ctx, ontime, &counter, i, 0, 1);
        assert(ev[i]);
    }

    for (i = 0; i < N; i++) {
        heap_timer_run(ctx);
        system_sleep(1);
    }

    for (i = 0; i < N; i++) {
        free(ev[i]);
        ev[i] = NULL;
    }
    free(ev);

    heap_timer_destroy(ctx);
}

void heap_test2()
{
    int i;
    uint64_t counter = 0;
    uint64_t now;
    heap_timer_t *ctx = NULL;
    timer_event_t *ev = NULL;

    now = system_clock();
    ctx = heap_timer_create(N);
    assert(ctx);


    ev = timer_event_register(ctx, ontime, &counter, 10, 10, 0);
    assert(ev);

    printf("begin: %"PRIu64"\n", system_clock());
    system_sleep(N);
    heap_timer_run(ctx);
    printf("end: %"PRIu64"\n", system_clock());


    free(ev);
    heap_timer_destroy(ctx);
}

void heap_test3()
{
    int i;
    uint64_t counter = 0;
    uint64_t now;
    heap_timer_t *ctx = NULL;
    timer_event_t **ev = NULL;

    now = system_clock();
    ctx = heap_timer_create(N);
    assert(ctx);

    ev = (timer_event_t **)calloc(N, sizeof(timer_event_t*));
    assert(ev);
    for (i = 0; i < N; i++) {
        ev[i] = timer_event_register(ctx, ontime, &counter, i, i, i);
        assert(ev[i]);
    }

    for (i = 0; i < N / 2; i++) {
        timer_event_cancel(ctx, ev[i]);
    }

    printf("begin: %"PRIu64"\n", system_clock());
    system_sleep(N);
    heap_timer_run(ctx);
    printf("end: %"PRIu64"\n", system_clock());

    for (i = 0; i < N; i++) {
        free(ev[i]);
        ev[i] = NULL;
    }
    free(ev);
    heap_timer_destroy(ctx);
}

int main(void)
{
    heap_test1();
    heap_test2();
    heap_test3();
    return 0;
}
