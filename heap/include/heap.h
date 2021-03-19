/*
 * heap.h - heap
 *
 * Date   : 2021/03/14
 */
#ifndef __HEAP_H__
#define __HEAP_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct heap heap_t;

/// heap compare callback
/// @return 1 if ptr1 < ptr2, 0-other
typedef int (*heap_less)(void* param, const void* ptr1, const void* ptr2);

/// create heap
/// default min-heap, change heap_less behavor to create max-heap
heap_t* heap_create(heap_less compare, void* param);
void heap_destroy(heap_t* heap);

/// reserve heap capacity
/// if size <= capacity, do nothing
void heap_reserve(heap_t* heap, int size);

int heap_size(heap_t* heap);
int heap_empty(heap_t* heap);

int heap_push(heap_t* heap, void* ptr);
void heap_pop(heap_t* heap);
void* heap_top(heap_t* heap);

void* heap_get(heap_t* heap, int index);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
