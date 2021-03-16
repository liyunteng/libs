/*
 * bsearch.h - bsearch
 *
 * Date   : 2020/09/28
 */
#ifndef __BSEARCH_H__
#define __BSEARCH_H__
#include <stddef.h>

void *bsearch(const void*key, const void *base, size_t num, size_t size,
              int (*cmp)(const void *key, const void *elt));
#endif
