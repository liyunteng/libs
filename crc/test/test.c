/*
 * test.c - test
 *
 * Date   : 2020/04/25
 */

#include "crc.h"
#include <stdio.h>
typedef struct {
    unsigned char *data;
    int data_len;
    unsigned int out;
} crc_test_pair;
int
main(void)
{
    unsigned char d0[] = {0x00};
    // 0x4e, 0x08, 0xbf, 0xb4

    unsigned char d1[] = {0x00, 0xb0, 0x0d, 0x59, 0x81, 0xeb, 0x00, 0x00, 0x00, 0x01, 0xe0, 0x42};
    //0x5e, 0x44, 0x05, 0x9a

    unsigned char d2[] = {
        0x02, 0xb0, 0x17, 0x00, 0x01, 0xc1, 0x00, 0x00, 0xe1, 0x01, 0xf0,
        0x00, 0x03, 0xe1, 0x00, 0xf0, 0x00, 0x1b, 0xe1, 0x01, 0xf0, 0x00,
    };
    //0x5c, 0x45, 0xfc, 0x3d

    unsigned char d3[] = {
        0x02, 0xb0, 0x29, 0x00, 0x01, 0xc1, 0x00, 0x00, 0xf0, 0x01, 0xf0,
        0x0c, 0x05, 0x04, 0x48, 0x44, 0x4d, 0x56, 0x88, 0x04, 0x0f, 0xff,
        0xfc, 0xfc, 0x1b, 0xf0, 0x11, 0xf0, 0x06, 0x28, 0x04, 0x4d, 0x40,
        0x28, 0xbf, 0x0f, 0xf1, 0x00, 0xf0, 0x00,
    };
    // 0x8c, 0x2e, 0x11, 0xfd

    unsigned char d4[] = {
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x61, 0x62, 0x63, 0x64, 0x65, 0x66
    };

    unsigned char d5[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f', '\n',
    };

    crc_test_pair t[] = {
        {d0, sizeof(d0)/sizeof(d0[0]), 0x4E08BFB4},
        {d1, sizeof(d1)/sizeof(d1[0]), 0x5E44059A},
        {d2, sizeof(d2)/sizeof(d2[0]), 0x5C45FC3D},
        {d3, sizeof(d3)/sizeof(d3[0]), 0x8C2E11FD},
        {d4, sizeof(d4)/sizeof(d4[0]), 0xDBD233B1},
        {d5, sizeof(d5)/sizeof(d5[0]), 0xC769E75E},
    };
    crc_test_pair t1[] = {
        {d0, sizeof(d0)/sizeof(d0[0]), 0xD202EF8D},
        {d1, sizeof(d1)/sizeof(d1[0]), 0x56090ABF},
        {d2, sizeof(d2)/sizeof(d2[0]), 0x2369A5E5},
        {d3, sizeof(d3)/sizeof(d3[0]), 0x92007194},
        {d4, sizeof(d4)/sizeof(d4[0]), 0x68C4F033},
        {d5, sizeof(d5)/sizeof(d5[0]), 0x8D6FA375},
    };


    printf("CRC32MPEG2:\n");
    for (size_t i = 0; i < sizeof(t)/sizeof(t[0]); i++) {
        unsigned int crc = 0;
        crc = CRC32MPEG2(t[i].data, t[i].data_len);
        if (crc != t[i].out) {
            printf("failed crc: 0x%X should 0x%X\n", crc, t[i].out);
        } else {
            printf("success crc: 0x%X\n", crc);
        }
    }

    printf("CRC32:\n");
    for (size_t i = 0; i < sizeof(t1)/sizeof(t1[0]); i++) {
        unsigned int crc = 0;
        crc = CRC32(0, t1[i].data, t1[i].data_len);
        if (crc != t1[i].out) {
            printf("failed crc: 0x%X should 0x%X\n", crc, t1[i].out);
        } else {
            printf("success crc: 0x%X\n", crc);
        }
    }
    return 0;
}
