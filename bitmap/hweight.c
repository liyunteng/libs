/*
 * hweight.c - hweight
 *
 * Date   : 2021/03/16
 */
#include "hweight.h"

int hweight8(uint8_t w)
{
	w = w - ((w >> 1) & 0x55);
	w = (w & 0x33) + ((w >> 2) & 0x33);
	return (w + (w >> 4)) & 0x0F;
}

int hweight16(uint16_t w)
{
	w = w - ((w >> 1) & 0x5555);
	w = (w & 0x3333) + ((w >> 2) & 0x3333);
	w = (w  + (w >> 4)) & 0x0F0F;
	return (w + (w >> 8)) & 0x00FF;
}

int hweight32(uint32_t w)
{
	w = w - ((w >> 1) & 0x55555555);
	w = (w & 0x33333333) + ((w >> 2) & 0x33333333);
	w = (w + (w >> 4)) & 0x0F0F0F0F;
	w = (w + (w >> 8));
	return (w + (w >> 16)) & 0x000000FF;
}

int hweight64(uint64_t w)
{
	w = w - ((w >> 1) & 0x5555555555555555ull);
	w = (w & 0x3333333333333333ull) + ((w >> 2) & 0x3333333333333333ull);
	w = (w + (w >> 4)) & 0x0F0F0F0F0F0F0F0Full;
	w = (w + (w >> 8));
	w = (w + (w >> 16));
	return (w + (w >> 32)) & 0x00000000000000FFull;
}
