/*
 * test.c - test
 *
 * Date   : 2021/03/16
 */
#include "atomic.h"
#include <stdlib.h>
#include <pthread.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

void atomic_test(void)
{
	int32_t val32 = 100;
	int32_t *pval32 = &val32;
	int64_t *pval64;

#if defined(_WIN32)
	pval64 = _aligned_malloc(sizeof(int64_t), 8);
#else
	assert(posix_memalign((void**)&pval64, 8, sizeof(int64_t)) == 0);
#endif
	*pval64 = 0x0011001100110011LL;

	assert(100 == atomic_load32(&val32));
	assert(101 == atomic_increment32(&val32) && 101==val32);
	assert(100 == atomic_decrement32(&val32) && 100==val32);
	assert(200 == atomic_add32(&val32, 100) && 200 == val32);
	assert(100 == atomic_add32(&val32, -100) && 100 == val32);
	assert(0 != atomic_cas32(&val32, 100, 101) && 101 == val32);
	assert(0 == atomic_cas32(&val32, 100, 101) && 101 == val32);

#if !defined(_WIN32_WINNT ) || _WIN32_WINNT >= 0x0502
	assert(0x0011001100110011 == atomic_load64(pval64));
	assert(0x0011001100110012 == atomic_increment64(pval64) && 0x0011001100110012==*pval64);
	assert(0x0011001100110011 == atomic_decrement64(pval64) && 0x0011001100110011==*pval64);
	assert(0x0022002200220022 == atomic_add64(pval64, 0x0011001100110011) && 0x0022002200220022 == *pval64);
	assert(0x0011001100110011 == atomic_add64(pval64, -0x0011001100110011) && 0x0011001100110011 == *pval64);
	assert(1 == atomic_cas64(pval64, 0x0011001100110011, 0x0022002200220022) && 0x0022002200220022 == *pval64);
	assert(0 == atomic_cas64(pval64, 0x0011001100110011, 0x0022002200220022) && 0x0022002200220022 == *pval64);
#endif

	assert(1 == atomic_cas_ptr((void**)&pval32, &val32, pval64) && pval64 == (int64_t*)pval32);

#if defined(_WIN32)
	_aligned_free(val64);
#else
	free(pval64);
#endif
}


static int32_t s_v32 = 100;
static int64_t s_v64 = 0x0011001100110011;
static void * run(void *arg)
{
    int i;
    (void)arg;

    for (i = 0; i < 1000000; i++) {
        atomic_increment32(&s_v32);
        atomic_increment64(&s_v64);
        atomic_decrement32(&s_v32);
        atomic_decrement64(&s_v64);
    }
    return (void *)0;
}

#define N_THREAD 32
void atomic_test2(void)
{
    int i = 0;
    pthread_t threads[N_THREAD];

    for (i = 0; i < N_THREAD; i++) {
        assert(pthread_create(&threads[i], NULL, run, NULL) == 0);
    }

    for (i = 0; i < N_THREAD; i++) {
        pthread_join(threads[i], NULL);
    }

    assert(s_v32 == 100);
	assert(s_v64 == 0x0011001100110011);
}

int main(void)
{
    atomic_test();
    atomic_test2();
    return 0;
}
