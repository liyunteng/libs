/*
 * test.c - test
 *
 * Date   : 2021/03/24
 */
#include "mpeg4-avc.h"
#include <stdint.h>
#include <assert.h>
#include <string.h>

static void mpeg4_annexbtomp4_test2(void)
{
	const uint8_t sps[] = { 0x00,0x00,0x00,0x01,0x67,0x42,0xe0,0x1e,0xab,0xcd, };
	const uint8_t pps[] = { 0x00,0x00,0x00,0x01,0x28,0xce,0x3c,0x80 };
	const uint8_t sps1[] = { 0x00,0x00,0x00,0x01,0x67,0x42,0xe0,0x1e,0x4b,0xcd, 0x01 };
	const uint8_t pps1[] = { 0x00,0x00,0x00,0x01,0x28,0xce,0x3c,0x80, 0x01 };
	const uint8_t sps2[] = { 0x00,0x00,0x00,0x01,0x67,0x42,0xe0,0x1e,0xab };
	const uint8_t pps2[] = { 0x00,0x00,0x00,0x01,0x28,0xce,0x3c };

	int vcl, update;
	uint8_t buffer[128];
	struct mpeg4_avc_t avc;
	memset(&avc, 0, sizeof(avc));

	h264_annexbtomp4(&avc, sps, sizeof(sps), buffer, sizeof(buffer), &vcl, &update);
	assert(0 == vcl && 1 == update);
	h264_annexbtomp4(&avc, pps, sizeof(pps), buffer, sizeof(buffer), &vcl, &update);
	assert(0 == vcl && 1 == update && 1 == avc.nb_sps && avc.sps[0].bytes == sizeof(sps)-4 && 0 == memcmp(avc.sps[0].data, sps+4, sizeof(sps) - 4) && 1 == avc.nb_pps && avc.pps[0].bytes == sizeof(pps) - 4 && 0 == memcmp(avc.pps[0].data, pps+4, sizeof(pps) - 4));

	h264_annexbtomp4(&avc, sps1, sizeof(sps1), buffer, sizeof(buffer), &vcl, &update);
	assert(0 == vcl && 1 == update && 2 == avc.nb_sps && avc.sps[0].bytes == sizeof(sps) - 4 && avc.sps[1].bytes == sizeof(sps1) - 4 && 0 == memcmp(avc.sps[0].data, sps+4, sizeof(sps) - 4) && 0 == memcmp(avc.sps[1].data, sps1 + 4, sizeof(sps1) - 4) && 1 == avc.nb_pps && avc.pps[0].bytes == sizeof(pps) - 4 && 0 == memcmp(avc.pps[0].data, pps + 4, sizeof(pps) - 4));

	h264_annexbtomp4(&avc, pps1, sizeof(pps1), buffer, sizeof(buffer), &vcl, &update);
	assert(0 == vcl && 1 == update && 2 == avc.nb_sps && avc.sps[0].bytes == sizeof(sps) - 4 && avc.sps[1].bytes == sizeof(sps1) - 4 && 0 == memcmp(avc.sps[0].data, sps + 4, sizeof(sps) - 4) && 0 == memcmp(avc.sps[1].data, sps1 + 4, sizeof(sps1) - 4) && 1 == avc.nb_pps && avc.pps[0].bytes == sizeof(pps1) - 4 && 0 == memcmp(avc.pps[0].data, pps1 + 4, sizeof(pps1) - 4));

	h264_annexbtomp4(&avc, sps2, sizeof(sps2), buffer, sizeof(buffer), &vcl, &update);
	assert(0 == vcl && 1 == update && 2 == avc.nb_sps && avc.sps[0].bytes == sizeof(sps2) - 4 && avc.sps[1].bytes == sizeof(sps1) - 4 && 0 == memcmp(avc.sps[0].data, sps2 + 4, sizeof(sps2) - 4) && 0 == memcmp(avc.sps[1].data, sps1 + 4, sizeof(sps1) - 4) && 1 == avc.nb_pps && avc.pps[0].bytes == sizeof(pps1) - 4 && 0 == memcmp(avc.pps[0].data, pps1 + 4, sizeof(pps1) - 4));

	h264_annexbtomp4(&avc, pps2, sizeof(pps2), buffer, sizeof(buffer), &vcl, &update);
	assert(0 == vcl && 1 == update && 2 == avc.nb_sps && avc.sps[0].bytes == sizeof(sps2) - 4 && avc.sps[1].bytes == sizeof(sps1) - 4 && 0 == memcmp(avc.sps[0].data, sps2 + 4, sizeof(sps2) - 4) && 0 == memcmp(avc.sps[1].data, sps1 + 4, sizeof(sps1) - 4) && 1 == avc.nb_pps && avc.pps[0].bytes == sizeof(pps2) - 4 && 0 == memcmp(avc.pps[0].data, pps2 + 4, sizeof(pps2) - 4));
}

