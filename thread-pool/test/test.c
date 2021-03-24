/*
 * test.c - test
 *
 * Date   : 2021/03/24
 */
#include "atomic.h"
#include "system.h"
#include "thread-pool.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

static int32_t total = 0;

static void
worker(void *param)
{
    int i = 0;
    int n = (int)param + 1;

    printf("[%d] start\n", n);

    while (i++ < n) {
        system_sleep(100);
        if (210 == atomic_increment32(&total)) {
            printf("[%d] I'm the KING\n", n);
            assert(i == n);
            break;
        }
    }

    printf("[%d] done total: %u\n", n, atomic_load32(&total));
}

void
thread_pool_test(void)
{
    int i, r;
    thread_pool_t *pool;

    pool = thread_pool_create(4, 10);
    assert(pool);

    for (i = 0; i < 20; i++) {
        system_sleep(10); // wait for thread start
        r = thread_pool_push(pool, worker, (void *)i);
        assert(r == 0);
    }

    while (atomic_load32(&total) != 210) {
        printf("current threads: %d\n", thread_pool_threads_count(pool));
        system_sleep(10);
    }
    printf("current threads: %d\n", thread_pool_threads_count(pool));
    thread_pool_destroy(pool);
}

int
main(void)
{
    thread_pool_test();
    return 0;
}
