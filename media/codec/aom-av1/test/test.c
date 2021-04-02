/*
 * test.c - test
 *
 * Date   : 2021/04/02
 */
#include "aom-av1.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

void aom_av1_test(void)
{
	const unsigned char src[] = {
		0x81, 0x04, 0x0c, 0x00, 0x0a, 0x0b, 0x00, 0x00, 0x00, 0x24, 0xcf, 0x7f, 0x0d, 0xbf, 0xff, 0x30, 0x08
	};
	unsigned char data[sizeof(src)];

	struct aom_av1_t av1;
	assert(sizeof(src) == aom_av1_codec_configuration_record_load(src, sizeof(src), &av1));
	assert(1 == av1.version && 0 == av1.seq_profile && 4 == av1.seq_level_idx_0);
	assert(0 == av1.seq_tier_0 && 0 == av1.high_bitdepth && 0 == av1.twelve_bit && 0 == av1.monochrome && 1 == av1.chroma_subsampling_x && 1 == av1.chroma_subsampling_y && 0 == av1.chroma_sample_position);
	assert(0 == av1.initial_presentation_delay_present && 0 == av1.initial_presentation_delay_minus_one);
	assert(13 == av1.bytes);
	assert(sizeof(src) == aom_av1_codec_configuration_record_save(&av1, data, sizeof(data)));
	assert(0 == memcmp(src, data, sizeof(src)));

	aom_av1_codecs(&av1, (char*)data, sizeof(data));
	assert(0 == memcmp("av01.0.04M.08", data, 13));
}

int main(void)
{
    aom_av1_test();
    return 0;
}
