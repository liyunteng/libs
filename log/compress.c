/*
 * compress.c - compress
 *
 * Date   : 2021/04/15
 */
#include "log_priv.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#define CHUNK 4096
int
log_gzcompress(const char *src_file, const char *dst_file)
{
    FILE *src_fp             = NULL;
    gzFile dst_fp             = NULL;
    unsigned char buf[CHUNK] = {0};
    int n;

    src_fp = fopen(src_file, "rb");
    if (!src_fp) {
        ERROR_LOG("fopen %s failed: (%s)\n", src_file, strerror(errno));
        goto failed;
    }

    dst_fp = gzopen(dst_file, "wb");
    if (!dst_fp) {
        ERROR_LOG("fopen %s failed: (%s)\n", dst_file, strerror(errno));
        goto failed;
    }

    while((n = fread(buf, 1, CHUNK, src_fp)) > 0) {
        if (gzwrite(dst_fp, buf, n) != n) {
            ERROR_LOG("gzwrite failed\n");
            goto failed;
        }
    }

    ERROR_LOG("%s => %s\n", src_file, dst_file);
    unlink(src_file);
    fclose(src_fp);
    gzclose(dst_fp);
    return 0;

failed:
    if (src_fp) {
        fclose(src_fp);
        src_fp = NULL;
    }
    if (dst_fp) {
        gzclose(dst_fp);
        dst_fp = NULL;
    }
    return -1;
}
