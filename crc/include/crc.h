/*
 * crc.h - crc
 *
 * Date   : 2020/04/25
 */
#ifndef __CRC32_H__
#define __CRC32_H__

#ifdef __cplusplus
extern "C" {
#endif

unsigned int CRC32MPEG2(unsigned char *data, int length);
unsigned int CRC32(unsigned crcinit, const unsigned char *data, int size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