void mpeg4_annexbtomp4_test(void)
{
	const uint8_t sps[] = { 0x67,0x42,0xe0,0x1e,0xab };
	const uint8_t pps[] = { 0x28,0xce,0x3c,0x80 };
	const uint8_t annexb[] = { 0x00,0x00,0x00,0x01,0x67,0x42,0xe0,0x1e,0xab, 0x00,0x00,0x00,0x01,0x28,0xce,0x3c,0x80,0x00,0x00,0x00,0x01,0x65,0x11 };
	uint8_t output[256];
	int vcl, update;

	struct mpeg4_avc_t avc;
	memset(&avc, 0, sizeof(avc));
	assert(h264_annexbtomp4(&avc, annexb, sizeof(annexb), output, sizeof(output), &vcl, &update) > 0);
	assert(1 == avc.nb_sps && avc.sps[0].bytes == sizeof(sps) && 0 == memcmp(avc.sps[0].data, sps, sizeof(sps)));
	assert(1 == avc.nb_pps && avc.pps[0].bytes == sizeof(pps) && 0 == memcmp(avc.pps[0].data, pps, sizeof(pps)));
	assert(vcl == 1);

}

void mpeg4_avc_test(void)
{
	const unsigned char src[] = {
		0x01,0x42,0xe0,0x1e,0xff,0xe1,0x00,0x21,0x67,0x42,0xe0,0x1e,0xab,0x40,0xf0,0x28,
		0xd0,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x19,0x70,0x20,0x00,0x78,0x00,0x00,0x0f,
		0x00,0x16,0xb1,0xb0,0x3c,0x50,0xaa,0x80,0x80,0x01,0x00,0x04,0x28,0xce,0x3c,0x80
	};
	const unsigned char nalu[] = {
		0x00,0x00,0x00,0x01,0x67,0x42,0xe0,0x1e,0xab,0x40,0xf0,0x28,0xd0,0x80,0x00,0x00,
		0x00,0x80,0x00,0x00,0x19,0x70,0x20,0x00,0x78,0x00,0x00,0x0f,0x00,0x16,0xb1,0xb0,
		0x3c,0x50,0xaa,0x80,0x80,0x00,0x00,0x00,0x01,0x28,0xce,0x3c,0x80
	};
	unsigned char data[sizeof(src)];

	struct mpeg4_avc_t avc;
	assert(sizeof(src) == mpeg4_avc_decoder_configuration_record_load(src, sizeof(src), &avc));
	assert(0x42 == avc.profile && 0xe0 == avc.compatibility && 0x1e == avc.level);
	assert(4 == avc.nalu && 1 == avc.nb_sps && 1 == avc.nb_pps);
	assert(sizeof(src) == mpeg4_avc_decoder_configuration_record_save(&avc, data, sizeof(data)));
	assert(0 == memcmp(src, data, sizeof(src)));
    mpeg4_avc_codecs(&avc, (char*)data, sizeof(data));
    assert(0 == memcmp("avc1.42e01e", data, 11));

	assert(sizeof(nalu) == mpeg4_avc_to_nalu(&avc, data, sizeof(data)));
	assert(0 == memcmp(nalu, data, sizeof(nalu)));
}

int main(void)
{
    mpeg4_avc_test();
    mpeg4_annexbtomp4_test();
    mpeg4_annexbtomp4_test2();
    return 0;
}
