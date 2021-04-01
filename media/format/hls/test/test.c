/*
 * test.c - test
 *
 * Date   : 2021/04/02
 */
#include <stdio.h>

extern void hls_parser_test(const char *m3u8);
extern void hls_fmp4_test(const char *mp4);
extern void hls_media_test(const char *ts);
int main(void)
{
    /* hls_parser_test("/home/lyt/master.m3u8"); */
    /* hls_parser_test("/home/lyt/playlist.m3u8"); */
    /* hls_fmp4_test("/home/lyt/abc.mp4"); */
    hls_media_test("/home/lyt/abc.ts");
    return 0;
}
