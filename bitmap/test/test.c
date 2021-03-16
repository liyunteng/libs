/*
 * test.c - test
 *
 * Date   : 2021/03/16
 */
#include "bitmap.h"
#include "hweight.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>


#define N 1001

void hweight_test(void)
{
	assert(8 == hweight8(0xFF));
	assert(0 == hweight8(0x00));
	assert(2 == hweight8(0x88));
	assert(2 == hweight8(0x11));
	assert(5 == hweight8(0xF8));
	assert(4 == hweight8(0x0F));
	assert(4 == hweight8(0x3C));

	assert(16 == hweight16(0xFFFF));
	assert(0 == hweight16(0x0000));
	assert(4 == hweight16(0x8421));
	assert(4 == hweight16(0x1428));
	assert(10 == hweight16(0xF88F));
	assert(8 == hweight16(0x0FF0));
	assert(8 == hweight16(0x3CC3));

	assert(32 == hweight32(0xFFFFFFFF));
	assert(0 == hweight32(0x00000000));
	assert(8 == hweight32(0x84211248));
	assert(8 == hweight32(0x14282418));
	assert(20 == hweight32(0xF88FF11F));
	assert(16 == hweight32(0x0FF063C9));
	assert(16 == hweight32(0x3CC35699));

	assert(64 == hweight64(0xFFFFFFFFFFFFFFFFull));
	assert(0 == hweight64(0x0000000000000000ull));
	assert(16 == hweight64(0x8421824181428124ull));
	assert(16 == hweight64(0x1248128414281842ull));
	assert(40 == hweight64(0xF8F4F2F1373E57E9ull));
	assert(32 == hweight64(0x0F172E33CC3CC371ull));
	assert(32 == hweight64(0x9CC993396CC66336ull));
}

void bitmap_test(void)
{
	unsigned int i, j, n;
	uint8_t bitmap[(N + 7)/8];
	uint8_t ffs[4] = { 0xEF, 0xCD, 0xA8, 0x19 };
	assert(18 == bitmap_weight(ffs, 32));
	assert(0 == bitmap_count_leading_zero(ffs, 32));
	assert(3 == bitmap_find_first_zero(ffs, 32));
	assert(0 == bitmap_count_next_zero(ffs, 32, 12));
	assert(2 == bitmap_find_next_zero(ffs, 32, 15));
	assert(5 == bitmap_count_next_zero(ffs, 32, 22));
	assert(4 == bitmap_find_next_zero(ffs, 32, 6));

	bitmap_zero(bitmap, N);
	assert(0 == bitmap_weight(bitmap, N));
	assert(N == bitmap_count_leading_zero(bitmap, N));
	for (i = 0; i < N; i++)
	{
		assert(!bitmap_test_bit(bitmap, i));
	}

	bitmap_fill(bitmap, N);
	assert(N == bitmap_weight(bitmap, N));
	assert(0 == bitmap_count_leading_zero(bitmap, N));
	for (i = 0; i < N; i++)
	{
		assert(bitmap_test_bit(bitmap, i));
	}

	srand(time(NULL));
	//srand(31415926);
	for (i = 0; i < sizeof(bitmap) / sizeof(bitmap[0]); i++)
		bitmap[i] = (uint8_t)(rand() * 31415926UL);

	n = bitmap_count_leading_zero(bitmap, N);
	assert(n == bitmap_count_next_zero(bitmap, N, 0));
	for (j = 0; j < N; j += n + 1)
	{
		n = bitmap_count_next_zero(bitmap, N, j);

		for (i = 0; i < n; i++)
		{
			assert(!bitmap_test_bit(bitmap, j + i));
		}

		assert(j + n == N || bitmap_test_bit(bitmap, j + n));
	}

	j = rand() % N;
	n = rand() % (N - j);
	bitmap_set(bitmap, j, n);
	for (i = j; i < j + n; i++)
	{
		assert(bitmap_test_bit(bitmap, i));
	}

	j = rand() % N;
	n = rand() % (N - j);
	bitmap_clear(bitmap, j, n);
	for (i = j; i < j + n; i++)
	{
		assert(!bitmap_test_bit(bitmap, i));
	}

	printf("bitmap test ok\n");
}


int main(void)
{
    hweight_test();
    bitmap_test();
    return 0;
}
