/*
 * tls.h - tls
 *
 * Date   : 2021/03/16
 */
#ifndef __TLS_H__
#define __TLS_H__


#if defined(_WIN32)
#include <windows.h>
typedef DWORD tlskey_t;
#else
#include <pthread.h>
typedef pthread_key_t tlskey_t;
#endif

// Thread Local Storage
//-------------------------------------------------------------------------------------
// int tls_create(tlskey_t* key);
// int tls_destroy(tlskey_t key);
// int tls_setvalue(tlskey_t key, void* value);
// void* tls_getvalue(tlskey_t key);
//-------------------------------------------------------------------------------------

///@return 0-ok, other-error
static inline int tls_create(tlskey_t* key)
{
#if defined(_WIN32)
	*key = TlsAlloc();
	return TLS_OUT_OF_INDEXES == *key ? GetLastError() : 0;
#else
	return pthread_key_create(key, NULL);
#endif
}

///@return 0-ok, other-error
static inline int tls_destroy(tlskey_t key)
{
#if defined(_WIN32)
	return TlsFree(key) ? 0 : GetLastError();
#else
	return pthread_key_delete(key);
#endif
}

///@return 0-ok, other-error
static inline int tls_setvalue(tlskey_t key, void* value)
{
#if defined(_WIN32)
	return TlsSetValue(key, value) ? 0 : GetLastError();
#else
	return pthread_setspecific(key, value);
#endif
}

static inline void* tls_getvalue(tlskey_t key)
{
#if defined(_WIN32)
	return TlsGetValue(key);
#else
	return pthread_getspecific(key);
#endif
}


#endif
