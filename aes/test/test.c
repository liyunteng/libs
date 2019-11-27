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

#if 0
int test0()
{
    int rc = aes_self_test(1);
    if (rc) {
        printf("failed");
    }
    return rc;
}
#endif

int
test1()
{
    printf("ecb:\n");
    size_t i;
    aes_context ctx, dctx;
    unsigned char key[256] = {0};
    for (i = 0; i < sizeof(key); i++) {
        key[i] = i;
    }
    aes_setkey_enc(&ctx, key, sizeof(key));

    unsigned char input[16]  = "0123456789abcdef";
    unsigned char output[16] = {0};
    aes_crypt_ecb(&ctx, AES_ENCRYPT, input, output);
    for (i = 0; i < sizeof(output); i++) {
        printf("%02x", output[i]);
    }
    printf("\n");

    memset(input, 0, sizeof(input));
    aes_setkey_dec(&dctx, key, sizeof(key));
    aes_crypt_ecb(&dctx, AES_DECRYPT, output, input);
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
    for (i = 0; i < sizeof(key); i++) {
        key[i] = i;
    }
    aes_setkey_enc(&ctx, key, sizeof(key));
    unsigned char input[1024] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned char output[1024] = {0};
    unsigned char iv[16]       = "0123456789abcdef";
    aes_crypt_cbc(&ctx, AES_ENCRYPT, sizeof(input), iv, input, output);
    for (i = 0; i < sizeof(output); i++) {
        printf("%02x", output[i]);
    }
    printf("\n");

    memset(input, 0, sizeof(input));
    memcpy(iv, "0123456789abcdef", sizeof(iv));
    aes_setkey_dec(&dctx, key, sizeof(key));
    aes_crypt_cbc(&dctx, AES_DECRYPT, sizeof(output), iv, output, input);
    for (i = 0; i < sizeof(input); i++) {
        printf("%c", input[i]);
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
    unsigned char key[256] = {0};
    for (i = 0; i < sizeof(key); i++) {
        key[i] = i;
    }
    aes_setkey_enc(&ctx, key, sizeof(key));
    unsigned char input[1024] =
        "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned char output[1024] = {0};
    unsigned char iv[16]       = "0123456789abcdef";
    int iv_off                 = 0;
    aes_crypt_cfb128(&ctx, AES_ENCRYPT, sizeof(input), &iv_off, iv, input,
                     output);
    for (i = 0; i < sizeof(output); i++) {
        printf("%02x", output[i]);
    }
    printf("\n");

    iv_off = 0;
    memset(input, 0, sizeof(input));
    memcpy(iv, "0123456789abcdef", sizeof(iv));
    aes_setkey_enc(&dctx, key,
                   sizeof(key)); /* internal only used AES_ENCRYPT */
    aes_crypt_cfb128(&dctx, AES_DECRYPT, sizeof(output), &iv_off, iv, output,
                     input);
    for (i = 0; i < sizeof(input); i++) {
        printf("%c", input[i]);
    }
    printf("\n");

    printf("\n\n");
    return 0;
}

int
main(void)
{
    /* test0(); */
    test1();
    test2();
    test3();
    return 0;
}
