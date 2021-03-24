/*
 * test.c - test
 *
 * Date   : 2021/03/24
 */
#include "mpeg4-aac.h"
#include <assert.h>
#include <string.h>

void mpeg4_aac_test(void)
{
	struct mpeg4_aac_t aac, aac2;
	const unsigned char asc[] = { 0x13, 0x88 };
	const unsigned char adts[] = { 0xFF, 0xF1, 0x5C, 0x40, 0x01, 0x1F, 0xFC };
//	const unsigned char ascsbr[] = { 0x13, 0x10, 0x56, 0xe5, 0x9d, 0x48, 0x00 };
	const unsigned char ascsbr[] = { 0x2b, 0x92, 0x08, 0x00 };

	unsigned char data[8];

	assert(sizeof(ascsbr) == mpeg4_aac_audio_specific_config_load(ascsbr, sizeof(ascsbr), &aac));
	assert(2 == aac.profile && 7 == aac.sampling_frequency_index && 2 == aac.channel_configuration);
	//assert(sizeof(ascsbr) == mpeg4_aac_audio_specific_config_save(&aac, data, sizeof(data)));
	//assert(0 == memcmp(ascsbr, data, sizeof(ascsbr)));

	assert(sizeof(asc) == mpeg4_aac_audio_specific_config_load(asc, sizeof(asc), &aac));
	assert(2 == aac.profile && 7 == aac.sampling_frequency_index && 1 == aac.channel_configuration);
	assert(sizeof(asc) == mpeg4_aac_audio_specific_config_save(&aac, data, sizeof(data)));
	assert(0 == memcmp(asc, data, sizeof(asc)));

	assert(sizeof(adts) == mpeg4_aac_adts_save(&aac, 1, data, sizeof(data)));
	assert(0 == memcmp(adts, data, sizeof(adts)));
	assert(7 == mpeg4_aac_adts_load(data, sizeof(adts), &aac2));
	assert(0 == memcmp(&aac, &aac2, sizeof(aac)));

	assert(22050 == mpeg4_aac_audio_frequency_to(aac.sampling_frequency_index));
	assert(aac.sampling_frequency_index == mpeg4_aac_audio_frequency_from(22050));

	//assert(sizeof(ascsbr) == mpeg4_aac_audio_specific_config_load(ascsbr, sizeof(ascsbr), &aac));
	//assert(2 == aac.profile && 6 == aac.sampling_frequency_index && 1 == aac.channel_configuration);
}

int main(void)
{
    mpeg4_aac_test();
    return 0;
}
