/*
 * atomic_x86_64.h - atomic_x86_64
 *
 * Date   : 2021/03/16
 */
#ifndef __ARCH_X86_64_ATOMIC__
#define __ARCH_X86_64_ATOMIC__

#include "types.h"

#define CONFIG_SMP
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#ifdef CONFIG_SMP
#    define LOCK_PREFIX                                                        \
        ".section .smp_locks,\"a\"\n"                                          \
        "  .align 8\n"                                                         \
        "  .quad 661f\n" /* address */                                         \
        ".previous\n"                                                          \
        "661:\n\tlock; "
#else
#    define LOCK_PREFIX
#endif  // CONFIG_SMP

#ifdef CONFIG_SMP
#    define LOCK "lock ; "
#else
#    define LOCK ""
#endif

typedef struct {
    volatile int counter;
} atomic_t;
typedef struct {
    atomic_t ref;
} spin_lock_t;

#define ATOMIC_INIT(i)                                                         \
    {                                                                          \
        (i)                                                                    \
    }
/**
 * atiomic_read - read atomic variable
 * @v:     pointer of type atomic_t
 *
 * Description:
 *  Atomically reads the value of @v
 */
#define atomic_read(v) ((v)->counter)

/**
 * atomic_set - set atomic variable
 * @v:     pointer of type atomic_t
 * @i:     required value
 *
 * Description:
 * Atomically sets the value of @v to @i.
 */
#define atomic_set(v, i) (((v)->counter) = (i))

/**
 * atomic_add - add integer to atomic variable
 * @i:     integer value to add
 * @v:     pointer of type atomic_t
 *
 * Description:
 * Atomically add @i to @v.
 */
static __inline__ void
atomic_add(int i, atomic_t *v)
{
    __asm__ __volatile__(LOCK_PREFIX "addl %1,%0"
                         : "=m"(v->counter)
                         : "ir"(i), "m"(v->counter));
}

/**
 * atomic_sub - subtract the atomic variable
 * @i:     integer value to subtract
 * @v:     pointer of type atomic_t
 *
 * Description:
 * Atomically subtracts @i from @v.
 */
static __inline__ void
atomic_sub(int i, atomic_t *v)
{
    __asm__ __volatile__(LOCK_PREFIX "subl %1,%0"
                         : "=m"(v->counter)
                         : "ir"(i), "m"(v->counter));
}

/**
 * atomic_sub_and_test - subtract value from variable and test result
 * @i:     integer value to suntract
 * @v:     pointer of type atomic_t
 *
 * Description:
 * Atomically subtracts @i from @v and returns true if the result is
 * zero, or false for all other cases.
 */
static __inline__ int
atomic_sub_and_test(int i, atomic_t *v)
{
    unsigned char c;
    __asm__ __volatile(LOCK_PREFIX "subl %2,%0; sete %1"
                       : "=m"(v->counter), "=qm"(c)
                       : "ir"(i), "m"(v->counter)
                       : "memory");

    return c;
}

/**
 * atomic_inc - increment atomic variable
 * @v:     pointer to type atomic_t
 *
 * Description:
 *  Atomically increments @v by 1.
 */
static __inline__ void
atomic_inc(atomic_t *v)
{
    __asm__ __volatile__(LOCK_PREFIX "incl %0"
                         : "=m"(v->counter)
                         : "m"(v->counter));
}

/**
 * atomic_dec - decrement atomic variable
 * @v:     pointer of type atomic_t
 *
 * Description:
 *  Atomically decrements @v by 1.
 */
static __inline__ void
atomic_dec(atomic_t *v)
{
    __asm__ __volatile__(LOCK_PREFIX "decl %0"
                         : "=m"(v->counter)
                         : "m"(v->counter));
}

/**
 * atomic_dec_and_test - decrement and test
 * @v:     pointer of type atomic_t
 *
 * Description:
 * Atomically decrements @v by 1 and returns true if the result is 0,
 * or false for all other cases.
 */
static __inline__ int
atomic_dec_and_test(atomic_t *v)
{
    unsigned char c;

    __asm__ __volatile__(LOCK_PREFIX "decl %0; sete %1"
                         : "=m"(v->counter), "=qm"(c)
                         : "m"(v->counter)
                         : "memory");

    return c != 0;
}

/**
 * atomic_inc_and_test - increment and test
 * @v:     pointer of type atomic_t
 * @param:     description
 *
 * Description:
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all other cases.
 */
static __inline__ int
atomic_inc_and_test(atomic_t *v)
{
    unsigned char c;

    __asm__ __volatile__(LOCK_PREFIX "incl %0; sete %1"
                         : "=m"(v->counter), "=qm"(c)
                         : "m"(v->counter)
                         : "memory");

    return c != 0;
}

/**
 * atomic_add_negative - add and test if negative
 * @i:     integer value to add
 * @v:     pointer of type atomic_t
 *
 * Description:
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when result
 * is greater than or equal to zero.
 */
