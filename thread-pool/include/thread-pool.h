/*
 * thread-pool.h - thread-pool
 *
 * Date   : 2021/03/24
 */
#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct thread_pool_ctx thread_pool_t;

///create thread pool
///@param[in] num initialize thread count
///@param[in] max maximum thread count
///@return 0-error, other-thread pool id
thread_pool_t *thread_pool_create(int num, int max);

///destroy thread pool
///@param[in] pool thread pool id
void thread_pool_destroy(thread_pool_t *pool);

///get thread count of the thread pool
///@param[in] pool thread pool id
///@return <0-error code, >=0-thread count
int thread_pool_threads_count(thread_pool_t *pool);

///thread pool procedure
///@param[in] param user parameter
typedef void (*thread_pool_proc)(void *param);

///push a task to thread pool
///@param[in] pool thread pool id
///@param[in] proc task procedure
///@param[in] param user parameter
///@return =0-ok, <0-error code
int thread_pool_push(thread_pool_t *pool, thread_pool_proc proc, void *param);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __THREAD_POOL_H__ */
