/*
 * mpool.h - mpool
 *
 * Date   : 2021/03/17
 */
#ifndef __MPOOL_H__
#define __MPOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define MPOOL_DBG

#define MPOOL_FLAG_ALLOC 0x01U
#define MPOOL_FLAG_READY 0x02U

#define ALIGN_SIZE 8
typedef volatile uint32_t vuint32_t;
typedef volatile int32_t vint32_t;
typedef vint32_t atomic_t;

typedef struct {
    void *ctx;
    atomic_t ref;

    //For debug
#ifdef MPOOL_DBG
    void *get;
    void *put;
#endif
    uint8_t data[0];
} mpool_item_t;


typedef struct {
    size_t size;   //size of a mpool_item_t, align to 8 bytes
    size_t msize;  //size of a member, sizeof(*mpool_item_t->data)
    uint32_t max;  // capacity
    uint32_t flag;
    atomic_t ref;
    vuint32_t head;
    vuint32_t tail;
    mpool_item_t **queue; // array of mpool_item_t pointer
    uint8_t *pool; // data
} mpool_ctx_t;

/*
 *  Init a cache in system memory
 *  Parameter: ctx         context of memory cache to init.
 *             msize       size of member.
 *             total_size  the size of total memory
 *             pool        a piece of memory to use. If NULL, mpool will alloc memory by itself.
 * Return: 0 on success, -1 on error.
 */
mpool_ctx_t *mpool_init(size_t msize, size_t total_size, void *pool);

/*
 * Initialize a memory by the number of member.
 * Argument
 *   @nmem:  how many memebers do the pool store.
 *   @msize: the size of a member
 * Return
 *   A pointer to mpool_ctx_t. NULL if init failed.
 */
mpool_ctx_t *mpool_calloc(size_t nmem, size_t msize);

/*
 *  Cleanup
 */
void mpool_cleanup(mpool_ctx_t *ctx);

void *__mpool_get(mpool_ctx_t *ctx, const char *, int);
void __mpool_put(void *ptr, const char *, int);

#define mpool_get(ctx) __mpool_get(ctx, __FILE__, __LINE__)
#define mpool_put(ptr) __mpool_put(ptr, __FILE__, __LINE__)

/*
 *  Increace / decrease ref count.
 */
void __mpool_ref_inc(void *ptr, const char *, int);
void __mpool_ref_dec(void *ptr, const char *, int);
#define mpool_ref_inc(ptr) __mpool_ref_inc(ptr, __FILE__, __LINE__)
#define mpool_ref_dec(ptr) __mpool_ref_dec(ptr, __FILE__, __LINE__)

/*
 *  Get index for a piece of memory
 */
int mpool_get_idx(mpool_ctx_t *ctx, void *ptr);
void *mpool_get_by_idx(mpool_ctx_t *ctx, int idx);

/*
 * Test if mpool is empty
 */
static inline int
mpool_empty(mpool_ctx_t *ctx)
{
    return ctx->tail - ctx->head >= ctx->max;
}

static inline int
mpool_full(mpool_ctx_t *ctx)
{
    //mpool is full means queue is empty
    return ctx->tail == ctx->head;
}

static inline int
mpool_size(mpool_ctx_t *ctx)
{
    return ctx->max;
}

static inline int
mpool_count(mpool_ctx_t *ctx)
{
    return ctx->max + ctx->head - ctx->tail;
}

#ifdef __cplusplus
}
#endif
#endif
