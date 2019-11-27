/*
 * aes.c - aes
 *
 * Author : liyunteng <liyunteng@streamocean.com>
 * Date   : 2019/08/25
 *
 * Copyright (C) 2019 StreamOcean, Inc.
 * All rights reserved.
 */
#include "aes.h"
#include <string.h>

/* #define  POLARSSL_SELF_TEST 1 */

/*
 * 32-bit integer manipulation macros (little endian)
 */
#ifndef GET_ULONG_LE
#    define GET_ULONG_LE(n, b, i)                                              \
        {                                                                      \
            (n) = ((unsigned long)(b)[(i)])                                    \
                  | ((unsigned long)(b)[(i) + 1] << 8)                         \
                  | ((unsigned long)(b)[(i) + 2] << 16)                        \
                  | ((unsigned long)(b)[(i) + 3] << 24);                       \
        }
#endif

#ifndef PUT_ULONG_LE
#    define PUT_ULONG_LE(n, b, i)                                              \
        {                                                                      \
            (b)[(i)]     = (unsigned char)((n));                               \
            (b)[(i) + 1] = (unsigned char)((n) >> 8);                          \
            (b)[(i) + 2] = (unsigned char)((n) >> 16);                         \
            (b)[(i) + 3] = (unsigned char)((n) >> 24);                         \
        }
#endif

#if defined(POLARSSL_AES_ROM_TABLES)
/*
 * Forward S-box
 */
#else

/*
 * Forward S-box & tables
 */
static unsigned char FSb[256];
static unsigned long FT0[256];
static unsigned long FT1[256];
static unsigned long FT2[256];
static unsigned long FT3[256];

/*
 * Reverse S-box & tables
 */
static unsigned char RSb[256];
static unsigned long RT0[256];
static unsigned long RT1[256];
static unsigned long RT2[256];
static unsigned long RT3[256];

/*
 * Round constants
 */
static unsigned long RCON[10];

/*
 * Tables generation code
 */
#    define ROTL8(x) ((x << 8) & 0xFFFFFFFF) | (x >> 24)
#    define XTIME(x) ((x << 1) ^ ((x & 0x80) ? 0x1B : 0x00))
#    define MUL(x, y) ((x && y) ? pow[(log[x] + log[y]) % 255] : 0)

static int aes_init_done = 0;

static void
aes_gen_tables(void)
{
    int i, x, y, z;
    int pow[256];
    int log[256];

    /*
     * compute pow and log tables over GF(2^8)
     */
    for (i = 0, x = 1; i < 256; i++) {
        pow[i] = x;
        log[x] = i;
        x      = (x ^ XTIME(x)) & 0xFF;
    }

    /*
     * calculate the round constants
     */
    for (i = 0, x = 1; i < 10; i++) {
        RCON[i] = (unsigned long)x;
        x       = XTIME(x) & 0xFF;
    }

    /*
     * generate the forward and reverse S-boxes
     */
    FSb[0x00] = 0x63;
    RSb[0x63] = 0x00;

    for (i = 1; i < 256; i++) {
        x = pow[255 - log[i]];

        y = x;
        y = ((y << 1) | (y >> 7)) & 0xFF;
        x ^= y;
        y = ((y << 1) | (y >> 7)) & 0xFF;
        x ^= y;
        y = ((y << 1) | (y >> 7)) & 0xFF;
        x ^= y;
        y = ((y << 1) | (y >> 7)) & 0xFF;
        x ^= y ^ 0x63;

        FSb[i] = (unsigned char)x;
        RSb[x] = (unsigned char)i;
    }

    /*
     * generate the forward and reverse tables
     */
    for (i = 0; i < 256; i++) {
        x = FSb[i];
        y = XTIME(x) & 0xFF;
        z = (y ^ x) & 0xFF;

        FT0[i] = ((unsigned long)y) ^ ((unsigned long)x << 8)
                 ^ ((unsigned long)x << 16) ^ ((unsigned long)z << 24);

        FT1[i] = ROTL8(FT0[i]);
        FT2[i] = ROTL8(FT1[i]);
        FT3[i] = ROTL8(FT2[i]);

        x = RSb[i];

        RT0[i] = ((unsigned long)MUL(0x0E, x))
                 ^ ((unsigned long)MUL(0x09, x) << 8)
                 ^ ((unsigned long)MUL(0x0D, x) << 16)
                 ^ ((unsigned long)MUL(0x0B, x) << 24);

        RT1[i] = ROTL8(RT0[i]);
        RT2[i] = ROTL8(RT1[i]);
        RT3[i] = ROTL8(RT2[i]);
    }
}

