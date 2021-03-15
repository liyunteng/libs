/*
 * test.c - test
 *
 * Date   : 2021/03/14
 */
#include "urlcodec.h"
#include <stdio.h>
#include <string.h>

int urlcodec_test(void)
{
    int i, ret;
    char encoded[256];
    char decoded[256];
    const char *urls[] = {
        "http://lyt:abc@www.baidu.com/search?keyword=abc#x",
        "http://adjfkasdj:asdf,asd!!!/ad**@",
    };

    for (i = 0; i < sizeof(urls)/sizeof(urls[0]); i++) {
        ret = url_encode(urls[i], strlen(urls[i]), encoded, 256);
        if (ret < 0) {
            printf("url_encode failed\n");
        } else {
            printf("encoded: %s\n", encoded);
        }

        ret = url_decode(encoded, strlen(encoded), decoded, 256);
        if (ret < 0) {
            printf("url_decode failed\n");
        } else {
            printf("decoded: %s\n", decoded);
            if (strcmp(urls[i], decoded) != 0) {
                printf("%s != %s\n", urls[i], decoded);
            }
        }
    }
    return 0;
}

int main(void)
{
    urlcodec_test();
    return 0;
}
