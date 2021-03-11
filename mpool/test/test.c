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

typedef struct {
    char str[120];
    int id;
} item_t;

int main(int argc, char* argv[])
{
    mpool_ctx_t *ctx;
    item_t *p;
    item_t* queue[32] = {0};
    int i;
    ctx = mpool_calloc(16, sizeof(item_t));

    printf("================================GET================================\n");
    for(i = 0; i < 16; i++) {
        p = (item_t *)mpool_get(ctx);
        if(!p) break;
        queue[i] = p;
        printf("%d / %d [%d]\n", mpool_count(ctx), mpool_size(ctx), mpool_get_idx(ctx, p));
    }

    printf("================================PUT================================\n");
    for(i = 0; i < 16; i++) {
        p = queue[i];
        if(!p) break;
        p->id = i;
        mpool_put(p);
        printf("%d / %d [%d] %d\n", mpool_count(ctx), mpool_size(ctx), mpool_get_idx(ctx, p), p->id);
    }

    mpool_put(p);
    for (i = 0; i < 16; i++) {
        p = mpool_get_by_idx(ctx, i);
        printf("%d\n", p->id);
    }


    mpool_cleanup(ctx);
    return 0;
}
