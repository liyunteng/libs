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

int test0()
{
    aes_self_test(1);
    return 0;
}

int
test1()
{
    printf("ecb:\n");
    size_t i;
    aes_context ctx, dctx;
    unsigned char key[256] = {0};
    printf("key: ");
    for (i = 0; i < sizeof(key); i++) {
        key[i] = i;
        printf("%02x", key[i]);
    }
    printf("\n");
    aes_setkey_enc(&ctx, key, sizeof(key));

    unsigned char input[16]  = "0123456789abcdef";
    unsigned char output[16] = {0};
    aes_crypt_ecb(&ctx, AES_ENCRYPT, input, output);
    printf("output: ");
    for (i = 0; i < sizeof(output); i++) {
        printf("%02x", output[i]);
    }
    printf("\n");

    memset(input, 0, sizeof(input));
    aes_setkey_dec(&dctx, key, sizeof(key));
    aes_crypt_ecb(&dctx, AES_DECRYPT, output, input);
    printf("input: ");
    for (i = 0; i < sizeof(input); i++) {
        printf("%c", input[i]);
    }
    printf("\n");

    printf("\n\n");
    return 0;
}

int
test2()
{
    printf("cbc:\n");
    size_t i;
    aes_context ctx, dctx;
    unsigned char key[256] = {0};
    printf("key: ");
    for (i = 0; i < sizeof(key); i++) {
        key[i] = i;
        printf("%02x", key[i]);
    }
    printf("\n");
    aes_setkey_enc(&ctx, key, sizeof(key));

    unsigned char input[64] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned char output[64] = {0};
    unsigned char iv[16]       = "0123456789abcdef";
    aes_crypt_cbc(&ctx, AES_ENCRYPT, sizeof(input), iv, input, output);
    printf("output: ");
    for (i = 0; i < sizeof(output); i++) {
        printf("%02x", output[i]);
    }
    printf("\n");

    memset(input, 0, sizeof(input));
    memcpy(iv, "0123456789abcdef", sizeof(iv));
    aes_setkey_dec(&dctx, key, sizeof(key));
    aes_crypt_cbc(&dctx, AES_DECRYPT, sizeof(output), iv, output, input);
    printf("input: ");
    for (i = 0; i < sizeof(input); i++) {
        printf("%c", input[i]);
    }
    printf("\n");

    printf("iv: ");
    for (i = 0; i < sizeof(iv); i++) {
        printf("%02x", iv[i]);
    }
    printf("\n");

    printf("\n\n");
    return 0;
}

int
test3()
{
    printf("cfb128:\n");
    size_t i;
    aes_context ctx, dctx;
    unsigned char key[128] = {0};
    printf("key: ");
    for (i = 0; i < sizeof(key); i++) {
        key[i] = i;
        printf("%02x", key[i]);
    }
    printf("\n");
    aes_setkey_enc(&ctx, key, sizeof(key));

    unsigned char input[256] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned char output[256] = {0};
    unsigned char iv[16]       = "0123456789abcdef";
    int iv_off                 = 0;
    aes_crypt_cfb128(&ctx, AES_ENCRYPT, sizeof(input), &iv_off, iv, input,
                     output);
    printf("output: ");
    for (i = 0; i < sizeof(output); i++) {
        printf("%02x", output[i]);
    }
    printf("\n");
    printf("iv: ");
    for (i = 0; i < sizeof(iv); i++) {
        printf("%02x", iv[i]);
    }
    printf("\n");

    printf("ivoff: %d\n", iv_off);

    iv_off = 0;
    memset(input, 0, sizeof(input));
    memcpy(iv, "0123456789abcdef", sizeof(iv));
    aes_setkey_enc(&dctx, key,
                   sizeof(key)); /* internal only used AES_ENCRYPT */
    aes_crypt_cfb128(&dctx, AES_DECRYPT, sizeof(output), &iv_off, iv, output,
                     input);
    printf("input: ");
    for (i = 0; i < sizeof(input); i++) {
        printf("%c", input[i]);
    }
    printf("\n");

    printf("iv: ");
    for (i = 0; i < sizeof(iv); i++) {
        printf("%02x", iv[i]);
    }
    printf("\n");

    printf("ivoff: %d\n", iv_off);

    printf("\n\n");
    return 0;
}

int
main(void)
{
    test0();
    test1();
    test2();
    test3();
    return 0;
}