static __inline__ int
atomic_add_negative(int i, atomic_t *v)
{
    unsigned char c;

    __asm__ __volatile__(LOCK_PREFIX "addl %2,%0; sets %1"
                         : "=m"(v->counter), "=qm"(c)
                         : "ir"(i), "m"(v->counter)
                         : "memory");

    return c;
}

/**
 * atomic_add_return - add and return
 * @i: integer value to add
 * @v: pointer of type atomic_t
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static __inline__ int
atomic_add_return(int i, atomic_t *v)
{
    int __i = i;
    __asm__ __volatile__(LOCK_PREFIX "xaddl %0, %1;"
                         : "=r"(i)
                         : "m"(v->counter), "0"(i));
    return i + __i;
}

static __inline__ int
atomic_sub_return(int i, atomic_t *v)
{
    return atomic_add_return(-i, v);
}

#define atomic_inc_return(v) (atomic_add_return(1, v))
#define atomic_dec_return(v) (atomic_sub_return(1, v))

typedef struct {
    volatile long counter;
} atomic64_t;

#define ATOMIC64_INIT(i)                                                       \
    {                                                                          \
        (i)                                                                    \
    }

/**
 * atomic64_read - read atomic64 variable
 * @v: pointer of type atomic64_t
 *
 * Atomically reads the value of @v.
 * Doesn't imply a read memory barrier.
 */
#define atomic64_read(v) ((v)->counter)

/**
 * atomic64_set - set atomic64 variable
 * @v: pointer to type atomic64_t
 * @i: required value
 *
 * Atomically sets the value of @v to @i.
 */
#define atomic64_set(v, i) (((v)->counter) = (i))

/**
 * atomic64_add - add integer to atomic64 variable
 * @i: integer value to add
 * @v: pointer to type atomic64_t
 *
 * Atomically adds @i to @v.
 */
static __inline__ void
atomic64_add(long i, atomic64_t *v)
{
    __asm__ __volatile__(LOCK_PREFIX "addq %1,%0"
                         : "=m"(v->counter)
                         : "ir"(i), "m"(v->counter));
}

/**
 * atomic64_sub - subtract the atomic64 variable
 * @i: integer value to subtract
 * @v: pointer to type atomic64_t
 *
 * Atomically subtracts @i from @v.
 */
static __inline__ void
atomic64_sub(long i, atomic64_t *v)
{
    __asm__ __volatile__(LOCK_PREFIX "subq %1,%0"
                         : "=m"(v->counter)
                         : "ir"(i), "m"(v->counter));
}

/**
 * atomic64_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer to type atomic64_t
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static __inline__ int
atomic64_sub_and_test(long i, atomic64_t *v)
{
    unsigned char c;

    __asm__ __volatile__(LOCK_PREFIX "subq %2,%0; sete %1"
                         : "=m"(v->counter), "=qm"(c)
                         : "ir"(i), "m"(v->counter)
                         : "memory");
    return c;
}

/**
 * atomic64_inc - increment atomic64 variable
 * @v: pointer to type atomic64_t
 *
 * Atomically increments @v by 1.
 */
static __inline__ void
atomic64_inc(atomic64_t *v)
{
    __asm__ __volatile__(LOCK_PREFIX "incq %0"
                         : "=m"(v->counter)
                         : "m"(v->counter));
}

/**
 * atomic64_dec - decrement atomic64 variable
 * @v: pointer to type atomic64_t
 *
 * Atomically decrements @v by 1.
 */
static __inline__ void
atomic64_dec(atomic64_t *v)
{
    __asm__ __volatile__(LOCK_PREFIX "decq %0"
                         : "=m"(v->counter)
                         : "m"(v->counter));
}

/**
 * atomic64_dec_and_test - decrement and test
 * @v: pointer to type atomic64_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static __inline__ int
atomic64_dec_and_test(atomic64_t *v)
{
    unsigned char c;

    __asm__ __volatile__(LOCK_PREFIX "decq %0; sete %1"
                         : "=m"(v->counter), "=qm"(c)
                         : "m"(v->counter)
                         : "memory");
    return c != 0;
}

/**
 * atomic64_inc_and_test - increment and test
 * @v: pointer to type atomic64_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static __inline__ int
atomic64_inc_and_test(atomic64_t *v)
{
    unsigned char c;

    __asm__ __volatile__(LOCK_PREFIX "incq %0; sete %1"
                         : "=m"(v->counter), "=qm"(c)
                         : "m"(v->counter)
                         : "memory");
    return c != 0;
}

/**
 * atomic64_add_negative - add and test if negative
 * @i: integer value to add
 * @v: pointer to type atomic64_t
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static __inline__ int
atomic64_add_negative(long i, atomic64_t *v)
{
    unsigned char c;

    __asm__ __volatile__(LOCK_PREFIX "addq %2,%0; sets %1"
                         : "=m"(v->counter), "=qm"(c)
                         : "ir"(i), "m"(v->counter)
                         : "memory");
    return c;
}

/**
 * atomic64_add_return - add and return
 * @i: integer value to add
 * @v: pointer to type atomic64_t
 *
 * Atomically adds @i to @v and returns @i + @v
 */
