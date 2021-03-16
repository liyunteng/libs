/*
 * test.c - test
 *
 * Date   : 2021/03/16
 */
#include "bits.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>


static void bits_test2(void)
{
	struct bits_t bits;
	// 1, 010, 011, 00100, 00101, 00110, 00111, 0001000, 0001001, 0001010, 0001011, 0001100, 0001101, 0001110, 0001111
	const uint8_t data[] = { 0xA6, 0x42, 0x98, 0xE2, 0x04, 0x8A, 0x16, 0x30, 0x68, 0xE1, 0xE0 };
	bits_init(&bits, data, sizeof(data));
	assert(0 == bits_read_ue(&bits));
	assert(1 == bits_read_ue(&bits));
	assert(2 == bits_read_ue(&bits));
	assert(3 == bits_read_ue(&bits));
	assert(4 == bits_read_ue(&bits));
	assert(5 == bits_read_ue(&bits));
	assert(6 == bits_read_ue(&bits));
	assert(7 == bits_read_ue(&bits));
	assert(8 == bits_read_ue(&bits));
	assert(9 == bits_read_ue(&bits));
	assert(10 == bits_read_ue(&bits));
	assert(11 == bits_read_ue(&bits));
	assert(12 == bits_read_ue(&bits));
	assert(13 == bits_read_ue(&bits));
	assert(14 == bits_read_ue(&bits));
}

void bits_test(void)
{
	struct bits_t bits;
	const uint8_t data[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xCD, 0xAB };
	bits_init(&bits, data, sizeof(data));
	assert(0 == bits_read(&bits));
	assert(0 == bits_read(&bits));
	assert(0 == bits_read(&bits));
	assert(0 == bits_read(&bits));
	assert(0 == bits_read(&bits));
	assert(0 == bits_read(&bits));
	assert(0 == bits_read(&bits));
	assert(1 == bits_read(&bits));

	assert(0 == bits_read(&bits));
	assert(0 == bits_read(&bits));
	assert(1 == bits_read(&bits));
	assert(0 == bits_read(&bits));
	assert(0 == bits_read(&bits));
	assert(0 == bits_read(&bits));
	assert(1 == bits_read(&bits));
//	assert(1 == bits_read(&bits));

	assert(0x05 == bits_read_n(&bits, 3));
	assert(0x15 == bits_read_n(&bits, 8));
	assert(0x27 == bits_read_n(&bits, 6));
	assert(0x08 == bits_read_n(&bits, 4));
	assert(0x09ABCDEF == bits_read_n(&bits, 28));

	assert(0 == bits_read_n(&bits, 17) && bits.error);
}

int main(void)
{
    bits_test();
    bits_test2();
    return 0;
}
