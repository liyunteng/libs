/*
 * test.c - test
 *
 * Author : liyunteng <liyunteng@streamocean.com>
 * Date   : 2019/08/06
 *
 * Copyright (C) 2019 StreamOcean, Inc.
 * All rights reserved.
 */
#include "log2.h"
#include <stdio.h>

int
test1()
{
    unsigned int u = 1024;
    printf("%d\n", ilog2(u));
    printf("%d\n", ilog2(1025));
    return 0;
}

int
main(void)
{
    test1();
    return 0;
}

/* Local Variables: */
/* compile-command: "clang -Wall -o test test.c -g -I../ -I../../include" */
/* End: */
