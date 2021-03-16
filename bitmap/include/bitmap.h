/*
 * bitmap.h - bitmap
 *
 * Date   : 2021/03/16
 */
#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bitmap_zero(uint8_t *bitmap, size_t nbits);
void bitmap_fill(uint8_t *bitmap, size_t nbits);
void bitmap_copy(uint8_t *bitmap, const uint8_t *src, size_t nbits);

void bitmap_set(uint8_t *bitmap, size_t start, size_t len);
void bitmap_clear(uint8_t *bitmap, size_t start, size_t len);

void bitmap_or(uint8_t *result, const uint8_t *src1, const uint8_t *src2,
               size_t nbits);
void bitmap_and(uint8_t *result, const uint8_t *sr1, const uint8_t *src2,
                size_t nbits);
void bitmap_xor(uint8_t *result, const uint8_t *src1, const uint8_t *src2,
                size_t nbits);

size_t bitmap_count_leading_zero(const uint8_t *bitmap, size_t nbits);
    size_t bitmap_count_next_zero(const uint8_t *bitmap, size_t nbits, size_t start);
size_t bitmap_find_first_zero(const uint8_t *bitmap, size_t nbits);
size_t bitmap_find_next_zero(const uint8_t *bitmap, size_t nbits, size_t start);
size_t bitmap_weight(const uint8_t *bitmap, size_t nbits);

/// @return 0-not set, other-set to 1
int bitmap_test_bit(const uint8_t *bitmap, size_t bits);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