#endif

/*
 * AES key schedule (encryption)
 */
void
aes_setkey_enc(aes_context *ctx, unsigned char *key, int keysize)
{
    int i;
    unsigned long *RK;

#if !defined(POLARSSL_AES_ROM_TABLES)
    if (aes_init_done == 0) {
        aes_gen_tables();
        aes_init_done = 1;
    }
#endif

    switch (keysize) {
    case 128:
        ctx->nr = 10;
        break;
    case 192:
        ctx->nr = 12;
        break;
    case 256:
        ctx->nr = 14;
        break;
    default:
        return;
    }

#if defined(PADLOCK_ALIGN16)
    ctx->rk = RK = PADLOCK_ALIGN16(ctx->buf);
#else
    ctx->rk = RK = ctx->buf;
#endif

    for (i = 0; i < (keysize >> 5); i++) {
        GET_ULONG_LE(RK[i], key, i << 2);
    }

    switch (ctx->nr) {
    case 10:

        for (i = 0; i < 10; i++, RK += 4) {
            RK[4] = RK[0] ^ RCON[i] ^ ((unsigned long)FSb[(RK[3] >> 8) & 0xFF])
                    ^ ((unsigned long)FSb[(RK[3] >> 16) & 0xFF] << 8)
                    ^ ((unsigned long)FSb[(RK[3] >> 24) & 0xFF] << 16)
                    ^ ((unsigned long)FSb[(RK[3]) & 0xFF] << 24);

            RK[5] = RK[1] ^ RK[4];
            RK[6] = RK[2] ^ RK[5];
            RK[7] = RK[3] ^ RK[6];
        }
        break;

    case 12:

        for (i = 0; i < 8; i++, RK += 6) {
            RK[6] = RK[0] ^ RCON[i] ^ ((unsigned long)FSb[(RK[5] >> 8) & 0xFF])
                    ^ ((unsigned long)FSb[(RK[5] >> 16) & 0xFF] << 8)
                    ^ ((unsigned long)FSb[(RK[5] >> 24) & 0xFF] << 16)
                    ^ ((unsigned long)FSb[(RK[5]) & 0xFF] << 24);

            RK[7]  = RK[1] ^ RK[6];
            RK[8]  = RK[2] ^ RK[7];
            RK[9]  = RK[3] ^ RK[8];
            RK[10] = RK[4] ^ RK[9];
            RK[11] = RK[5] ^ RK[10];
        }
        break;

    case 14:

        for (i = 0; i < 7; i++, RK += 8) {
            RK[8] = RK[0] ^ RCON[i] ^ ((unsigned long)FSb[(RK[7] >> 8) & 0xFF])
                    ^ ((unsigned long)FSb[(RK[7] >> 16) & 0xFF] << 8)
                    ^ ((unsigned long)FSb[(RK[7] >> 24) & 0xFF] << 16)
                    ^ ((unsigned long)FSb[(RK[7]) & 0xFF] << 24);

            RK[9]  = RK[1] ^ RK[8];
            RK[10] = RK[2] ^ RK[9];
            RK[11] = RK[3] ^ RK[10];

            RK[12] = RK[4] ^ ((unsigned long)FSb[(RK[11]) & 0xFF])
                     ^ ((unsigned long)FSb[(RK[11] >> 8) & 0xFF] << 8)
                     ^ ((unsigned long)FSb[(RK[11] >> 16) & 0xFF] << 16)
                     ^ ((unsigned long)FSb[(RK[11] >> 24) & 0xFF] << 24);

            RK[13] = RK[5] ^ RK[12];
            RK[14] = RK[6] ^ RK[13];
            RK[15] = RK[7] ^ RK[14];
        }
        break;

    default:

        break;
    }
}

