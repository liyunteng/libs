/*
 * test.c - test
 *
 * Date   : 2021/04/02
 */
#include <stdio.h>

extern void avc2flv_test(const char *inputH264, const char *outputFLV);


int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("%s: <h264 file> <out file>\n", argv[0]);
        return -1;
    }
    avc2flv_test(argv[1], argv[2]);
    return 0;
}
