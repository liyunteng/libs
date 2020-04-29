/*
 * md5.h - md5
 *
 * Date   : 2020/04/25
 */
#ifndef MD5_H
#define MD5_H

#define MD5_DIGEST_CHARS    16
#define MD5_DIGEST_INTS     (MD5_DIGEST_CHARS / sizeof(unsigned int))

typedef union {
    unsigned char c[MD5_DIGEST_CHARS];
    unsigned int i[MD5_DIGEST_INTS];
} md5_u;

typedef struct {
    unsigned int state[4];      /* state (ABCD) */
    unsigned int count[2];      /* number of bits, module 2^64 (lsb first) */
    unsigned char buffer[64];   /* input buffer */
} MD5_CTX;

void MD5Init(MD5_CTX *ctx);
void MD5Update(MD5_CTX *ctx, const unsigned char *input, unsigned int inputLen);
void MD5Final(MD5_CTX *ctx, unsigned char digest[16]);
#endif
