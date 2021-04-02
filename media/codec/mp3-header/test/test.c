/*
 * test.c - test
 *
 * Date   : 2021/03/24
 */
#include "mp3-header.h"
#include <stdint.h>
#include <assert.h>
#include <string.h>

void mp3_header_test(void)
{
	uint8_t v[4] = { 0xff, 0xfb, 0xe0, 0x64 };
	uint8_t v2[4];
	struct mp3_header_t mp3;

	assert(4 == mp3_header_load(&mp3, v, 4));
	assert(MP3_MPEG1 == mp3.version && MP3_LAYER3 == mp3.layer);
	assert(14 == mp3.bitrate_index && 320000 == mp3_get_bitrate(&mp3));
	assert(0 == mp3.sampling_frequency && 44100 == mp3_get_frequency(&mp3));
	assert(1 == mp3.mode && 1 == mp3.protection);
	assert(4 == mp3_header_save(&mp3, v2, 4));
	assert(0 ==  memcmp(v, v2, 4));
}


int main(void)
{
    mp3_header_test();
    return 0;
}