/*
 * AES key schedule (decryption)
 */
void
aes_setkey_dec(aes_context *ctx, unsigned char *key, int keysize)
{
    int i, j;
    aes_context cty;
    unsigned long *RK;
    unsigned long *SK;

    switch (keysize) {
    case 128:
        ctx->nr = 10;
        break;
    case 192:
        ctx->nr = 12;
        break;
    case 256:
        ctx->nr = 14;
        break;
    default:
        return;
    }

#if defined(PADLOCK_ALIGN16)
    ctx->rk = RK = PADLOCK_ALIGN16(ctx->buf);
#else
    ctx->rk = RK = ctx->buf;
#endif

    aes_setkey_enc(&cty, key, keysize);
    SK = cty.rk + cty.nr * 4;

    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;

    for (i = ctx->nr - 1, SK -= 8; i > 0; i--, SK -= 8) {
        for (j = 0; j < 4; j++, SK++) {
            *RK++ = RT0[FSb[(*SK) & 0xFF]] ^ RT1[FSb[(*SK >> 8) & 0xFF]]
                    ^ RT2[FSb[(*SK >> 16) & 0xFF]]
                    ^ RT3[FSb[(*SK >> 24) & 0xFF]];
        }
    }

    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;
    *RK++ = *SK++;

    memset(&cty, 0, sizeof(aes_context));
}

#define AES_FROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3)                             \
    {                                                                          \
        X0 = *RK++ ^ FT0[(Y0)&0xFF] ^ FT1[(Y1 >> 8) & 0xFF]                    \
             ^ FT2[(Y2 >> 16) & 0xFF] ^ FT3[(Y3 >> 24) & 0xFF];                \
                                                                               \
        X1 = *RK++ ^ FT0[(Y1)&0xFF] ^ FT1[(Y2 >> 8) & 0xFF]                    \
             ^ FT2[(Y3 >> 16) & 0xFF] ^ FT3[(Y0 >> 24) & 0xFF];                \
                                                                               \
        X2 = *RK++ ^ FT0[(Y2)&0xFF] ^ FT1[(Y3 >> 8) & 0xFF]                    \
             ^ FT2[(Y0 >> 16) & 0xFF] ^ FT3[(Y1 >> 24) & 0xFF];                \
                                                                               \
        X3 = *RK++ ^ FT0[(Y3)&0xFF] ^ FT1[(Y0 >> 8) & 0xFF]                    \
             ^ FT2[(Y1 >> 16) & 0xFF] ^ FT3[(Y2 >> 24) & 0xFF];                \
    }

#define AES_RROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3)                             \
    {                                                                          \
        X0 = *RK++ ^ RT0[(Y0)&0xFF] ^ RT1[(Y3 >> 8) & 0xFF]                    \
             ^ RT2[(Y2 >> 16) & 0xFF] ^ RT3[(Y1 >> 24) & 0xFF];                \
                                                                               \
        X1 = *RK++ ^ RT0[(Y1)&0xFF] ^ RT1[(Y0 >> 8) & 0xFF]                    \
             ^ RT2[(Y3 >> 16) & 0xFF] ^ RT3[(Y2 >> 24) & 0xFF];                \
                                                                               \
        X2 = *RK++ ^ RT0[(Y2)&0xFF] ^ RT1[(Y1 >> 8) & 0xFF]                    \
             ^ RT2[(Y0 >> 16) & 0xFF] ^ RT3[(Y3 >> 24) & 0xFF];                \
                                                                               \
        X3 = *RK++ ^ RT0[(Y3)&0xFF] ^ RT1[(Y2 >> 8) & 0xFF]                    \
             ^ RT2[(Y1 >> 16) & 0xFF] ^ RT3[(Y0 >> 24) & 0xFF];                \
    }

