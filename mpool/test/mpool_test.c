/*
 * mpool_test.c -- mpool test
 *
 * Copyright (C) 2016 liyunteng
 * Auther: liyunteng <li_yunteng@163.com>
 * License: GPL
 * Update time:  2016/06/06 00:57:32
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpool.h"

int
main(int argc, char *argv[])
{
    mpool_ctx_t *ctx;
    unsigned char *p;
    unsigned char *queue[4096];
    int i;
    ctx = mpool_init(123, 1048576, NULL);
    printf("%d / %d head:%d tail:%d\n", mpool_count(ctx), mpool_size(ctx),
           ctx->head, ctx->tail);

    memset(queue, 0, sizeof(queue));
    int j;
    for (j = 0; j < 10; j++) {

        printf("====================GET====================\n");
        for (i = 0; i < ARRAY_SIZE(queue); i++) {
            p = (unsigned char *)mpool_get(ctx);
            if (!p) {
                printf("get failed %d / %d head:%d tail:%d\n", mpool_count(ctx),
                       mpool_size(ctx), ctx->head, ctx->tail);
                break;
            }
            queue[i] = p;
        }

        printf("====================PUT====================\n");
        for (i = 0; i < ARRAY_SIZE(queue); i++) {
            p = queue[i];
            if (!p) {
                printf("put failed %d / %d head:%d tail:%d\n", mpool_count(ctx),
                       mpool_size(ctx), ctx->head, ctx->tail);
                break;
            }
            mpool_put(p);
        }
    }

    mpool_cleanup(ctx);
    return 0;
}
