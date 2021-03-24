/*
 * test.c - test
 *
 * Date   : 2021/03/24
 */
#include "webm-vpx.h"
#include <stdint.h>
#include <assert.h>
#include <string.h>

void webm_vpx_test(void)
{
    const unsigned char src[] = {
        0x00, 0x1f, 0x80, 0x02, 0x02, 0x02, 0x00, 0x00
    };
    unsigned char data[sizeof(src)];

    struct webm_vpx_t vpx;
    assert(sizeof(src) == webm_vpx_codec_configuration_record_load(src, sizeof(src), &vpx));
    assert(0 == vpx.profile && 31 == vpx.level && 8 == vpx.bit_depth && 0 == vpx.chroma_subsampling && 0 == vpx.video_full_range_flag);
    assert(sizeof(src) == webm_vpx_codec_configuration_record_save(&vpx, data, sizeof(data)));
    assert(0 == memcmp(src, data, sizeof(src)));
}

int main(void)
{
    webm_vpx_test();
    return 0;
}