/*
 * AES-ECB block encryption/decryption
 */
void
aes_crypt_ecb(aes_context *ctx, int mode, unsigned char input[16],
              unsigned char output[16])
{
    int i;
    unsigned long *RK, X0, X1, X2, X3, Y0, Y1, Y2, Y3;

#if defined(POLARSSL_PADLOCK_C) && defined(POLARSSL_HAVE_X86)
    if (padlock_supports(PADLOCK_ACE)) {
        if (padlock_xcryptecb(ctx, mode, input, output) == 0)
            return;
    }
#endif

    RK = ctx->rk;

    GET_ULONG_LE(X0, input, 0);
    X0 ^= *RK++;
    GET_ULONG_LE(X1, input, 4);
    X1 ^= *RK++;
    GET_ULONG_LE(X2, input, 8);
    X2 ^= *RK++;
    GET_ULONG_LE(X3, input, 12);
    X3 ^= *RK++;

    if (mode == AES_DECRYPT) {
        for (i = (ctx->nr >> 1) - 1; i > 0; i--) {
            AES_RROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);
            AES_RROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3);
        }

        AES_RROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);

        X0 = *RK++ ^ ((unsigned long)RSb[(Y0)&0xFF])
             ^ ((unsigned long)RSb[(Y3 >> 8) & 0xFF] << 8)
             ^ ((unsigned long)RSb[(Y2 >> 16) & 0xFF] << 16)
             ^ ((unsigned long)RSb[(Y1 >> 24) & 0xFF] << 24);

        X1 = *RK++ ^ ((unsigned long)RSb[(Y1)&0xFF])
             ^ ((unsigned long)RSb[(Y0 >> 8) & 0xFF] << 8)
             ^ ((unsigned long)RSb[(Y3 >> 16) & 0xFF] << 16)
             ^ ((unsigned long)RSb[(Y2 >> 24) & 0xFF] << 24);

        X2 = *RK++ ^ ((unsigned long)RSb[(Y2)&0xFF])
             ^ ((unsigned long)RSb[(Y1 >> 8) & 0xFF] << 8)
             ^ ((unsigned long)RSb[(Y0 >> 16) & 0xFF] << 16)
             ^ ((unsigned long)RSb[(Y3 >> 24) & 0xFF] << 24);

        X3 = *RK++ ^ ((unsigned long)RSb[(Y3)&0xFF])
             ^ ((unsigned long)RSb[(Y2 >> 8) & 0xFF] << 8)
             ^ ((unsigned long)RSb[(Y1 >> 16) & 0xFF] << 16)
             ^ ((unsigned long)RSb[(Y0 >> 24) & 0xFF] << 24);
    } else /* AES_ENCRYPT */
    {
        for (i = (ctx->nr >> 1) - 1; i > 0; i--) {
            AES_FROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);
            AES_FROUND(X0, X1, X2, X3, Y0, Y1, Y2, Y3);
        }

        AES_FROUND(Y0, Y1, Y2, Y3, X0, X1, X2, X3);

        X0 = *RK++ ^ ((unsigned long)FSb[(Y0)&0xFF])
             ^ ((unsigned long)FSb[(Y1 >> 8) & 0xFF] << 8)
             ^ ((unsigned long)FSb[(Y2 >> 16) & 0xFF] << 16)
             ^ ((unsigned long)FSb[(Y3 >> 24) & 0xFF] << 24);

        X1 = *RK++ ^ ((unsigned long)FSb[(Y1)&0xFF])
             ^ ((unsigned long)FSb[(Y2 >> 8) & 0xFF] << 8)
             ^ ((unsigned long)FSb[(Y3 >> 16) & 0xFF] << 16)
             ^ ((unsigned long)FSb[(Y0 >> 24) & 0xFF] << 24);

        X2 = *RK++ ^ ((unsigned long)FSb[(Y2)&0xFF])
             ^ ((unsigned long)FSb[(Y3 >> 8) & 0xFF] << 8)
             ^ ((unsigned long)FSb[(Y0 >> 16) & 0xFF] << 16)
             ^ ((unsigned long)FSb[(Y1 >> 24) & 0xFF] << 24);

        X3 = *RK++ ^ ((unsigned long)FSb[(Y3)&0xFF])
             ^ ((unsigned long)FSb[(Y0 >> 8) & 0xFF] << 8)
             ^ ((unsigned long)FSb[(Y1 >> 16) & 0xFF] << 16)
             ^ ((unsigned long)FSb[(Y2 >> 24) & 0xFF] << 24);
    }

    PUT_ULONG_LE(X0, output, 0);
    PUT_ULONG_LE(X1, output, 4);
    PUT_ULONG_LE(X2, output, 8);
    PUT_ULONG_LE(X3, output, 12);
}

