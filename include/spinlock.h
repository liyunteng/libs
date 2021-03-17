/*
 * spinlock.h - spinlock
 *
 * Date   : 2021/03/16
 */
#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#if defined(_WIN32)
#include <Windows.h>
typedef CRITICAL_SECTION spinlock_t;
#elif defined(__APPLE__)
#include <assert.h>

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12 || __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_10_0
#include <os/lock.h>
typedef struct os_unfair_lock_s spinlock_t;
#else
// https://blog.postmates.com/why-spinlocks-are-bad-on-ios-b69fc5221058
// https://blog.ibireme.com/2016/01/16/spinlock_is_unsafe_in_ios/
// https://mjtsai.com/blog/2015/12/16/osspinlock-is-unsafe/
// https://github.com/protocolbuffers/protobuf/pull/1060/commits/d6590d653415c0bfacf97e7f768dd3c994cb8d26
#include <libkern/OSAtomic.h>
typedef OSSpinLock spinlock_t;
#endif

#elif defined(__KERNEL__)
#include <linux/spinlock.h>
#else
#include <pthread.h>
typedef pthread_spinlock_t spinlock_t;
#endif

//----------------------------------------------
// int spinlock_create(spinlock_t *locker)
// int spinlock_destroy(spinlock_t *locker)
// void spinlock_lock(spinlock_t *locker)
// void spinlock_unlock(spinlock_t *locker)
// int spinlock_trylock(spinlock_t *locker)
//----------------------------------------------

static inline int spinlock_create(spinlock_t *locker)
{
#if defined(_WIN32)
	// Minimum support OS: WinXP
	return InitializeCriticalSectionAndSpinCount(locker, 0x00000400) ? 0: -1;
#elif defined(__APPLE__)
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12 || __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_10_0
    locker->_os_unfair_lock_opaque = 0;
#else
    // see more Apple Developer spinlock
    // OSSpinLock is an integer type.  The convention is that unlocked is zero, and locked is nonzero.
    // Locks must be naturally aligned and cannot be in cache-inhibited memory.
#if TARGET_CPU_X86_64 || TARGET_CPU_PPC64
    assert((intptr_t)locker % 8 == 0);
#else
    assert((intptr_t)locker % 4 == 0);
#endif
    *locker = 0;
#endif
	return 0;
#elif defined(__KERNEL__)
	spin_lock_init(locker);
	return 0;
#else
	return pthread_spin_init(locker, PTHREAD_PROCESS_PRIVATE);
#endif
}

static inline int spinlock_destroy(spinlock_t *locker)
{
#if defined(_WIN32)
	DeleteCriticalSection(locker); return 0;
#elif defined(__APPLE__)
    (void)locker;
	return 0; // do nothing
#elif defined(__KERNEL__)
	return 0; // do nothing
#else
	return pthread_spin_destroy(locker);
#endif
}

static inline void spinlock_lock(spinlock_t *locker)
{
#if defined(_WIN32)
	EnterCriticalSection(locker);
#elif defined(__APPLE__)
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12 || __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_10_0
    os_unfair_lock_lock(locker);
#else
	OSSpinLockLock(locker);
#endif
#elif defined(__KERNEL__)
	spin_lock(locker);
#else
	pthread_spin_lock(locker);
#endif
}

static inline void spinlock_unlock(spinlock_t *locker)
{
#if defined(_WIN32)
	LeaveCriticalSection(locker);
#elif defined(__APPLE__)
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12 || __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_10_0
    os_unfair_lock_unlock(locker);
#else
	OSSpinLockUnlock(locker);
#endif
#elif defined(__KERNEL__)
	spin_unlock(locker);
#else
	pthread_spin_unlock(locker);
#endif
}

static inline int spinlock_trylock(spinlock_t *locker)
{
#if defined(_WIN32)
	return TryEnterCriticalSection(locker) ? 1 : 0;
#elif defined(__APPLE__)
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12 || __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_10_0
    return os_unfair_lock_trylock(locker) ? 1 : 0;
#else
	return OSSpinLockTry(locker) ? 1 : 0;
#endif
#elif defined(__KERNEL__)
	return spin_trylock(locker);
#else
	return pthread_spin_trylock(locker);
#endif
}


#endif
