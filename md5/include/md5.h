/*
 * md5.h - md5
 *
 * Date   : 2020/04/25
 */
#ifndef __MD5_H__
#define __MD5_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MD5_DIGEST_CHARS    16
#define MD5_DIGEST_INTS     (MD5_DIGEST_CHARS / sizeof(unsigned int))

typedef union {
    unsigned char c[MD5_DIGEST_CHARS];
    unsigned int i[MD5_DIGEST_INTS];
} md5_u;

typedef struct md5_ctx {
    unsigned int state[4];      /* state (ABCD) */
    unsigned int count[2];      /* number of bits, module 2^64 (lsb first) */
    unsigned char buffer[64];   /* input buffer */
} md5_ctx_t;


/// MD5 initialization. Begins an MD5 operation, writing a new context
void md5_init(md5_ctx_t *ctx);

/// MD5 block update operation. Continues an MD5 message-digest
///   operation, processing another message block, and updating the
///   context.
void md5_update(md5_ctx_t *ctx, const unsigned char *input, unsigned int inputLen);


/// MD5 finalization. Ends an MD5 message-digest operation, writing the
///   message digest and zeroizing the context.
void md5_final(md5_ctx_t *ctx, unsigned char digest[16]);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
