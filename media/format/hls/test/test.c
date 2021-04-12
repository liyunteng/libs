/*
 * test.c - test
 *
 * Date   : 2021/04/02
 */
#include <stdio.h>

extern void hls_parser_test(const char *m3u8);
extern void hls_fmp4_test(const char *mp4, const char *m3u8);
extern void hls_media_test(const char *ts, const char *m3u8);
int main(int argc, char *argv[])
{
    /* hls_parser_test("/home/lyt/master.m3u8"); */
    /* hls_parser_test("/home/lyt/playlist.m3u8"); */
    /* hls_fmp4_test("/home/lyt/abc.mp4", "fmp4.m3u8"); */
    if (argc < 3) {
        printf("%s: <ts file> <output m3u8 file>\n", argv[0]);
        return -1;
    }
    hls_media_test(argv[1], argv[2]);
    return 0;
}
