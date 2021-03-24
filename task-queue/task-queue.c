/*
 * task-queue.c - task-queue
 *
 * Date   : 2021/03/24
 */

#include "task-queue.h"
#include "thread-pool.h"
#include "list.h"
#include "atomic.h"
#include "thread.h"
#include "system.h"
#include "locker.h"
#include "sema.h"
#include <errno.h>

enum {
    PRIORITY_IDLE = 0,
    PRIORITY_LOWEST,
    PRIORITY_NORMAL,
    PRIORITY_CRITICAL
};

struct task_queue_ctx {
    int32_t ref;
    int running;
    int maxWorker;
    thread_pool_t *pool;

    pthread_t thread_scheduler;
    sema_t sema_worker;
    sema_t sema_request;

    locker_t locker;
    struct list_head tasks;
    struct list_head tasks_recycle;
    size_t tasks_count;
    size_t tasks_recycle_count;
};

struct task_ctx {
    struct list_head head;

    struct task_queue_ctx *taskQ;
    int timeout;
    task_proc proc;
    void *param;

    uint64_t stime;             /* start time */
    uint64_t etime;             /* end time */
    tid_t thread;
    int priority;
};

static struct task_ctx *task_alloc(struct task_queue_ctx *taskQ)
{
    struct task_ctx *task;
    if (list_empty(&taskQ->tasks_recycle)) {
        task = (struct task_ctx *) malloc(sizeof(struct task_ctx));
    } else {
        task = list_first_entry(&taskQ->tasks_recycle, struct task_ctx, head);
        list_del(&task->head);
        assert(taskQ->tasks_recycle_count > 0);
        -- taskQ->tasks_recycle_count;
    }

    if (task) {
        memset(task, 0, sizeof(struct task_ctx));
        INIT_LIST_HEAD(&task->head);
    }

    return task;
}

static void task_recycle(struct task_queue_ctx *taskQ, struct task_ctx * task)
{
    // max recycle count
    if (taskQ->tasks_recycle_count > 500) {
        free(task);
        return;
    }

    list_add_tail(&task->head, &taskQ->tasks_recycle);
    assert(taskQ->tasks_recycle_count >= 0);
    ++taskQ->tasks_recycle_count;
}

static void task_clean(struct task_queue_ctx *taskQ)
{
    struct task_ctx *task, *tmp;
    list_for_each_entry_safe(task, tmp, &taskQ->tasks_recycle, head) {
        free(task);
    }

    list_for_each_entry_safe(task, tmp, &taskQ->tasks, head) {
        free(task);
    }
}

static void task_push(struct task_queue_ctx *taskQ, struct task_ctx *task)
{
    list_add_tail(&task->head, &taskQ->tasks);
    assert(taskQ->tasks_count >= 0);
    ++taskQ->tasks_count;
}

static struct task_ctx *task_pop(struct task_queue_ctx *taskQ)
{
    struct task_ctx *task = NULL;
    if (list_empty(&taskQ->tasks))
        return NULL;

    assert(taskQ->tasks_count > 0);
    -- taskQ->tasks_count;
    task = list_first_entry(&taskQ->tasks, struct task_ctx, head);
    list_del(&task->head);

    return task;
}

static struct task_ctx *task_pop_timeout(struct task_queue_ctx *taskQ)
{
    uint64_t clock;
    struct task_ctx *task, *tmp;
    struct list_head *p, *n;

    clock = system_clock();
    list_for_each_entry_safe(task, tmp, &taskQ->tasks, head) {
        if (clock - task->stime > (size_t)task->timeout) {
            list_del(&task->head);
            return task;
        }
    }

    return NULL;
}

static void task_queue_release(struct task_queue_ctx *taskQ)
{
    if (0 == atomic_decrement32(&taskQ->ref)) {
        sema_destroy(&taskQ->sema_request);
        sema_destroy(&taskQ->sema_worker);
        locker_destroy(&taskQ->locker);

        task_clean(taskQ);
        free(taskQ);
    }
}

static void task_action(void *param)
{
    struct task_ctx *task;
    struct task_queue_ctx *taskQ;

    assert(param);
    task = (struct task_ctx *)param;
    taskQ = task->taskQ;
    assert(taskQ);

    if (task->proc) {
        task->proc(task->param);
    }

    task->etime = system_clock();
    task->thread = thread_getid(thread_self());
    locker_lock(&taskQ->locker);
    task_recycle(taskQ, task); // recycle task
    locker_unlock(&taskQ->locker);

    sema_post(&taskQ->sema_worker); // add worker
    task_queue_release(taskQ);
}

static int STDCALL task_queue_scheduler(void *param)
{
    int r;
    struct task_ctx *task;
    struct task_queue_ctx *taskQ;

    assert(param);
    taskQ = (struct task_queue_ctx *)param;
    assert(taskQ);
    while (taskQ->running) {
        r = sema_wait(&taskQ->sema_request);
        r = sema_wait(&taskQ->sema_worker);

        if (0 == r && taskQ->running) {
            locker_lock(&taskQ->locker);
            task = task_pop(taskQ);
            locker_unlock(&taskQ->locker);

            assert(task);
            if (task) {
                atomic_increment32(&taskQ->ref);
                r = thread_pool_push(taskQ->pool, task_action, task);
                assert(r == 0);
            }
        }
    }

    return 0;
}

int task_queue_post(struct task_queue_ctx *taskQ, task_proc proc, void *param)
{
    struct task_ctx *task;

    assert(taskQ);
    locker_lock(&taskQ->locker);
    task = task_alloc(taskQ);
    locker_unlock(&taskQ->locker);
    if (!task)
        return -ENOMEM;

    task->taskQ = taskQ;
    task->stime = system_clock();
    task->timeout = 5000;
    task->priority = PRIORITY_IDLE;
    task->proc = proc;
    task->param = param;
    locker_lock(&taskQ->locker);
    task_push(taskQ, task);
    locker_unlock(&taskQ->locker);

    return sema_post(&taskQ->sema_request);
}

struct task_queue_ctx *task_queue_create(int maxWorker)
{
    int r;
    struct task_queue_ctx *taskQ;

    taskQ = (struct task_queue_ctx *)malloc(sizeof(struct task_queue_ctx));
    if (!taskQ) {
        return NULL;
    }

    taskQ->pool = thread_pool_create(maxWorker, maxWorker);
    assert(taskQ->pool);

    taskQ->ref = 1;
    taskQ->running = 1;
    taskQ->maxWorker = maxWorker;
    taskQ->tasks_recycle_count = 0;
    INIT_LIST_HEAD(&taskQ->tasks);
    INIT_LIST_HEAD(&taskQ->tasks_recycle);

    r = locker_create(&taskQ->locker);
    assert(r == 0);

    // create worker semaphore
    r = sema_create(&taskQ->sema_worker, NULL, maxWorker);
    assert(r == 0);

    // create request semaphore
    r = sema_create(&taskQ->sema_request, NULL, 0);
    assert(r == 0);

    // create schedule thread
    r = thread_create(&taskQ->thread_scheduler, task_queue_scheduler, taskQ);
    assert(r == 0);

    return taskQ;
}

void task_queue_destroy(struct task_queue_ctx *taskQ)
{
    assert(taskQ);
    taskQ->running = 0;
    sema_post(&taskQ->sema_request);
    sema_post(&taskQ->sema_worker);
    thread_destroy(taskQ->thread_scheduler);
    thread_pool_destroy(taskQ->pool);

    task_queue_release(taskQ);

}
