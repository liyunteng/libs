/*
 * test.c - test
 *
 * Date   : 2020/04/25
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "md5.h"

typedef struct md5_test_pair_ {
    char *input;
    char *output;
} md5_test_pair;

static md5_test_pair data[] = {
    {"", "d41d8cd98f00b204e9800998ecf8427e"},
    {"a", "0cc175b9c0f1b6a831c399e269772661"},
    {"abc", "900150983cd24fb0d6963f7d28e17f72"},
    {"message digest", "f96b697d7cb7938d525a2f31aaf161d0"},
    {"abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b"},
    {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "d174ab98d277d9f5a5611c2c9f419d9f"},
    {"12345678901234567890123456789012345678901234567890123456789012345678901234567890", "57edf4a22be3c955ac49da2e2107b67a"},
    {"/data/mod_list.jsp?title=10002&page=1", "4d101056476ecc313fe9375ac6683ba2"},
    {"/data/mod_list.jsp?title=10004&page=1", "f1624fad3f1e7bc6f0e594e2194c4f43"},
    {"this will be fail", "f1624fad3f1e7bc6f0e594e2194c4f43"},
    {NULL, NULL}
};

int
main (void)
{
    char buf[40], *bp;
    int i;
    MD5_CTX md5_ctx;
    md5_u   sid;
    md5_test_pair *pdata = &data[0];

    while (pdata->input) {
        MD5Init(&md5_ctx);
        MD5Update(&md5_ctx, (const unsigned char *)pdata->input, strlen(pdata->input));
        MD5Final(&md5_ctx, sid.c);

        bp = buf;
        for (i = 0; i < sizeof(sid); i++) {
            bp += sprintf(bp, "%02x", sid.c[i]);
        }
        *bp = 0;
        if (strcmp(buf, pdata->output)) {
            printf("md5(%s) error(%s != %s)\n", pdata->input, buf, pdata->output);
        }
        else {
            printf("md5(%s) [%s] successfully.\n", pdata->input, buf);
        }

        pdata++;
    }

    return(0);
}
