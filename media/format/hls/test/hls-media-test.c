/*
 * hls-media-test.c - hls-media-test
 *
 * Date   : 2021/04/02
 */
#include "hls-m3u8.h"
#include "hls-media.h"
#include "hls-param.h"
#include "mpeg-ps.h"
#include "mpeg-ts-proto.h"
#include "mpeg-ts.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

static int
hls_segment(void *m3u8, const void *data, size_t bytes, int64_t pts,
            int64_t dts, int64_t duration)
{
    static int64_t s_dts = -1;
	int discontinue = -1 != s_dts ? 0 : (dts > s_dts + HLS_DURATION / 2 ? 1 : 0);
	s_dts = dts;

    printf("hls pts: %"PRId64" dts: %"PRId64" durtaion: %"PRId64"\n", pts, dts, duration);
	static int i = 0;
	char name[128] = {0};
    snprintf(name, sizeof(name), "/home/lyt/segment-%03d.ts", i++);
	hls_m3u8_add((hls_m3u8_t*)m3u8, name, pts, duration, discontinue);

	FILE* fp = fopen(name, "wb");
    if(fp)
    {
        fwrite(data, 1, bytes, fp);
        fclose(fp);
    }

	return 0;
}
extern int mpeg_h264_find_keyframe(const uint8_t* p, size_t bytes);
static int
on_ts_packet(void *param, int program, int stream, int avtype, int flags,
             int64_t pts, int64_t dts, const void *data, size_t bytes)
{
    hls_media_t *ctx = (hls_media_t *)param;
    assert(ctx);
    int key = 0;

    printf("pts: %"PRId64" dts: %"PRId64" bytes: %lu flags: %d\n", pts, dts, bytes, flags);
    switch (avtype) {
    case PSI_STREAM_AAC:
        return hls_media_input(ctx, STREAM_AUDIO_AAC, data, bytes, pts, dts, 0);

    case PSI_STREAM_H264:
        key = mpeg_h264_find_keyframe(data, bytes);
        if (key) {
            hls_media_input(ctx, STREAM_VIDEO_H264, NULL, 0, 0, 0, 0);
        }
        return hls_media_input(ctx, STREAM_VIDEO_H264, data, bytes, pts, dts, key);

    case PSI_STREAM_H265:
        return hls_media_input(ctx, STREAM_VIDEO_H265, data, bytes, pts, dts, (flags & MPEG_FLAG_IDR_FRAME) ? HLS_FLAGS_KEYFRAME : 0);

    default:
        printf("avtype: 0x%x\n", avtype);
        assert(0);
    }

}

void
hls_media_test(const char *ts)
{
    hls_m3u8_t *m3u  = hls_m3u8_create(0, 3);
    hls_media_t *ctx = hls_media_create(HLS_DURATION * 1000, hls_segment, m3u);

    unsigned char ptr[188];
    FILE *fp = fopen(ts, "rb");
    assert(fp);

    struct ts_demuxer_t *demux = ts_demuxer_create(on_ts_packet, ctx);
    assert(ts);
    while (1 == fread(ptr, sizeof(ptr), 1, fp)) {
        ts_demuxer_input(demux, ptr, sizeof(ptr));
    }
    ts_demuxer_flush(demux);

    static char data[2 * 1024 * 1024];
    // write m3u8 file
    hls_media_input(ctx, STREAM_VIDEO_H264, NULL, 0, 0, 0, 0);
    hls_m3u8_playlist(m3u, 1, data, sizeof(data));
    printf("%s\n", data);
    FILE *m3u8fp = fopen("/home/lyt/ts.m3u8", "w");
    assert(m3u8fp);
    fwrite(data, 1, strlen(data), m3u8fp);
    fclose(m3u8fp);

    hls_m3u8_destroy(m3u);
    ts_demuxer_destroy(demux);
    hls_media_destroy(ctx);
}
