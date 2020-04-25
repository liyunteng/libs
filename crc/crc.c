/*
 * crc.c - crc
 *
 * Date   : 2020/04/25
 */
#include <stdio.h>
#include <assert.h>
#include "crc.h"

#ifndef bool
#define bool unsigned char
#endif // bool

#ifndef false
#define false 0
#endif // false

#ifndef true
#define true 1
#endif // true


unsigned int
CRCGet(unsigned char *data, int length)
{
    unsigned char i = 0, j = 0;
    unsigned int uVal = 0xffffffff;
    unsigned int upoly = 0x02608edb;
    unsigned int utemp = 0, bit = 0;

    assert(data != NULL);

    for (i = 0; i < length; i++) {
        for (j = 0; j < 8; j++) {
            bit = ((data[i] << j) & 0x80) >> 7;
            utemp = ((uVal & 0x80000000) >> 31) ^ bit;
            if (utemp == 1) {
                uVal ^= upoly;
            }
            uVal <<= 1;
            uVal |= utemp;
        }
    }

    return uVal;
}
