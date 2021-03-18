/*
 * timer.h - timer
 *
 * Date   : 2020/04/30
 */
#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>
#include <pthread.h>

#include "mpool.h"

#define HZ 500
#define MAX_EVENT_COUNT (16 * 1024)

typedef void(*timer_cb_t)(void *data);

typedef struct {
    uint32_t id;
    uint64_t time;              /* next trigger time */
    timer_cb_t cb;
    void *data;
    uint32_t delay;
    uint32_t interval;
    uint32_t count;
    uint8_t enable;
} timer_event_t;

typedef struct {
    volatile uint64_t timer_jiffies;
} timer_tick_t;

typedef struct {
    uint32_t nevts;
    timer_event_t *heap[MAX_EVENT_COUNT];
    mpool_ctx_t *mpool;
} timer_ctx_t;

typedef struct {
    pthread_mutex_t lock;       /* only used when init and cleanup */
    pthread_t timer_thread;
    uint8_t ready;
    timer_tick_t clock;
} timer_thread_ctx_t;

/*
 *  Init timer system. MUST be called before any other functions.
 */
extern timer_ctx_t* timer_init(timer_ctx_t* ctx);

/*
 *  Clean up all alloced resource.
 *  This will block until timer thread exit.
 */
extern void timer_cleanup(timer_ctx_t *ctx);

/*
 *  Register a event to timer.
 *
 *  Parameter:   ctx     : timer context
 *               cb      : callback function pointer
 *               data    : parameter of callback function
 *               dalay   : Milliseconds between register and the first time cb is called. 0 for calling cb immediately.
 *               interval: Milliseconds of interval for calling cb cyclically. 0 for just call once.
 *               count   : How many times cb will be called. 0 for infinity.
 *
 *  Return: event id on success. -1 on error.
 */
extern int timer_register(timer_ctx_t *ctx, timer_cb_t cb, void* data, uint32_t delay, uint32_t interval, uint32_t count);

/*
 *  Cancel a event which is not excuting. If event is excuting, will cancel it after excution finished.
 *
 */
extern void timer_cancel(timer_ctx_t *ctx, int event_id);


extern void timer_run(timer_ctx_t *ctx);

extern uint64_t get_current_clock(void);


#endif