/*
 * AES-CBC buffer encryption/decryption
 */
void
aes_crypt_cbc(aes_context *ctx, int mode, int length, unsigned char iv[16],
              unsigned char *input, unsigned char *output)
{
    int i;
    unsigned char temp[16];

#if defined(POLARSSL_PADLOCK_C) && defined(POLARSSL_HAVE_X86)
    if (padlock_supports(PADLOCK_ACE)) {
        if (padlock_xcryptcbc(ctx, mode, length, iv, input, output) == 0)
            return;
    }
#endif

    if (mode == AES_DECRYPT) {
        while (length > 0) {
            memcpy(temp, input, 16);
            aes_crypt_ecb(ctx, mode, input, output);

            for (i = 0; i < 16; i++)
                output[i] = (unsigned char)(output[i] ^ iv[i]);

            memcpy(iv, temp, 16);

            input += 16;
            output += 16;
            length -= 16;
        }
    } else {
        while (length > 0) {
            for (i = 0; i < 16; i++)
                output[i] = (unsigned char)(input[i] ^ iv[i]);

            aes_crypt_ecb(ctx, mode, output, output);
            memcpy(iv, output, 16);

            input += 16;
            output += 16;
            length -= 16;
        }
    }
}

/*
 * AES-CFB128 buffer encryption/decryption
 */
void
aes_crypt_cfb128(aes_context *ctx, int mode, int length, int *iv_off,
                 unsigned char iv[16], unsigned char *input,
                 unsigned char *output)
{
    int c, n = *iv_off;

    if (mode == AES_DECRYPT) {
        while (length--) {
            if (n == 0)
                aes_crypt_ecb(ctx, AES_ENCRYPT, iv, iv);

            c         = *input++;
            *output++ = (unsigned char)(c ^ iv[n]);
            iv[n]     = (unsigned char)c;

            n = (n + 1) & 0x0F;
        }
    } else {
        while (length--) {
            if (n == 0)
                aes_crypt_ecb(ctx, AES_ENCRYPT, iv, iv);

            iv[n] = *output++ = (unsigned char)(iv[n] ^ *input++);

            n = (n + 1) & 0x0F;
        }
    }

    *iv_off = n;
}

#if defined(POLARSSL_SELF_TEST)

#    include <stdio.h>

/*
 * AES test vectors from:
 *
 * http://csrc.nist.gov/archive/aes/rijndael/rijndael-vals.zip
 */
static const unsigned char aes_test_ecb_dec[3][16] = {
    {0x44, 0x41, 0x6A, 0xC2, 0xD1, 0xF5, 0x3C, 0x58, 0x33, 0x03, 0x91, 0x7E,
     0x6B, 0xE9, 0xEB, 0xE0},
    {0x48, 0xE3, 0x1E, 0x9E, 0x25, 0x67, 0x18, 0xF2, 0x92, 0x29, 0x31, 0x9C,
     0x19, 0xF1, 0x5B, 0xA4},
    {0x05, 0x8C, 0xCF, 0xFD, 0xBB, 0xCB, 0x38, 0x2D, 0x1F, 0x6F, 0x56, 0x58,
     0x5D, 0x8A, 0x4A, 0xDE}};

