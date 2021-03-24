/*
 * test.c - test
 *
 * Date   : 2021/03/24
 */

extern void fmp4_writer_test2(const char *mp4, const char *outmp4);
extern void mov_reader_test(const char *mp4);
extern void mov_writer_adts_test(const char *mp4, const char *outmp4);
extern void mov_writer_audio(const char *audio, int type, const char *mp4);
extern void mov_writer_h264(const char *h264, int width, int height,
                            const char *mp4);
extern void mov_writer_h265(const char *h265, int width, int height,
                            const char *mp4);
extern void mov_writer_subtitle(const char *mp4, const char *outmp4);
extern void mov_writer_test(int w, int h, const char *inflv,
                            const char *outmp4);

#define MP4 "/Users/lyt/abc.mp4"
int
main(void)
{
    fmp4_writer_test2(MP4, "fmp4_writer.mp4");
    mov_reader_test(MP4);
    mov_writer_adts_test("a.aac", "adts.mp4");
    mov_writer_audio("a.aac", 1, "audio.aac");
    mov_writer_h264("v.h264", 1280, 720, "out.mp4");
    return 0;
}