static __inline__ long
atomic64_add_return(long i, atomic64_t *v)
{
    long __i = i;
    __asm__ __volatile__(LOCK_PREFIX "xaddq %0, %1;"
                         : "=r"(i)
                         : "m"(v->counter), "0"(i));
    return i + __i;
}

static __inline__ long
atomic64_sub_return(long i, atomic64_t *v)
{
    return atomic64_add_return(-i, v);
}

#define atomic64_inc_return(v) (atomic64_add_return(1, v))
#define atomic64_dec_return(v) (atomic64_sub_return(1, v))

#define __xg(x) ((volatile long *)(x))

/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */

#define __HAVE_ARCH_CMPXCHG 1

static inline unsigned long
__cmpxchg(volatile void *ptr, unsigned long old, unsigned long new, int size)
{
    unsigned long prev;
    switch (size) {
    case 1:
        __asm__ __volatile__(LOCK_PREFIX "cmpxchgb %b1,%2"
                             : "=a"(prev)
                             : "q"(new), "m"(*__xg(ptr)), "0"(old)
                             : "memory");
        return prev;
    case 2:
        __asm__ __volatile__(LOCK_PREFIX "cmpxchgw %w1,%2"
                             : "=a"(prev)
                             : "r"(new), "m"(*__xg(ptr)), "0"(old)
                             : "memory");
        return prev;
    case 4:
        __asm__ __volatile__(LOCK_PREFIX "cmpxchgl %k1,%2"
                             : "=a"(prev)
                             : "r"(new), "m"(*__xg(ptr)), "0"(old)
                             : "memory");
        return prev;
    case 8:
        __asm__ __volatile__(LOCK_PREFIX "cmpxchgq %1,%2"
                             : "=a"(prev)
                             : "r"(new), "m"(*__xg(ptr)), "0"(old)
                             : "memory");
        return prev;
    }
    return old;
}

#define cmpxchg(ptr, o, n)                                                     \
    ((__typeof__(*(ptr)))__cmpxchg((ptr), (unsigned long)(o),                  \
                                   (unsigned long)(n), sizeof(*(ptr))))

#define atomic_cmpxchg(v, old, new) ((int)cmpxchg(&((v)->counter), old, new))
#define atomic_xchg(v, new) (xchg(&((v)->counter), new))

/**
 * atomic_add_unless - add unless the number is a given value
 * @v: pointer of type atomic_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so long as it was not @u.
 * Returns non-zero if @v was not @u, and zero otherwise.
 */
#define atomic_add_unless(v, a, u)                                             \
    ({                                                                         \
        int c, old;                                                            \
        c = atomic_read(v);                                                    \
        for (;;) {                                                             \
            if (unlikely(c == (u)))                                            \
                break;                                                         \
            old = atomic_cmpxchg((v), c, c + (a));                             \
            if (likely(old == c))                                              \
                break;                                                         \
            c = old;                                                           \
        }                                                                      \
        c != (u);                                                              \
    })
#define atomic_inc_not_zero(v) atomic_add_unless((v), 1, 0)

/* These are x86-specific, used by some header files */
#define atomic_clear_mask(mask, addr)                                          \
    __asm__ __volatile__(LOCK_PREFIX "andl %0,%1"                              \
                         :                                                     \
                         : "r"(~(mask)), "m"(*addr)                            \
                         : "memory")

#define atomic_set_mask(mask, addr)                                            \
    __asm__ __volatile__(LOCK_PREFIX "orl %0,%1"                               \
                         :                                                     \
                         : "r"((unsigned)mask), "m"(*(addr))                   \
                         : "memory")

/* Atomic operations are already serializing on x86 */
#define smp_mb__before_atomic_dec() barrier()
#define smp_mb__after_atomic_dec() barrier()
#define smp_mb__before_atomic_inc() barrier()
#define smp_mb__after_atomic_inc() barrier()

static __inline__ bool
spin_test(spin_lock_t *lock)
{
    if (atomic_dec_and_test(&lock->ref)) {
        return TRUE;
    }
    atomic_inc(&lock->ref);
    return FALSE;
}

static __inline__ bool
spin_lock(spin_lock_t *lock)
{
    int64_t retries = 1000000L;
    int64_t i       = 0;

    while (spin_test(lock) == FALSE) {
        __asm__ __volatile__("nop;\n\t"
                             "nop;\n\t"
                             "nop;\n\t"
                             "nop;\n\t"
                             "nop;\n\t"
                             "nop;\n\t"
                             "nop;\n\t"
                             "nop;\n\t"
                             "nop;\n\t");
        i++;
        if (i >= retries) {
            usleep(1);
            i = 0;
            continue;
            // return(FALSE);
        }
    }

    return (TRUE);
}

static __inline__ void
spin_unlock(spin_lock_t *lock)
{
    atomic_inc(&lock->ref);
}

static __inline__ void
spin_lock_init(spin_lock_t *lock)
{
    atomic_set(&lock->ref, 1);
}

#endif
