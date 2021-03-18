/*
 * fifo.c - fifo
 *
 * Date   : 2021/03/18
 */
#include "fifo.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define is_power_of_2(x) ((x) != 0 && (((x) & ((x)-1)) == 0))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

static uint32_t CLZ_32(uint32_t n)
{
    int ret = 0;
    uint32_t tmp = ~n;
    while(tmp & 0x80000000) {
        tmp <<= 1;
        ret ++;
    }

    return ret;
}

uint32_t rounddown_pow_of_two(uint32_t n)
{
    uint32_t ret;
    if (n == 0) {
        return 0;
    }

    if ((n & (n - 1)) == 0) {
        return n;
    }

    ret = CLZ_32(n);
    return 1 << (32 - ret);
}

static inline uint32_t
fifo_unused(struct fifo *fifo)
{
    return (fifo->mask + 1) - (fifo->in - fifo->out);
}


int fifo_alloc(struct fifo *fifo, uint32_t nelem, uint32_t esize)
{
    // round down to the next power of 2, since our 'let the indices
    // wrap' technique works only in this case

    if (!is_power_of_2(nelem)) {
        // size = rounddown_power_of_two(size);
        fifo->data = NULL;
        fifo->mask = 0;
        return -EINVAL;
    }

    fifo->in = 0;
    fifo->out = 0;
    fifo->esize = esize;

    if (nelem < 2) {
        fifo->data = NULL;
        fifo->mask = 0;
        return -EINVAL;
    }

    fifo->data = malloc(nelem * esize);
    if (!fifo->data) {
        fifo->mask = 0;
        return -ENOMEM;
    }

    fifo->mask = nelem - 1;

    return 0;
}

void fifo_free(struct fifo*fifo)
{
    free(fifo->data);
    fifo->in = 0;
    fifo->out = 0;
    fifo->esize = 0;
    fifo->data = NULL;
    fifo->mask = 0;
}

int fifo_init(struct fifo*fifo, void *buf, uint32_t nelem, uint32_t esize)
{

    if (!is_power_of_2(nelem)) {
        fifo->mask = 0;
        // size = rounddown_pow_of_two(total_size);
        return -EINVAL;
    }

    fifo->in = 0;
    fifo->out = 0;
    fifo->esize = esize;
    fifo->data = buf;

    if (nelem < 2) {
        fifo->mask = 0;
        return -EINVAL;
    }
    fifo->mask = nelem - 1;

    return 0;
}


static void fifo_copy_in(struct fifo *fifo, const void *src, uint32_t nelem, uint32_t off)
{
    uint32_t size = fifo->mask + 1;
    uint32_t esize = fifo->esize;
    uint32_t l;

    off &= fifo->mask;
    if (esize != 1) {
        off *= esize;
        size *= esize;
        nelem *= esize; //
    }

    l = MIN(nelem, size - off);

    memcpy(fifo->data + off, src, l);
    memcpy(fifo->data, src+l, nelem - l);

    // smp_wmb(;
}

uint32_t fifo_in(struct fifo* fifo, const void *buf, uint32_t nelem)
{
    uint32_t l;

    l = fifo_unused(fifo);
    if (nelem > l)
        nelem = l;

    fifo_copy_in(fifo, buf, nelem, fifo->in);
    fifo->in += nelem;
    return nelem;
}


static void fifo_copy_out(struct fifo *fifo, void *dst, uint32_t nelem, uint32_t off)
{
    uint32_t size = fifo->mask + 1;
    uint32_t esize = fifo->esize;
    uint32_t l;

    off &= fifo->mask;
    if (esize != 1) {
        off *= esize;
        size *= esize;
        nelem *= esize; //
    }

    l = MIN(nelem, size - off);
    memcpy(dst, fifo->data + off, l);
    memcpy(dst + l, fifo->data, nelem - l);

    // smp_wmb();
}


uint32_t fifo_peek(struct fifo*fifo, void *buf, uint32_t nelem)
{
    uint32_t l;

    l = fifo->in - fifo->out;
    if (nelem > l)
        nelem = l;

    fifo_copy_out(fifo, buf, nelem, fifo->out);
    return nelem;
}

uint32_t fifo_out(struct fifo*fifo, void *buf, uint32_t nelem)
{
    nelem = fifo_peek(fifo, buf, nelem);
    fifo->out += nelem;

    return nelem;
}