static const unsigned char aes_test_ecb_enc[3][16] = {
    {0xC3, 0x4C, 0x05, 0x2C, 0xC0, 0xDA, 0x8D, 0x73, 0x45, 0x1A, 0xFE, 0x5F,
     0x03, 0xBE, 0x29, 0x7F},
    {0xF3, 0xF6, 0x75, 0x2A, 0xE8, 0xD7, 0x83, 0x11, 0x38, 0xF0, 0x41, 0x56,
     0x06, 0x31, 0xB1, 0x14},
    {0x8B, 0x79, 0xEE, 0xCC, 0x93, 0xA0, 0xEE, 0x5D, 0xFF, 0x30, 0xB4, 0xEA,
     0x21, 0x63, 0x6D, 0xA4}};

static const unsigned char aes_test_cbc_dec[3][16] = {
    {0xFA, 0xCA, 0x37, 0xE0, 0xB0, 0xC8, 0x53, 0x73, 0xDF, 0x70, 0x6E, 0x73,
     0xF7, 0xC9, 0xAF, 0x86},
    {0x5D, 0xF6, 0x78, 0xDD, 0x17, 0xBA, 0x4E, 0x75, 0xB6, 0x17, 0x68, 0xC6,
     0xAD, 0xEF, 0x7C, 0x7B},
    {0x48, 0x04, 0xE1, 0x81, 0x8F, 0xE6, 0x29, 0x75, 0x19, 0xA3, 0xE8, 0x8C,
     0x57, 0x31, 0x04, 0x13}};

static const unsigned char aes_test_cbc_enc[3][16] = {
    {0x8A, 0x05, 0xFC, 0x5E, 0x09, 0x5A, 0xF4, 0x84, 0x8A, 0x08, 0xD3, 0x28,
     0xD3, 0x68, 0x8E, 0x3D},
    {0x7B, 0xD9, 0x66, 0xD5, 0x3A, 0xD8, 0xC1, 0xBB, 0x85, 0xD2, 0xAD, 0xFA,
     0xE8, 0x7B, 0xB1, 0x04},
    {0xFE, 0x3C, 0x53, 0x65, 0x3E, 0x2F, 0x45, 0xB5, 0x6F, 0xCD, 0x88, 0xB2,
     0xCC, 0x89, 0x8F, 0xF0}};

/*
 * AES-CFB128 test vectors from:
 *
 * http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf
 */
static const unsigned char aes_test_cfb128_key[3][32] = {
    {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88,
     0x09, 0xCF, 0x4F, 0x3C},
    {0x8E, 0x73, 0xB0, 0xF7, 0xDA, 0x0E, 0x64, 0x52, 0xC8, 0x10, 0xF3, 0x2B,
     0x80, 0x90, 0x79, 0xE5, 0x62, 0xF8, 0xEA, 0xD2, 0x52, 0x2C, 0x6B, 0x7B},
    {0x60, 0x3D, 0xEB, 0x10, 0x15, 0xCA, 0x71, 0xBE, 0x2B, 0x73, 0xAE,
     0xF0, 0x85, 0x7D, 0x77, 0x81, 0x1F, 0x35, 0x2C, 0x07, 0x3B, 0x61,
     0x08, 0xD7, 0x2D, 0x98, 0x10, 0xA3, 0x09, 0x14, 0xDF, 0xF4}};

