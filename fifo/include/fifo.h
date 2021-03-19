/*
 * fifo.h - fifo
 *
 * Date   : 2021/03/18
 */
#ifndef __FIFO_H__
#define __FIFO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct fifo {
    uint32_t in;
    uint32_t out;
    uint32_t mask;
    uint32_t esize;
    void *data;
} fifo_t;

uint32_t rounddown_pow_of_two(uint32_t n);

///
/// nelem: n elemnt, nelem must be 2^order
/// esize: element size
/// return: 0-success other-failed
int fifo_init(fifo_t *fifo, void *buf, uint32_t nelem, uint32_t esize);
int fifo_alloc(fifo_t *fifo, uint32_t nelem, uint32_t esize);
void fifo_free(fifo_t *fifo);

uint32_t fifo_in(fifo_t *fifo, const void *buf, uint32_t nelem);
uint32_t fifo_peek(fifo_t *fifo, void *buf, uint32_t nelem);
uint32_t fifo_out(fifo_t *fifo, void *buf,uint32_t nelem);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
