/*
 * channel.h - channel
 *
 * Date   : 2021/03/12
 */

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct channel channel_t;

channel_t* channel_create(int capacity, int elementsize);
void channel_destroy(channel_t** pc);

//void channel_clear(struct channel_t* c);
int channel_count(channel_t* c);

// block push/pop
int channel_push(channel_t* c, const void* e);
int channel_pop(channel_t* c, void* e);

/// @param[in] timeout MS
/// @return 0-success, WAIT_TIMEOUT-timeout, other-error
int channel_push_timeout(channel_t* c, const void* e, int timeout);
int channel_pop_timeout(channel_t* c, void* e, int timeout);

#if defined(__cplusplus)
}
#endif
#endif /* !_channel_h_ */