static const unsigned char aes_test_cfb128_iv[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

static const unsigned char aes_test_cfb128_pt[64] = {
    0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96, 0xE9, 0x3D, 0x7E,
    0x11, 0x73, 0x93, 0x17, 0x2A, 0xAE, 0x2D, 0x8A, 0x57, 0x1E, 0x03,
    0xAC, 0x9C, 0x9E, 0xB7, 0x6F, 0xAC, 0x45, 0xAF, 0x8E, 0x51, 0x30,
    0xC8, 0x1C, 0x46, 0xA3, 0x5C, 0xE4, 0x11, 0xE5, 0xFB, 0xC1, 0x19,
    0x1A, 0x0A, 0x52, 0xEF, 0xF6, 0x9F, 0x24, 0x45, 0xDF, 0x4F, 0x9B,
    0x17, 0xAD, 0x2B, 0x41, 0x7B, 0xE6, 0x6C, 0x37, 0x10};

static const unsigned char aes_test_cfb128_ct[3][64] = {
    {0x3B, 0x3F, 0xD9, 0x2E, 0xB7, 0x2D, 0xAD, 0x20, 0x33, 0x34, 0x49,
     0xF8, 0xE8, 0x3C, 0xFB, 0x4A, 0xC8, 0xA6, 0x45, 0x37, 0xA0, 0xB3,
     0xA9, 0x3F, 0xCD, 0xE3, 0xCD, 0xAD, 0x9F, 0x1C, 0xE5, 0x8B, 0x26,
     0x75, 0x1F, 0x67, 0xA3, 0xCB, 0xB1, 0x40, 0xB1, 0x80, 0x8C, 0xF1,
     0x87, 0xA4, 0xF4, 0xDF, 0xC0, 0x4B, 0x05, 0x35, 0x7C, 0x5D, 0x1C,
     0x0E, 0xEA, 0xC4, 0xC6, 0x6F, 0x9F, 0xF7, 0xF2, 0xE6},
    {0xCD, 0xC8, 0x0D, 0x6F, 0xDD, 0xF1, 0x8C, 0xAB, 0x34, 0xC2, 0x59,
     0x09, 0xC9, 0x9A, 0x41, 0x74, 0x67, 0xCE, 0x7F, 0x7F, 0x81, 0x17,
     0x36, 0x21, 0x96, 0x1A, 0x2B, 0x70, 0x17, 0x1D, 0x3D, 0x7A, 0x2E,
     0x1E, 0x8A, 0x1D, 0xD5, 0x9B, 0x88, 0xB1, 0xC8, 0xE6, 0x0F, 0xED,
     0x1E, 0xFA, 0xC4, 0xC9, 0xC0, 0x5F, 0x9F, 0x9C, 0xA9, 0x83, 0x4F,
     0xA0, 0x42, 0xAE, 0x8F, 0xBA, 0x58, 0x4B, 0x09, 0xFF},
    {0xDC, 0x7E, 0x84, 0xBF, 0xDA, 0x79, 0x16, 0x4B, 0x7E, 0xCD, 0x84,
     0x86, 0x98, 0x5D, 0x38, 0x60, 0x39, 0xFF, 0xED, 0x14, 0x3B, 0x28,
     0xB1, 0xC8, 0x32, 0x11, 0x3C, 0x63, 0x31, 0xE5, 0x40, 0x7B, 0xDF,
     0x10, 0x13, 0x24, 0x15, 0xE5, 0x4B, 0x92, 0xA1, 0x3E, 0xD0, 0xA8,
     0x26, 0x7A, 0xE2, 0xF9, 0x75, 0xA3, 0x85, 0x74, 0x1A, 0xB9, 0xCE,
     0xF8, 0x20, 0x31, 0x62, 0x3D, 0x55, 0xB1, 0xE4, 0x71}};

/*
 * Checkup routine
 */
int
aes_self_test(int verbose)
{
    int i, j, u, v, offset;
    unsigned char key[32];
    unsigned char buf[64];
    unsigned char prv[16];
    unsigned char iv[16];
    aes_context ctx;

    memset(key, 0, 32);

    /*
     * ECB mode
     */
    for (i = 0; i < 6; i++) {
        u = i >> 1;
        v = i & 1;

        if (verbose != 0)
            printf("  AES-ECB-%3d (%s): ", 128 + u * 64,
                   (v == AES_DECRYPT) ? "dec" : "enc");

        memset(buf, 0, 16);

        if (v == AES_DECRYPT) {
            aes_setkey_dec(&ctx, key, 128 + u * 64);

            for (j = 0; j < 10000; j++)
                aes_crypt_ecb(&ctx, v, buf, buf);

            if (memcmp(buf, aes_test_ecb_dec[u], 16) != 0) {
                if (verbose != 0)
                    printf("failed\n");

                return (1);
            }
        } else {
            aes_setkey_enc(&ctx, key, 128 + u * 64);

            for (j = 0; j < 10000; j++)
                aes_crypt_ecb(&ctx, v, buf, buf);

            if (memcmp(buf, aes_test_ecb_enc[u], 16) != 0) {
                if (verbose != 0)
                    printf("failed\n");

                return (1);
            }
        }

        if (verbose != 0)
            printf("passed\n");
    }

    if (verbose != 0)
        printf("\n");

    /*
     * CBC mode
     */
    for (i = 0; i < 6; i++) {
        u = i >> 1;
        v = i & 1;

        if (verbose != 0)
            printf("  AES-CBC-%3d (%s): ", 128 + u * 64,
                   (v == AES_DECRYPT) ? "dec" : "enc");

        memset(iv, 0, 16);
        memset(prv, 0, 16);
        memset(buf, 0, 16);

        if (v == AES_DECRYPT) {
            aes_setkey_dec(&ctx, key, 128 + u * 64);

            for (j = 0; j < 10000; j++)
                aes_crypt_cbc(&ctx, v, 16, iv, buf, buf);

            if (memcmp(buf, aes_test_cbc_dec[u], 16) != 0) {
                if (verbose != 0)
                    printf("failed\n");

                return (1);
            }
        } else {
            aes_setkey_enc(&ctx, key, 128 + u * 64);

            for (j = 0; j < 10000; j++) {
                unsigned char tmp[16];

                aes_crypt_cbc(&ctx, v, 16, iv, buf, buf);

                memcpy(tmp, prv, 16);
                memcpy(prv, buf, 16);
                memcpy(buf, tmp, 16);
            }

            if (memcmp(prv, aes_test_cbc_enc[u], 16) != 0) {
                if (verbose != 0)
                    printf("failed\n");

                return (1);
            }
        }

        if (verbose != 0)
            printf("passed\n");
    }

    if (verbose != 0)
        printf("\n");

    /*
     * CFB128 mode
     */
    for (i = 0; i < 6; i++) {
        u = i >> 1;
        v = i & 1;

        if (verbose != 0)
            printf("  AES-CFB128-%3d (%s): ", 128 + u * 64,
                   (v == AES_DECRYPT) ? "dec" : "enc");

        memcpy(iv, aes_test_cfb128_iv, 16);
        memcpy(key, aes_test_cfb128_key[u], 16 + u * 8);

        offset = 0;
        aes_setkey_enc(&ctx, key, 128 + u * 64);

        if (v == AES_DECRYPT) {
            memcpy(buf, aes_test_cfb128_ct[u], 64);
            aes_crypt_cfb128(&ctx, v, 64, &offset, iv, buf, buf);

            if (memcmp(buf, aes_test_cfb128_pt, 64) != 0) {
                if (verbose != 0)
                    printf("failed\n");

                return (1);
            }
        } else {
            memcpy(buf, aes_test_cfb128_pt, 64);
            aes_crypt_cfb128(&ctx, v, 64, &offset, iv, buf, buf);

            if (memcmp(buf, aes_test_cfb128_ct[u], 64) != 0) {
                if (verbose != 0)
                    printf("failed\n");

                return (1);
            }
        }

        if (verbose != 0)
            printf("passed\n");
    }

    if (verbose != 0)
        printf("\n");

    return (0);
}

#endif
