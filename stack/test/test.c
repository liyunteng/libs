/*
 * test.c - test
 *
 * Date   : 2021/03/19
 */

#include "stack.h"
#include "thread.h"
#include "macro.h"
#include "system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#define N 10
typedef struct {
    struct stack_node  node;
    int id;
    char str[128];
} item_t;

int produce(void *arg)
{
    struct stack *stack = (struct stack *)arg;
    int i;
    item_t *p = NULL;
    for (i = 0; i < 100; i++) {
        p = (item_t *) malloc(sizeof(item_t));
        assert(p);
        p->id = i;
        snprintf(p->str, sizeof(p->str)-1, "this is %d from %lx", i,
                 (long)thread_self());
        stack_push(stack, &p->node);
    }

    return 0;
}

int consume(void *arg)
{
    struct stack *stack = (struct stack *)arg;
    int i;
    item_t *p = NULL;
    struct stack_node *node = NULL;
    int zero_count = 0, n_count = 0, total_count = 0;

    system_sleep(10);
    while (!stack_empty(stack)) {
        node = stack_top(stack);
        assert(node);
        p = stack_entry(node, item_t, node);
        assert(p);
        printf("id: %d  str: %s\n", p->id, p->str);
        if (p->id == 0) {
            zero_count ++;
        } else if (p->id == N-1) {
            n_count ++;
        }
        total_count ++;
        stack_pop(stack);
    }

    assert(zero_count == N);
    assert(n_count == N);
    assert(total_count == N * 100);
    return 0;
}

int stack_test(void)
{
    tid_t produces[N];
    tid_t consumes[1];
    size_t i;
    struct stack stack;

    stack_init(&stack);

    for (i = 0; i < ARRAY_SIZE(produces); i++) {
        thread_create(&produces[i], produce, (void *)&stack);
    }
    for (i = 0; i < ARRAY_SIZE(consumes); i++) {
        thread_create(&consumes[i], consume, (void *)&stack);
    }

    for (i = 0; i < ARRAY_SIZE(produces); i++) {
        thread_destroy(produces[i]);
    }
    for (i = 0; i < ARRAY_SIZE(consumes); i++) {
        thread_destroy(consumes[i]);
    }

    return 0;
}


int main(void)
{
    stack_test();
    return 0;
}
