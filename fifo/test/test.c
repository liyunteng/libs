/*
 * test.c - test
 *
 * Date   : 2021/03/18
 */
#include "fifo.h"
#include "thread.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

typedef struct {
    int id;
    char str[128];
} item_t;

int
fifo_test(void)
{
    int i;
    int n;
    item_t p;
    fifo_t fifo;

    assert(fifo_alloc(&fifo, 1024, sizeof(item_t)) == 0);
    printf("size: %u\n", fifo.mask+1);
    for (i = 0; i < 1024; i++) {
        p.id = i;
        snprintf(p.str, sizeof(p.str), "this is %d", i);
        assert(fifo_in(&fifo, &p, 1) == 1);
    }

    for (i = 0; i < 1024; i++) {
        assert(fifo_out(&fifo, &p, 1) == 1);
        printf("id: %d  str: %s\n", p.id, p.str);
    }

    fifo_free(&fifo);

    return 0;
}
int
produce(void *arg)
{
    fifo_t *fifo = (fifo_t *)arg;
    int i;
    item_t p;

    for (i = 0; i < 1024; i++) {
        p.id = i;
        snprintf(p.str, sizeof(p.str), "this is %d", i);
        assert(fifo_in(fifo, &p, 1) == 1);
    }

    return 0;
}

int
consume(void *arg)
{
    fifo_t *fifo = (fifo_t *)arg;
    int n, i;
    item_t p;

    i = 0;
    while (1) {
        if (i == 1024-1) {
            break;
        }
        n = fifo_out(fifo, &p, 1);
        if (n) {
            printf("id: %d  str: %s\n", p.id, p.str);
            i++;
        }
    }


    return 0;
}

int
fifo_thread(void)
{
    tid_t p[10];
    tid_t c[10];
    int i;
    fifo_t fifo;

    assert(fifo_alloc(&fifo, 1024, sizeof(item_t)) == 0);

    for (i = 0; i < 1; i++) {
        thread_create(&p[i], produce, (void *)&fifo);
    }
    for (i = 0; i < 1; i++) {
        thread_create(&c[i], consume, (void *)&fifo);
    }

    for (i = 0; i < 1; i++) {
        thread_destroy(p[i]);
    }
    for (i = 0; i < 1; i++) {
        thread_destroy(c[i]);
    }

    fifo_free(&fifo);
    return 0;
}

int
main(void)
{
    fifo_test();
    fifo_thread();
    printf("%d\n", rounddown_pow_of_two(1023));
    printf("%d\n", rounddown_pow_of_two(1025));
    printf("%d\n", rounddown_pow_of_two(1024));
    return 0;
}
