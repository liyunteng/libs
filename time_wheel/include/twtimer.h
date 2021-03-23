/*
 * twtimer.h - twtimer
 *
 * Date   : 2021/03/23
 */
#ifndef __TWTIMER_H__
#define __TWTIMER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct twtimer {
    uint64_t expire;  // expire clock time

    struct twtimer *next;
    struct twtimer **pprev; // pointer to prev node's next

    void (*ontimeout)(void *param);
    void *param;
} twtimer_t;

typedef struct time_wheel time_wheel_t;
time_wheel_t *time_wheel_create(uint64_t clock);
int time_wheel_destroy(time_wheel_t *tm);
void time_wheel_dump(time_wheel_t *tm);
/// @return sleep time(ms)
int twtimer_process(time_wheel_t *tm, uint64_t clock);

/// one-shoot timeout timer
/// @return 0-ok, other-error
int twtimer_start(time_wheel_t *tm, twtimer_t *timer);
/// @return  0-ok, other-timer can't be stop(timer have triggered or will be
/// triggered)
int twtimer_stop(time_wheel_t *tm, twtimer_t *timer);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
