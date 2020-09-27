/*
 * bsearch.h - bsearch
 *
 * Date   : 2020/09/28
 */
#ifndef BSEARCH_H
#    define BSEARCH_H

#include "types.h"
#include <sys/types.h>

void *bsearch(const void*key, const void *base, size_t num, size_t size,
              int (*cmp)(const void *key, const void *elt));
#endif
