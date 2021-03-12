/*
 * rwlocker.h - rwlocker
 *
 * Date   : 2021/03/12
 */

#ifndef _platform_rwlocker_h_
#define _platform_rwlocker_h_

#include <errno.h>

#if defined(_WIN32)
#include <Windows.h>
typedef SRWLOCK	rwlocker_t;
#else
#include <pthread.h>
typedef pthread_rwlock_t rwlocker_t;
#endif

//-------------------------------------------------------------------------------------
// int rwlocker_create(rwlocker_t* locker);
// int rwlocker_destroy(rwlocker_t* locker);
// int rwlocker_rdlock(rwlocker_t* locker); // Shared mode
// int rwlocker_wrlock(rwlocker_t* locker); // Exclusive mode
// int rwlocker_rdunlock(rwlocker_t* locker);
// int rwlocker_wrunlock(rwlocker_t* locker);
// int rwlocker_tryrdlock(rwlocker_t* locker);
// int rwlocker_trywrlock(rwlocker_t* locker);
//-------------------------------------------------------------------------------------

static inline int rwlocker_create(rwlocker_t* locker)
{
#if defined(_WIN32)
	InitializeSRWLock(locker); return 0;
#else
	//pthread_rwlockattr_t attr;
	//pthread_rwlockattr_init(&attr);
	//pthread_rwlockattr_destroy(&attr);
	return pthread_rwlock_init(locker, NULL);
#endif
}

static inline int rwlocker_destroy(rwlocker_t* locker)
{
#if defined(_WIN32)
	//MSDN: SRW locks do not need to be explicitly destroyed.
	return 0;
#else
	return pthread_rwlock_destroy(locker);
#endif
}

static inline int rwlocker_rdlock(rwlocker_t* locker)
{
#if defined(_WIN32)
	AcquireSRWLockShared(locker); return 0;
#else
	return pthread_rwlock_rdlock(locker);
#endif
}

static inline int rwlocker_wrlock(rwlocker_t* locker)
{
#if defined(_WIN32)
	AcquireSRWLockExclusive(locker); return 0;
#else
	return pthread_rwlock_wrlock(locker);
#endif
}

static inline int rwlocker_rdunlock(rwlocker_t* locker)
{
#if defined(_WIN32)
	ReleaseSRWLockShared(locker); return 0;
#else
	return pthread_rwlock_unlock(locker);
#endif
}

static inline int rwlocker_wrunlock(rwlocker_t* locker)
{
#if defined(_WIN32)
	ReleaseSRWLockExclusive(locker); return 0;
#else
	return pthread_rwlock_unlock(locker);
#endif
}

// @return 0-ok, other-error
static inline int rwlocker_tryrdlock(rwlocker_t* locker)
{
#if defined(_WIN32)
	return 0 == TryAcquireSRWLockShared(locker) ? -1 : 0;
#else
	return pthread_rwlock_tryrdlock(locker);
#endif
}

// @return 0-ok, other-error
static inline int rwlocker_trywrlock(rwlocker_t* locker)
{
#if defined(_WIN32)
	return 0 == TryAcquireSRWLockExclusive(locker) ? -1 : 0;
#else
	return pthread_rwlock_trywrlock(locker);
#endif
}

#endif /* !_platform_rwlocker_h_ */
