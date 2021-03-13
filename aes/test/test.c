/*
 * test.c - test
 *
 * Author : liyunteng <liyunteng@streamocean.com>
 * Date   : 2019/08/25
 *
 * Copyright (C) 2019 StreamOcean, Inc.
 * All rights reserved.
 */
#include "aes.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

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


int aes_test(const char *name,
             const uint8_t *input, const size_t input_len,
             uint8_t *key, size_t key_len,
             uint8_t iv[16])
{
    aes_context ctx;
    int i = 0;
    unsigned char block[16] = {0};
    int iv_offset = 0;
    unsigned char encrypted[64] = {0};
    unsigned char decrypted[64] = {0};
    unsigned char iv_bak[16] = {0};
    int encryptedlen = 0;
    int count = 0, left = 0;

    printf("AES-%3ld-%s (enc):\n", 128+key_len*64, name);
    aes_setkey_enc(&ctx, key, 128+key_len*64);

    if (strcmp(name, "ECB") == 0) {
        count = input_len / 16;
        left = input_len % 16;
        encryptedlen = (left == 0) ? (count*16) : (count+1)*16;
        for (i = 0; i < count; i++) {
            memcpy(block, input+i*16, 16);
            aes_crypt_ecb(&ctx, AES_ENCRYPT, block, encrypted+i*16);
        }

        // pading with 0x00
        if (left > 0) {
            memset(block, 0x00, 16);
            memcpy(block, input+i*16, left);
            aes_crypt_ecb(&ctx, AES_ENCRYPT, block, encrypted+i*16);
        }

    } else if (strcmp(name, "CBC") == 0) {
        count = input_len / 16;
        left = input_len % 16;
        encryptedlen = (left == 0) ? (count*16) : (count+1)*16;

        memcpy(iv_bak, iv, 16);
        aes_crypt_cbc(&ctx, AES_ENCRYPT, count*16, iv_bak, input, encrypted);

        // pading with 0x00
        if (left > 0) {
            memset(block, 0x00, 16);
            memcpy(block, input+count*16, left);
            aes_crypt_cbc(&ctx, AES_ENCRYPT, 16, iv_bak, block, encrypted+count*16);
        }

    } else {
        encryptedlen = input_len;

        iv_offset = 0;
        memcpy(iv_bak, iv, 16);
        aes_crypt_cfb128(&ctx, AES_ENCRYPT, input_len, &iv_offset, iv_bak, input, encrypted);
    }

    for (i = 0; i < encryptedlen; i++) {
        printf("%02x", encrypted[i]);
    }
    printf("\n");

    printf("AES-%3ld-%s (dec):\n", 128+key_len*64, name);
    if (strcmp(name, "ECB") == 0 || strcmp(name, "CBC") == 0) {
        aes_setkey_dec(&ctx, key, 128+key_len*64);
    }

    if (strcmp(name, "ECB") == 0) {
        for (i = 0; i < encryptedlen/16; i++) {
            aes_crypt_ecb(&ctx, AES_DECRYPT, encrypted+i*16, decrypted+i*16);
        }

    } else if (strcmp(name, "CBC") == 0) {
        memcpy(iv_bak, iv, 16);
        aes_crypt_cbc(&ctx, AES_DECRYPT, encryptedlen, iv_bak, encrypted, decrypted);
    } else {
        iv_offset = 0;
        memcpy(iv_bak, iv, 16);
        aes_crypt_cfb128(&ctx, AES_DECRYPT, input_len, &iv_offset, iv_bak, encrypted, decrypted);
    }



    printf("%s\n", decrypted);



    return 0;
}

