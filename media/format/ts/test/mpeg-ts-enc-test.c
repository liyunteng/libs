/*
 * mpeg-ts-enc-test.c - mpeg-ts-enc-test
 *
 * Date   : 2021/03/31
 */

#include "mpeg-ts-proto.h"
#include "mpeg-ts.h"
#include "mpeg4-avc.h"
#include "mpeg4-aac.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t s_packet[4 * 1024 * 1024];
static uint8_t s_extra_data[64 * 1024];

struct mpeg_ts_enc_test_t {
    void *ts;
    struct mpeg4_avc_t avc;
    struct mpeg4_aac_t aac;
    uint8_t *vptr;

    int vtrack, atrack;
    int pts, dts;
    int vcl;
};

static uint8_t *
file_read(const char *file, long *size)
{
    FILE *fp = fopen(file, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        *size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        uint8_t *ptr = (uint8_t *)malloc(*size);
        fread(ptr, 1, *size, fp);
        fclose(fp);

        return ptr;
    }

    return NULL;
}

static void *
ts_alloc(void *param, size_t bytes)
{
    static char buf[188];
    assert(bytes <= sizeof(buf));
    return buf;
}

static void
ts_free(void *param, void *ptr)
{
    return;
}

static int
ts_write(void *param, const void *packet, size_t bytes)
{
    return 1 == fwrite(packet, bytes, 1, (FILE *)param) ? 0 :
                                                          ferror((FILE *)param);
}

static void adts_reader(struct mpeg_ts_enc_test_t *ctx, const uint8_t *data, size_t bytes)
{
    int64_t pts = 0;
    while (data && bytes > 7) {
        int n = mpeg4_aac_adts_frame_length(data, bytes);
        if (n < 0)
            break;

        if (n > bytes)
            break;

        if (-1 == ctx->atrack) {
            uint8_t asc[16];
            assert(7 == mpeg4_aac_adts_load(data, bytes, &ctx->aac));
            int len = mpeg4_aac_audio_specific_config_save(&ctx->aac, asc, sizeof(asc));
            assert(len > 0 && len <= sizeof(asc));
            /* assert(0 == mpeg_ts_add_program(ctx->ts, 1, NULL, 0)); */
            /* ctx->atrack = mpeg_ts_add_program_stream(ctx->ts, 1, PSI_STREAM_AAC, NULL, 0); */
            ctx->atrack = mpeg_ts_add_stream(ctx->ts, PSI_STREAM_AAC, NULL, 0);
            assert(ctx->atrack > 0);
            printf("audio track: %d\n", ctx->atrack);
        }

        assert(0 == mpeg_ts_write(ctx->ts, ctx->atrack, MPEG_FLAG_IDR_FRAME,
                                  pts, pts, data+7, bytes-7));
        data += n;
        bytes -= n;
        pts += 1024 * 1000 / ctx->aac.sampling_frequency;
    }
}

static int
h264_write(struct mpeg_ts_enc_test_t *ctx, const void *data, int bytes)
{
    int vcl    = 0;
    int update = 0;
    int n = h264_annexbtomp4(&ctx->avc, data, bytes, s_packet, sizeof(s_packet),
                             &vcl, &update);
    if (ctx->vtrack < 0) {
        if (ctx->avc.nb_sps < 1 || ctx->avc.nb_pps < 1) {
            printf("waiting sps/pps\n");
            return -2;
        }

        printf("nb_sps: %d nb_pps: %d\n", ctx->avc.nb_sps, ctx->avc.nb_pps);
        int extra_data_size = mpeg4_avc_decoder_configuration_record_save(
            &ctx->avc, s_extra_data, sizeof(s_extra_data));
        if (extra_data_size <= 0) {
            assert(0);
            return -1;
        }


        printf("extradata: ");
        for (int i = 0; i < extra_data_size; i++) {
            printf("0x%02x ", s_extra_data[i]);
        }
        printf("\n");

        /* assert(0 == mpeg_ts_add_program(ctx->ts, 1, NULL, 0));
         * ctx->vtrack =
         *     mpeg_ts_add_program_stream(ctx->ts, 1, PSI_STREAM_H264, NULL, 0); */
        ctx->vtrack = mpeg_ts_add_stream(ctx->ts, PSI_STREAM_H264, NULL, 0);
        if (ctx->vtrack < 0) {
            assert(0);
            return -1;
        }
        printf("video track: %d\n", ctx->vtrack);
    }

    /* printf("write %d%s\n",  n, 1 == vcl ? "  [I]" : ""); */
    assert(0 == mpeg_ts_write(ctx->ts, ctx->vtrack,
                            1 == vcl ? MPEG_FLAG_IDR_FRAME : 0, ctx->pts * 90,
                              ctx->dts * 90, data, bytes));
    ctx->pts += 40;
    ctx->dts += 40;
    return 0;
}

static void
h264_handler(void *param, const uint8_t *nalu, int bytes)
{
    struct mpeg_ts_enc_test_t *ctx = (struct mpeg_ts_enc_test_t *)param;
    assert(ctx->vptr < nalu);

    const uint8_t *ptr = nalu - 3;
    uint8_t nalutype   = nalu[0] & 0x1f;
    printf("type: %d bytes: %d\n", nalutype, bytes);

    if (ctx->vcl > 0 && h264_is_new_access_unit(nalu, bytes)) {
        int r = h264_write(ctx, ctx->vptr, ptr - ctx->vptr);
        if (r == -1)
            return; // wait for more data

        ctx->vptr = ptr;
        ctx->vcl = 0;
    }

    if (1 <= nalutype && nalutype <= 5)
        ++ctx->vcl;
}

void
mpeg_ts_enc_test(const char *h264, const char *aac)
{
    struct mpeg_ts_enc_test_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.vtrack = -1;
    ctx.atrack = -1;

    char output[256] = {0};
    snprintf(output, sizeof(output), "%s.ts", h264);

    struct mpeg_ts_func_t muxer;
    muxer.alloc = ts_alloc;
    muxer.write = ts_write;
    muxer.free  = ts_free;

    FILE *fp = fopen(output, "wb");
    ctx.ts   = mpeg_ts_create(&muxer, fp);
    assert(ctx.ts);

    long bytes   = 0;

    uint8_t *ptr = file_read(h264, &bytes);
    assert(ptr);
    ctx.vptr = ptr;
    mpeg4_h264_annexb_nalu(ptr, bytes, h264_handler, &ctx);

    uint8_t *aptr = file_read(aac, &bytes);
    assert(aptr);
    adts_reader(&ctx, aptr, bytes);

    mpeg_ts_destroy(ctx.ts);
    free(ptr);
    free(aptr);
    fclose(fp);
}
