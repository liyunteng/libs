/*
 * hls-fmp4-test.c - hls-fmp4-test
 *
 * Date   : 2021/04/02
 */
#include "hls-fmp4.h"
#include "hls-m3u8.h"
#include "mov-reader.h"
#include "mov-format.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

static uint32_t s_ori_vtrack = 0xFFFFFFFF;
static uint32_t s_ori_atrack = 0xFFFFFFFF;
static uint32_t s_vtrack = 0xFFFFFFFF;
static uint32_t s_atrack = 0xFFFFFFFF;
static uint8_t s_buffer[4 * 1024 * 1024];
extern const struct mov_buffer_t *mov_file_buffer(void);

static char s_packet[2 * 1024 * 1024];

static int hls_segment(void* m3u8, const void* data, size_t bytes, int64_t pts, int64_t dts, int64_t duration)
{
	static int i = 0;
	static char name[128] = { 0 };
	snprintf(name, sizeof(name), "segment-%03d.mp4", ++i);
	FILE* fp = fopen(name, "wb");
	fwrite(data, 1, bytes, fp);
	fclose(fp);

	return hls_m3u8_add((hls_m3u8_t*)m3u8, name, dts, duration, 0);
}

static int hls_init_segment(hls_fmp4_t* hls, hls_m3u8_t* m3u)
{
	int bytes = hls_fmp4_init_segment(hls, s_packet, sizeof(s_packet));

	FILE* fp = fopen("segment-000.mp4", "wb");
	fwrite(s_packet, 1, bytes, fp);
	fclose(fp);

	return hls_m3u8_set_x_map(m3u, "segment-000.mp4");
}

static void onread(void* param, uint32_t track, const void* buffer, size_t bytes, int64_t pts, int64_t dts, int flags)
{
    static int count = 0;
    hls_fmp4_t *ctx = (hls_fmp4_t *)param;
    assert(ctx);

    if (track == s_ori_vtrack) {
        /* printf("video\n"); */
        if (count % 10 == 0) {
            hls_fmp4_init_segment(ctx, NULL, 0);
        }
        assert(hls_fmp4_input(ctx, s_vtrack, buffer, bytes, pts, dts, flags) == 0);
        count++;
    } else if (track == s_ori_atrack) {
        /* printf("audio\n"); */
        assert(hls_fmp4_input(ctx, s_atrack, buffer, bytes, pts, dts, flags) == 0);
    }
}

static void mov_video_info(void* param, uint32_t track, uint8_t object, int width, int height, const void* extra, size_t bytes)
{
    hls_fmp4_t *ctx = (hls_fmp4_t *)param;
    s_ori_vtrack = track;
    s_vtrack = hls_fmp4_add_video(ctx, object, width, height, extra, bytes);
    printf("s_vtrack: %d  s_ori_vtrack: %d\n", s_vtrack, track);
    assert(s_vtrack >= 0);
}

static void mov_audio_info(void* param, uint32_t track, uint8_t object, int channel_count, int bit_per_sample, int sample_rate, const void* extra, size_t bytes)
{
    hls_fmp4_t *ctx = (hls_fmp4_t *)param;
    s_ori_atrack = track;
    s_atrack = hls_fmp4_add_audio(ctx, object, channel_count, bit_per_sample, sample_rate, extra, bytes);
    printf("s_atrack: %d s_ori_atrack: %d\n", s_atrack, track);
    assert(s_atrack >= 0);
}

void hls_fmp4_test(const char *mp4, const char *m3u8)
{
    FILE *fp = fopen(mp4, "rb");
    assert(fp);

    mov_reader_t *mov = mov_reader_create(mov_file_buffer(), fp);
    assert(mov);

    uint64_t duration = mov_reader_getduration(mov);
    printf("duration: %"PRIu64"\n", duration);
    hls_m3u8_t *m3u = hls_m3u8_create(0, 7);
    hls_fmp4_t *ctx = hls_fmp4_create(2000, hls_segment, m3u);
    assert(ctx);

    struct mov_reader_trackinfo_t info = { mov_video_info, mov_audio_info, NULL};
    mov_reader_getinfo(mov, &info, ctx);

    hls_init_segment(ctx, m3u);
    while (mov_reader_read(mov, s_buffer, sizeof(s_buffer), onread, ctx) > 0) {

    }

    // write m3u8 file
	hls_m3u8_playlist(m3u, 1, s_packet, sizeof(s_packet));
    printf("%s\n", s_packet);
    FILE *m3u8fp = fopen(m3u8, "w");
    assert(m3u8fp);
    fprintf(m3u8fp, "%s", s_packet);
    fclose(m3u8fp);
	hls_m3u8_destroy(m3u);

    mov_reader_destroy(mov);
    hls_fmp4_destroy(ctx);
}
