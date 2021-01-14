/*
 * test.c - test
 *
 * Date   : 2021/01/13
 */
#include "log.h"

void test1()
{
    char buf[1024*8] = {0};
    log_ctx_p ctx = log_init("abc", "./", 4, 4096);
    for (int i = 0; i < sizeof(buf)-1; i++) {
        buf[i] = 'a';
    }
    for (int i = 0; i < 4096; i++) {
        log_print(ctx, "this is a test\n");
        log_print(ctx, "%s\n", buf);
    }


    log_destroy(ctx);
    ctx = NULL;
}

int main(void)
{
    test1();
    return 0;
}
