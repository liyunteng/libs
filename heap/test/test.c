/*
 * test.c - test
 *
 * Date   : 2021/03/14
 */
#include "heap.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#ifdef NDEBUG
#undef NDEBUG
#include <assert.h>
#endif
#define N 100
static int heap_test_compare(void* param, const void* p1, const void* p2)
{
	(void)param;
	return *(int*)p1 < *(int*)p2;
}

void heap_test(void)
{
	int i;
	int *n, *v;
	heap_t* heap;

	v = malloc(sizeof(int) * N);
	srand((unsigned int)time(NULL));

	heap = heap_create(heap_test_compare, NULL);
	for (i = 0; i < N; i++)
	{
		/* v[i] = rand(); */
        v[i] = i;
		heap_push(heap, &v[i]);
	}
	assert(heap_size(heap) == N);

	n = (int*)heap_top(heap); // first value
    printf("top: %d\n",*n);
	heap_pop(heap);
	for (i = 1; i < N; i++)
	{
		assert(*n <= *((int*)heap_top(heap)));
		*n = *(int*)heap_top(heap);
        printf("%d: %d ", i, *n);
		heap_pop(heap);
	}
	assert(heap_empty(heap));
	heap_destroy(heap);
	printf("heap test ok\n");
}


int main(void)
{
    heap_test();
    return 0;
}
