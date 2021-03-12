/*
 * test.c - test
 *
 * Date   : 2021/03/12
 */
#include "base64.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void
base64_test(void)
{
    char source[512] = {0};
    char target[512] = {0};
    int tl, sl;

    struct {
        const char *input;
        const char *output;
    } inputs[] = {
        {"", ""},
        {"a", "YQ=="},
        {"0123456789", "MDEyMzQ1Njc4OQ=="},
        {"abcdefghijklmnopqrstuvwxyz", "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXo="},
        {"ABCDEFGHIJKLMNOPQRSTUVWXYZ", "QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVo="},
        {"http://www.baidu.com/search?keyword=aaa&aa=bbb",
         "aHR0cDovL3d3dy5iYWlkdS5jb20vc2VhcmNoP2tleXdvcmQ9YWFhJmFhPWJiYg=="},
        {"Aladdin:open sesame", "QWxhZGRpbjpvcGVuIHNlc2FtZQ=="},
    };

    for (int i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        memset(source, 0, 512);
        memset(target, 0, 512);
        tl = base64_encode(target, inputs[i].input, strlen(inputs[i].input));
        if (memcmp(target, inputs[i].output, tl) == 0) {
            printf("encode %s [pass]\n", inputs[i].input);
        } else {
            printf("encode %s [failed]    ==> %s != %s\n", inputs[i].input,
                   target, inputs[i].output);
        }

        sl = base64_decode(source, target, tl);
        if (memcmp(source, inputs[i].input, sl) == 0) {
            printf("decode %s [pass]\n", source);
        } else {
            printf("decode %s [failed]    ==> %s != %s\n", source, source,
                   inputs[i].input);
        }
    }
}

int
main(void)
{
    base64_test();
    return 0;
}
