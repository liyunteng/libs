/*
 * task-queue.h - task-queue
 *
 * Date   : 2021/03/24
 */
#ifndef __TASK_QUEUE_H__
#define __TASK_QUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct task_queue_ctx task_queue_t;
typedef void (*task_proc)(void *param);

task_queue_t *task_queue_create(int maxWorker);
void task_queue_destroy(task_queue_t *taskQ);
int task_queue_post(task_queue_t *taskQ, task_proc proc, void *param);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* __TASK_QUEUE_H__ */
