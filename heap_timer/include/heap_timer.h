/*
 * heap_timer.h - heap_timer
 *
 * Date   : 2021/03/23
 */
#ifndef __HEAP_TIMER_H__
#define __HEAP_TIMER_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct heap_timer heap_timer_t;
typedef struct timer_event timer_event_t;
typedef void (*timer_cb_t)(void *data);

/* @max_evts  max timer_event_t num */
heap_timer_t *heap_timer_create(int max_evts);
void heap_timer_destroy(heap_timer_t *ctx);
void heap_timer_run(heap_timer_t *ctx);

/* @ctx         heap_timer_t ctx
 * @cb          timer_event callback
 * @data        callback param
 * @delay       delay clock
 * @interval    interval
 * @count       repeat times
 */
timer_event_t *timer_event_register(heap_timer_t *ctx, timer_cb_t cb,
                                    void *data, uint32_t delay,
                                    uint32_t interval, uint32_t count);
void timer_event_cancel(heap_timer_t *ctx, timer_event_t *ev);


#ifdef __cplusplus
}
#endif /* extern "C" */

#endif /* __HEAP_TIMER_H__ */
