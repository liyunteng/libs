/*
 * heap.c - heap
 *
 * Date   : 2021/03/14
 */

// https://en.wikipedia.org/wiki/Heap_(data_structure)
// https://en.wikipedia.org/wiki/Binary_heap
// https://en.wikipedia.org/wiki/Fibonacci_heap

#include "heap.h"
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

struct heap
{
	void** elts; // elements
	int size;
	int capacity;

	heap_less less;
	void* param;
};

static void heap_percolate_up(struct heap* heap);
static void heap_percolate_down(struct heap* heap);

struct heap* heap_create(heap_less compare, void* param)
{
	struct heap* heap;
	heap = (struct heap*)calloc(1, sizeof(*heap));
	if (heap)
	{
		heap->less = compare;
		heap->param = param;
	}
	return heap;
}

void heap_destroy(struct heap* heap)
{
	if (heap->elts)
	{
		assert(heap->capacity > 0);
		free(heap->elts);
		heap->elts = NULL;
	}

	free(heap);
}

void heap_reserve(struct heap* heap, int size)
{
	void* p;
	if (heap->capacity < size)
	{
		p = realloc(heap->elts, sizeof(void*) * size);
		if (p)
		{
			heap->elts = (void**)p;
			heap->capacity = size;
		}
	}
}

int heap_size(struct heap* heap)
{
	return heap->size;
}

int heap_empty(struct heap* heap)
{
	return !heap->size;
}

int heap_push(struct heap* heap, void* ptr)
{
	if (heap->size + 1 >= heap->capacity)
	{
		heap_reserve(heap, heap->size + 2014);
		if (heap->size + 1 >= heap->capacity)
			return -ENOMEM;
	}

	heap->elts[heap->size++] = ptr;
	heap_percolate_up(heap);
	return 0;
}

void heap_pop(struct heap* heap)
{
	if (heap->size < 1)
		return;
	heap_percolate_down(heap);
	heap->size -= 1;
}

void* heap_top(struct heap* heap)
{
	if (heap->size < 1)
		return NULL;
	return heap->elts[0];
}

void* heap_get(struct heap* heap, int index)
{
	if (index < 0 || index >= heap->size)
		return NULL;
	return heap->elts[index];
}

static inline void heap_swap(struct heap* heap, int l, int r)
{
	void* elt;
	elt = heap->elts[l];
	heap->elts[l] = heap->elts[r];
	heap->elts[r] = elt;
}

/*
1. Add the element to the bottom level of the heap.
2. Compare the added element with its parent; if they are in the correct order, stop.
3. If not, swap the element with its parent and return to the previous step.
*/
static void heap_percolate_up(struct heap* heap)
{
	int n, p;

	assert(heap->size > 0);
	for (n = heap->size - 1; n > 0; n = p)
	{
		p = (n - 1) / 2; // parent
		if (heap->less(heap->param, heap->elts[p], heap->elts[n]))
			break;

		heap_swap(heap, p, n);
	}
}

/*
1. Replace the root of the heap with the last element on the last level.
2. Compare the new root with its children; if they are in the correct order, stop.
3. If not, swap the element with one of its children and return to the previous step. (Swap with its smaller child in a min-heap and its larger child in a max-heap.)
*/
static void heap_percolate_down(struct heap* heap)
{
	int n, c, min;
	assert(heap->size > 0);

	// replace first with last
	heap->elts[0] = heap->elts[heap->size - 1];

	for (n = 0; n < heap->size - 1; n = min)
	{
		min = n;

		// left/right child
		for (c = n * 2 + 1; c < heap->size - 1 && c < n * 2 + 3; c++)
		{
			if (heap->less(heap->param, heap->elts[c], heap->elts[min]))
				min = c;
		}

		if (n == min)
			break;

		heap_swap(heap, min, n);
	}
}
