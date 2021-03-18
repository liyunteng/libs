/*
 * bsearch.h - bsearch
 *
 * Date   : 2020/09/28
 */

#ifndef __BSEARCH_H__
#define __BSEARCH_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *bsearch(const void*key, const void *base, size_t num, size_t size,
              int (*cmp)(const void *key, const void *elt));

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