int aes_test_compat_with_openssl(const char *name,
                                 const uint8_t *input, const size_t input_len,
                                 uint8_t *key, size_t key_len,
                                 uint8_t iv[16])
{
    aes_context ctx;
    int i = 0, j = 0;
    unsigned char block[16] = {0};
    int iv_offset = 0;
    unsigned char encrypted[64] = {0};
    unsigned char decrypted[64] = {0};
    unsigned char iv_bak[16] = {0};
    int left = 0, count = 0;
    int encryptedlen = 0;

    printf("AES-%3ld-%s (enc):\n", 128+key_len*64, name);
    aes_setkey_enc(&ctx, key, 128+key_len*64);

    if (strcmp(name, "ECB") == 0) {
        count = input_len / 16;
        left = input_len % 16;
        encryptedlen = (count + 1) * 16;
        for (i = 0; i < count; i++) {
            memcpy(block, input+i*16, 16);
            aes_crypt_ecb(&ctx, AES_ENCRYPT, block, encrypted+i*16);
        }

        // pading with (0x10 - left), if left==0, pad 16Byte
        memset(block, (0x10-left), 16);
        memcpy(block, input+i*16, left);
        aes_crypt_ecb(&ctx, AES_ENCRYPT, block, encrypted+i*16);

    } else if (strcmp(name, "CBC") == 0) {
        count = input_len / 16;
        left = input_len % 16;
        encryptedlen = (count + 1) * 16;
        memcpy(iv_bak, iv, 16);
        aes_crypt_cbc(&ctx, AES_ENCRYPT, count*16, iv_bak, input, encrypted);

        // pading with (0x10 - left), if left==0, pad 16Byte
        memset(block, (0x10-left), 16);
        memcpy(block, input+count*16, left);
        aes_crypt_cbc(&ctx, AES_ENCRYPT, 16, iv_bak, block, encrypted+count*16);
    } else {
        encryptedlen = input_len;
        iv_offset = 0;
        memcpy(iv_bak, iv, 16);
        aes_crypt_cfb128(&ctx, AES_ENCRYPT, input_len, &iv_offset, iv_bak, input, encrypted);
    }

    for (i = 0; i < encryptedlen; i++) {
        printf("%02x", encrypted[i]);
    }
    printf("\n");

    printf("AES-%3ld-%s (dec):\n", 128+key_len*64, name);
    if (strcmp(name, "ECB") == 0 || strcmp(name, "CBC") == 0) {
        aes_setkey_dec(&ctx, key, 128+key_len*64);
    }

    if (strcmp(name, "ECB") == 0) {
        for (i = 0; i < encryptedlen / 16; i++) {
            memcpy(block, encrypted+i*16, 16);
            aes_crypt_ecb(&ctx, AES_DECRYPT, block, decrypted+i*16);
        }
    } else if (strcmp(name, "CBC") == 0) {
        memcpy(iv_bak, iv, 16);
        aes_crypt_cbc(&ctx, AES_DECRYPT, encryptedlen, iv_bak, encrypted, decrypted);
    } else {
        iv_offset = 0;
        memcpy(iv_bak, iv, 16);
        aes_crypt_cfb128(&ctx, AES_DECRYPT, input_len, &iv_offset, iv_bak, encrypted, decrypted);
    }



    printf("%s\n", decrypted);

    return 0;
}

int
main(void)
{
    aes_self_test(1);
    int i;
    const unsigned char *input[] = {
        "abc",
        "0123456789",
        "0123456789abcdef",
        "abcdefghijklmnopqrstuvwxyz"
    };

    unsigned char key[32] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    unsigned char iv[16] = {0x00};

    // for test
    // echo -n 0123456789abcdef | openssl enc -aec-128-ecb -K
    // 000102030405060708090A0B0C0D0E0F -nosalt -iv 00 | xxd
    for (i = 0; i< sizeof(input)/sizeof(input[0]); i++) {
        aes_test_compat_with_openssl("ECB", input[i], strlen(input[i]), key, 0, iv);
        aes_test_compat_with_openssl("CBC", input[i], strlen(input[i]), key, 0, iv);
        aes_test_compat_with_openssl("CFB", input[i], strlen(input[i]), key, 0, iv);

        /* aes_test_compat_with_openssl("ECB", input[i], strlen(input[i]), key, 1, iv); */
        /* aes_test_compat_with_openssl("CBC", input[i], strlen(input[i]), key, 1, iv); */
        /* aes_test_compat_with_openssl("CFB", input[i], strlen(input[i]), key, 1, iv); */

        /* aes_test("ECB", input[i], strlen(input[i]), key, 0, iv); */
        /* aes_test("CBC", input[i], strlen(input[i]), key, 0, iv); */
        /* aes_test("CFB", input[i], strlen(input[i]), key, 0, iv); */
        printf("\n");
    }

    return 0;
}
