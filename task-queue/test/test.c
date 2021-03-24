/*
 * test.c - test
 *
 * Date   : 2021/03/24
 */
#include "task-queue.h"
#include "thread.h"
#include "sema.h"
#include <assert.h>
#include <stdio.h>

#define N_TASK 1000

static sema_t g_sema;

static void task_test(void *param)
{
    int n = *(int*)param;
    sema_post(&g_sema);
    printf("[%d]\n", n);
}

void task_queue_test(void)
{
    int i;
    task_queue_t *taskQ;
    int array[N_TASK] = {0};

    sema_create(&g_sema, NULL, 0);
    taskQ = task_queue_create(3);
    assert(taskQ);
    for (i = 0; i < N_TASK; i++) {
        array[i] = i;
        task_queue_post(taskQ, task_test, &array[i]);
    }

    for (i = 0; i < N_TASK; i++) {
        sema_wait(&g_sema);
    }

    task_queue_destroy(taskQ);
    sema_destroy(&g_sema);
}

int main(void)
{
    task_queue_test();
    return 0;
}
