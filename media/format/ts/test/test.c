/*
 * test.c - test
 *
 * Date   : 2021/03/30
 */
#include "mov-buffer.h"

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>


static int mov_file_read(void* fp, void* data, uint64_t bytes)
{
    if (bytes == fread(data, 1, bytes, (FILE*)fp))
        return 0;
	return 0 != ferror((FILE*)fp) ? ferror((FILE*)fp) : -1 /*EOF*/;
}

static int mov_file_write(void* fp, const void* data, uint64_t bytes)
{
	return bytes == fwrite(data, 1, bytes, (FILE*)fp) ? 0 : ferror((FILE*)fp);
}

static int mov_file_seek(void* fp, uint64_t offset)
{
	return fseek((FILE*)fp, offset, SEEK_SET);
}

static uint64_t mov_file_tell(void* fp)
{
	return ftell((FILE*)fp);
}

const struct mov_buffer_t* mov_file_buffer(void)
{
	static struct mov_buffer_t s_io = {
		mov_file_read,
		mov_file_write,
		mov_file_seek,
		mov_file_tell,
	};
	return &s_io;
}


extern void mpeg_ts_test(const char *input);
extern void mpeg_ts_multi_program_test(const char *mp4);
extern void mpeg_ts_dec_test(const char *file);
extern void mpeg_ts_enc_test(const char *h264, const char *aac);

int main(void)
{

    /* mpeg_ts_test("/home/lyt/abc.ts"); */
    /* mpeg_ts_dec_test("/home/lyt/abc.ts"); */
    /* mpeg_ts_multi_program_test("/home/lyt/abc.mp4"); */
    mpeg_ts_enc_test("/home/lyt/abc.h264", "/home/lyt/abc.aac");
    mpeg_ts_test("/home/lyt/abc.h264.ts");

    return 0;
}
