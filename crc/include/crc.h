/*
 * crc.h - crc
 *
 * Date   : 2020/04/25
 */
#ifndef __CRC32_H__
#define __CRC32_H__

unsigned int CRC32MPEG2(unsigned char *data, int length);
unsigned int CRC32(unsigned crcinit, const unsigned char *data, int size);

#endif
