/*
 * thread-pool.c - thread-pool
 *
 * Date   : 2021/03/24
 */
#include "thread-pool.h"
#include "locker.h"
#include "system.h"
#include "thread.h"
#include "event.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct thread_pool_ctx;
struct thread_list {
    struct thread_list *next;
    struct thread_pool_ctx *pool;
    pthread_t thread;
};

struct thread_task_list {
    struct thread_task_list *next;
    thread_pool_proc proc;
    void *param;
};

struct thread_pool_ctx {
    int run;
    int idle_max;

    int thread_count;
    int thread_count_max;
    int thread_count_idle;

    int task_count;
    struct thread_task_list *tasks;
    struct thread_task_list *recycle_tasks;

    struct thread_list *task_threads;

    locker_t locker;
    event_t event;
};

static void thread_pool_destroy_thread(struct thread_pool_ctx *ctx);

static int STDCALL thread_pool_worker(void *param)
{
    struct thread_list *threads;
    struct thread_task_list *task;
    struct thread_pool_ctx *ctx;

    threads = (struct thread_list *)param;
    assert(threads);
    ctx = threads->pool;
    assert(ctx);

    locker_lock(&ctx->locker);
    while (ctx->run) {
        // pop task
        task = ctx->tasks;
        while (task && ctx->run) {
            // remove task from task list
            ctx->tasks = task->next;
            -- ctx->task_count;

            // do task procedure
            --ctx->thread_count_idle;
            locker_unlock(&ctx->locker);
            task->proc(task->param);
            locker_lock(&ctx->locker);
            ++ctx->thread_count_idle;

            // recycle task: push task to recycle list
            task->next = ctx->recycle_tasks;
            ctx->recycle_tasks = task;

            // do next
            task = ctx->tasks;
        }

        // delete idle thread
        if (ctx->thread_count_idle > ctx->idle_max || !ctx->run)
            break;

        // wait for task
        locker_unlock(&ctx->locker);
        event_timewait(&ctx->event, 60*1000);
        locker_lock(&ctx->locker);
    }

    --ctx->thread_count;
    --ctx->thread_count_idle;
    thread_pool_destroy_thread(ctx);
    locker_unlock(&ctx->locker);

    return 0;
}

static struct thread_list *thread_pool_create_thread(struct thread_pool_ctx *ctx)
{
    struct thread_list *threads;

    assert(ctx);
    threads = (struct thread_list*)malloc(sizeof(struct thread_list));
    if (!threads)
        return NULL;

    memset(threads, 0, sizeof(struct thread_list));
    threads->pool = ctx;

    if (0 != thread_create(&threads->thread, thread_pool_worker, threads)) {
        free(threads);
        return NULL;
    }

    return threads;
}

static void thread_pool_destroy_thread(struct thread_pool_ctx *ctx)
{
    struct thread_list **head;
    struct thread_list *next;

    assert(ctx);
    head = &ctx->task_threads;
    while (*head) {
        if (thread_isself((*head)->thread)) {
            next = *head;
            *head = (*head)->next;
            free(next);
            break;
        }
        head = &(*head)->next;
    }
}

static void thread_pool_create_threads(struct thread_pool_ctx *ctx, int num)
{
    int i;
    struct thread_list *threads;

    assert(ctx);
    for (i = 0; i < num; i++) {
        threads = thread_pool_create_thread(ctx);
        if (!threads)
            break;

        // add to list head
        threads->next = ctx->task_threads;
        ctx->task_threads = threads;
    }

    ctx->thread_count += i;
    ctx->thread_count_idle += i;
}


static void thread_pool_destroy_threads(struct thread_list *threads)
{
    struct thread_list *next;

    while(threads) {
        next = threads->next;
        thread_destroy(threads->thread);
        free(threads);
        threads = next;
    }
}

static struct thread_task_list *thread_pool_create_task(struct thread_pool_ctx *ctx,
                                                        thread_pool_proc proc,
                                                        void *param)
{
    struct thread_task_list *task;

    assert(ctx);
    if (ctx->recycle_tasks) {
        task = ctx->recycle_tasks;
        ctx->recycle_tasks = ctx->recycle_tasks->next;
    } else {
        task = (struct thread_task_list *)malloc(sizeof(struct thread_task_list));
    }

    if (!task)
        return NULL;

    memset(task, 0, sizeof(struct thread_task_list));
    task->param = param;
    task->proc = proc;

    return task;
}

static int thread_pool_destroy_tasks(struct thread_task_list *tasks)
{
    struct thread_task_list *next;
    int count = 0;

    while(tasks) {
        next = tasks->next;
        free(tasks);
        tasks = next;
        count ++;
    }
    return count;
}

struct thread_pool_ctx *thread_pool_create(int num, int max)
{
    struct thread_pool_ctx *ctx;

    ctx = (struct thread_pool_ctx *)malloc(sizeof(struct thread_pool_ctx));
    if (!ctx)
        return NULL;

    memset(ctx, 0, sizeof(struct thread_pool_ctx));
    ctx->thread_count_max = max;
    ctx->idle_max = num;
    ctx->run = 1;

    if (0 != locker_create(&ctx->locker)) {
        free(ctx);
        return NULL;
    }

    if (0 != event_create(&ctx->event)) {
        locker_destroy(&ctx->locker);
        free(ctx);
        return NULL;
    }

    thread_pool_create_threads(ctx, num);

    return ctx;
}

void thread_pool_destroy(struct thread_pool_ctx *ctx)
{
    int count;

    assert(ctx);

    ctx->run = 0;
    locker_lock(&ctx->locker);
    while (ctx->thread_count) {
        event_signal(&ctx->event);
        locker_unlock(&ctx->locker);
        system_sleep(10);
        locker_lock(&ctx->locker);
    }
    locker_unlock(&ctx->locker);

    /* have destroyed by thread call thread_pool_destroy_thread */
    /* thread_pool_destroy_threads(ctx->task_threads); */

    count = thread_pool_destroy_tasks(ctx->recycle_tasks);
    /* printf("destroy %d task cache\n", count); */
    count = thread_pool_destroy_tasks(ctx->tasks);
    /* printf("destroy %d task\n", count); */

    event_destroy(&ctx->event);
    locker_destroy(&ctx->locker);

    free(ctx);
}

int thread_pool_threads_count(struct thread_pool_ctx *ctx)
{
    assert(ctx);
	return ctx->thread_count;
}

int thread_pool_push(struct thread_pool_ctx *ctx, thread_pool_proc proc, void *param)
{
    struct thread_task_list *task;

    assert(ctx);

    locker_lock(&ctx->locker);
    task = thread_pool_create_task(ctx, proc, param);
    if (!task) {
        locker_unlock(&ctx->locker);
        return -1;
    }

    // add to task list
    task->next = ctx->tasks;
    ctx->tasks = task;
    ++ctx->task_count;

    // add new thread to do task
    if (ctx->thread_count_idle < 1 &&
        ctx->thread_count < ctx->thread_count_max) {
        thread_pool_create_threads(ctx, 1);
    }

    event_signal(&ctx->event);
    locker_unlock(&ctx->locker);

    return 0;
}
