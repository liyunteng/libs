/*
 * hls-parser-test.c - hls-parser-test
 *
 * Date   : 2021/04/02
 */
#include "hls-parser.h"
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

void hls_parser_test(const char* m3u8)
{
	static char data[2 * 1024 * 1024];
	FILE* fp = fopen(m3u8, "rb");
	int n = (int)fread(data, 1, sizeof(data), fp);
	fclose(fp);

    int i, j;
	int v = hls_parser_probe(data, n);
	if (HLS_M3U8_MASTER == v)
	{
		struct hls_master_t* master;
		assert(0 == hls_master_parse(&master, data, n));
        printf("version: %d\n", master->version);
        printf("variant_count: %lu\n", master->variant_count);
        for (i = 0; i < master->variant_count; i++) {
            printf("\t[%d] bandwidth: %u\n", i,  master->variants[i].bandwidth);
            printf("\t[%d] average_bandwidth: %u\n", i, master->variants[i].average_bandwidth);
            printf("\t[%d] width: %d\n", i, master->variants[i].width);
            printf("\t[%d] height: %d\n", i, master->variants[i].height);
            printf("\t[%d] fps: %g\n", i, master->variants[i].fps);
            printf("\t[%d] hdcp_level: %s\n", i,  master->variants[i].hdcp_level);
            printf("\t[%d] uri: %s\n", i, master->variants[i].uri);
            printf("\t[%d] codecs: %s\n", i, master->variants[i].codecs);
            printf("\t[%d] video_range: %s\n", i, master->variants[i].video_range);
            printf("\t[%d] audio: %s\n", i, master->variants[i].audio);
            printf("\t[%d] video: %s\n", i, master->variants[i].video);
            printf("\t[%d] subtitle: %s\n", i, master->variants[i].subtitle);
            printf("\t[%d] closed_captions: %s\n", i, master->variants[i].closed_captions);
            printf("\n");
        }

        printf("media_count: %lu\n", master->media_count);
        for (i = 0; i < master->media_count; i++) {
            printf("\t[%d] type: %s\n", i, master->medias[i].type);
            printf("\t[%d] uri: %s\n", i, master->medias[i].uri);
            printf("\t[%d] group_id: %s\n", i, master->medias[i].group_id);
            printf("\t[%d] language: %s\n", i, master->medias[i].language);
            printf("\t[%d] assoc_language: %s\n", i, master->medias[i].assoc_language);
            printf("\t[%d] name: %s\n", i, master->medias[i].name);
            printf("\t[%d] instream_id: %s\n", i, master->medias[i].instream_id);
            printf("\t[%d] characteristics: %s\n", i, master->medias[i].characteristics);
            printf("\t[%d] channels: %s\n", i, master->medias[i].channels);
            printf("\t[%d] autoselect: %d\n", i, master->medias[i].autoselect);
            printf("\t[%d] is_default: %d\n", i, master->medias[i].is_default);
            printf("\t[%d] forced: %d\n", i, master->medias[i].forced);
            printf("\n");
        }

        printf("session_data_count: %lu\n", master->session_data_count);
        for (i = 0; i < master->session_data_count; i++) {
            printf("\t[%d] data_id: %s\n", i, master->session_data[i].data_id);
            printf("\t[%d] value: %s\n", i, master->session_data[i].value);
            printf("\t[%d] uri: %s\n", i, master->session_data[i].uri);
            printf("\t[%d] language: %s\n", i, master->session_data[i].language);
            printf("\n");
        }

        printf("session_key_count: %lu\n", master->session_key_count);
        for (i = 0; i < master->session_key_count; i++) {
            printf("\t[%d] method: %s\n", i, master->session_key[i].method);
            printf("\t[%d] uri: %s\n", i, master->session_key[i].uri);
            printf("\t[%d] keyformat: %s\n", i, master->session_key[i].keyformat);
            printf("\t[%d] keyformatversions: %s\n", i, master->session_key[i].keyformatversions);
            printf("\t[%d] iv: ", i);
            for (j = 0; j < 16; j++) {
                printf("0x%02x", master->session_key[i].iv[j]);
            }
            printf("\n");
            printf("\n");
        }
        printf("independent_segments: %d\n", master->independent_segments);
        printf("start_time_offset: %g\n", master->start_time_offset);
        printf("start_precise: %d\n", master->start_precise);
		hls_master_free(&master);
	}
	else if (HLS_M3U8_PLAYLIST == v)
	{
		struct hls_playlist_t* playlist;
		assert(0 == hls_playlist_parse(&playlist, data, n));
        printf("version: %d\n", playlist->version);
        printf("target_duration: %"PRIu64"\n", playlist->target_duration);
        printf("media_sequence: %"PRIu64"\n", playlist->media_sequence);
        printf("discontinuity_sequence: %"PRIu64"\n", playlist->discontinuity_sequence);
        printf("endlist: %d\n", playlist->endlist);
        printf("type: %d\n", playlist->type);
        printf("i_frames_only: %d\n", playlist->i_frames_only);
        printf("independent_segments: %d\n", playlist->independent_segments);
        printf("start_time_offset: %g\n", playlist->start_time_offset);
        printf("start_precise: %d\n", playlist->start_precise);
        printf("count: %lu\n", playlist->count);
        for (i = 0; i < playlist->count; i++) {
            printf("\t[%d] duration: %g\n", i, playlist->segments[i].duration);
            printf("\t[%d] uri: %s\n", i, playlist->segments[i].uri);
            printf("\t[%d] title: %s\n", i, playlist->segments[i].title);
            printf("\t[%d] bytes: %"PRId64"\n", i, playlist->segments[i].bytes);
            printf("\t[%d] offset: %"PRId64"\n", i, playlist->segments[i].offset);
            printf("\t[%d] discontinuity: %d\n", i, playlist->segments[i].discontinuity);
            printf("\t[%d] key.method: %s\n", i, playlist->segments[i].key.method);
            printf("\t[%d] key.uri: %s\n", i, playlist->segments[i].key.uri);
            printf("\t[%d] key.keyformat: %s\n", i, playlist->segments[i].key.keyformat);
            printf("\t[%d] key.keyformatversions: %s\n", i, playlist->segments[i].key.keyformatversions);
            printf("\t[%d] key.iv: ", i);
            for (j = 0; j < 16; j++) {
                printf("0x%02x", playlist->segments[i].key.iv[j]);
            }
            printf("\n");

            printf("\t[%d] map.uri: %s\n", i, playlist->segments[i].map.uri);
            printf("\t[%d] map.bytes: %"PRId64"\n", i, playlist->segments[i].map.bytes);
            printf("\t[%d] map.offset: %"PRId64"\n", i, playlist->segments[i].map.offset);

            printf("\t[%d] program_date_time: %s\n", i, playlist->segments[i].program_date_time);

            printf("\t[%d] daterange.id: %s\n", i, playlist->segments[i].daterange.id);
            printf("\t[%d] daterange.cls: %s\n", i, playlist->segments[i].daterange.cls);
            printf("\t[%d] daterange.start_date: %s\n", i, playlist->segments[i].daterange.start_date);
            printf("\t[%d] daterange.end_date: %s\n", i, playlist->segments[i].daterange.end_date);
            printf("\t[%d] daterange.duration: %g\n", i, playlist->segments[i].daterange.duration);
            printf("\t[%d] daterange.planned_duration: %g\n", i, playlist->segments[i].daterange.planned_duration);
            printf("\t[%d] daterange.x_client_attribute: %s\n", i, playlist->segments[i].daterange.x_client_attribute);
            printf("\t[%d] daterange.end_on_next: %d\n", i, playlist->segments[i].daterange.end_on_next);
            printf("\n");
        }
		hls_playlist_free(&playlist);
	}
	else
	{
		assert(0);
	}
}
