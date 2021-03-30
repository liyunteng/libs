/*
 * mpeg-ts-multi-program-test.c - mpeg-ts-multi-program-test
 *
 * Date   : 2021/03/30
 */

#include "mpeg-ps.h"
#include "mpeg-ts.h"
#include "mpeg-ts-proto.h"
#include "mpeg4-avc.h"
#include "mov-reader.h"
#include "mov-format.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

const struct mov_buffer_t *mov_file_buffer(void);

static uint8_t s_packet[2 * 1024 * 1024];
static uint8_t s_buffer[4 * 1024 * 1024];
static struct mpeg4_avc_t s_avc;

typedef struct mpeg_ts_multi_program_test_t
{
	int count;
	struct
	{
		int pn;
		int track;
		int stream;
	} pn[4];

	void* ts;
} mpeg_ts_multi_program_test_t;

static void* ts_alloc(void *param, size_t bytes)
{
	void *ptr = malloc(bytes);
    return ptr;
}

static void ts_free(void* param, void* packet)
{
    free(packet);
}

static int ts_write(void* param, const void* packet, size_t bytes)
{
	return 1 == fwrite(packet, bytes, 1, (FILE*)param) ? 0 : ferror((FILE*)param);
}

static void onread(void* param, uint32_t track, const void* data, size_t bytes, int64_t pts, int64_t dts, int flags)
{
	mpeg_ts_multi_program_test_t* ctx = (mpeg_ts_multi_program_test_t*)param;
	for (int i = 0; i < ctx->count; i++)
	{
		if (ctx->pn[i].track == track)
		{
            int n = h264_mp4toannexb(&s_avc, data, bytes, s_packet, sizeof(s_packet));
			/* assert(0 == mpeg_ts_write(ctx->ts, ctx->pn[i].stream, (flags & MOV_AV_FLAG_KEYFREAME) ? 1 : 0, pts * 90, dts * 90, data, bytes)); */
			assert(0 == mpeg_ts_write(ctx->ts, ctx->pn[i].stream, (flags & MOV_AV_FLAG_KEYFREAME) ? 1 : 0, pts * 90, dts * 90, s_packet, n));
			return;
		}
	}
}

static void mov_video_info(void* param, uint32_t track, uint8_t object, int width, int height, const void* extra, size_t bytes)
{
	assert(MOV_OBJECT_H264 == object || MOV_OBJECT_HEVC == object || MOV_OBJECT_AV1 == object || MOV_OBJECT_VP9 == object);
	mpeg_ts_multi_program_test_t* ctx = (mpeg_ts_multi_program_test_t*)param;
	ctx->pn[ctx->count].pn = ctx->count + 1;
	ctx->pn[ctx->count].track = track;
    printf("ctx->pn[%d].pn = %d\n", ctx->count, ctx->count+1);
    printf("ctx->pn[%d].track = %d\n", ctx->count, track);
    mpeg4_avc_decoder_configuration_record_load((const uint8_t*)extra, bytes, &s_avc);
	assert(0 == mpeg_ts_add_program(ctx->ts, ctx->pn[ctx->count].pn, NULL, 0));
	ctx->pn[ctx->count].stream = mpeg_ts_add_program_stream(ctx->ts, ctx->pn[ctx->count].pn, PSI_STREAM_H264, NULL, 0);
	ctx->count++;
}

static void mov_audio_info(void* param, uint32_t track, uint8_t object, int channel_count, int bit_per_sample, int sample_rate, const void* extra, size_t bytes)
{
	assert(MOV_OBJECT_AAC == object || MOV_OBJECT_OPUS == object);
	mpeg_ts_multi_program_test_t* ctx = (mpeg_ts_multi_program_test_t*)param;
	ctx->pn[ctx->count].pn = ctx->count + 1;
	ctx->pn[ctx->count].track = track;
	assert(0 == mpeg_ts_add_program(ctx->ts, ctx->pn[ctx->count].pn, NULL, 0));
	ctx->pn[ctx->count].stream = mpeg_ts_add_program_stream(ctx->ts, ctx->pn[ctx->count].pn, MOV_OBJECT_AAC == object ? PSI_STREAM_AAC : PSI_STREAM_AUDIO_OPUS, NULL, 0);
	ctx->count++;
}

void mpeg_ts_multi_program_test(const char* mp4)
{
	struct mpeg_ts_multi_program_test_t ctx;
	memset(&ctx, 0, sizeof(ctx));

	char output[256] = { 0 };
	snprintf(output, sizeof(output), "%s.ts", mp4);

	struct mpeg_ts_func_t tshandler;
	tshandler.alloc = ts_alloc;
	tshandler.write = ts_write;
	tshandler.free = ts_free;

	FILE* fp = fopen(output, "wb");
	ctx.ts = mpeg_ts_create(&tshandler, fp);

	FILE* rfp = fopen(mp4, "rb");
	mov_reader_t* mov = mov_reader_create(mov_file_buffer(), rfp);

	struct mov_reader_trackinfo_t info = { mov_video_info, mov_audio_info };
	mov_reader_getinfo(mov, &info, &ctx);

	while (mov_reader_read(mov, s_buffer, sizeof(s_buffer), onread, &ctx) > 0)
	{
	}

	mov_reader_destroy(mov);
	mpeg_ts_destroy(ctx.ts);
	fclose(rfp);
	fclose(fp);
}
