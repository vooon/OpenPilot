
#include "cmutex.h"

#ifndef WIN32
// We need to do this because the posix threads library under linux obviously
// suck:
extern "C"
{
#if defined(__APPLE__) || defined (__FreeBS3D__) || defined(__OpenBSD__) || defined(__ANDROID__)
	int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind);
#else
	int pthread_mutexattr_setkind_np(pthread_mutexattr_t *attr, int kind);
#endif
}
#endif

CMutex::CMutex()
{
#ifdef WIN32
	InitializeCriticalSection( &m_handle );
#else
	pthread_mutexattr_t attr;
	pthread_mutexattr_init( &attr );
# if defined(__FreeBSD__) || defined(__APPLE__) || defined(__ANDROID__)
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
# else
#  if PTHREAD_MUTEX_RECURSIVE_NP
	pthread_mutexattr_setkind_np( &attr, PTHREAD_MUTEX_RECURSIVE );
#  else
	pthread_mutexattr_setkind_np( &attr, PTHREAD_MUTEX_RECURSIVE_NP );
#  endif
# endif
	pthread_mutex_init( &m_handle, &attr );
	pthread_mutexattr_destroy( &attr );
#endif
}

CMutex::~CMutex()
{
#ifdef WIN32
	DeleteCriticalSection( &m_handle );
#else
	pthread_mutex_destroy( &m_handle );
#endif
}

void CMutex::lock()
{
#ifdef WIN32
	EnterCriticalSection( &m_handle );
#else
	pthread_mutex_lock( &m_handle );
#endif
}

bool CMutex::tryLock()
{
#ifdef WIN32
	BOOL result = TryEnterCriticalSection( &m_handle );
	return (result != FALSE);
#else
	int result = pthread_mutex_trylock( &m_handle );
	return (result == 0);
#endif
}

void CMutex::unlock()
{
#ifdef WIN32
	LeaveCriticalSection( &m_handle );
#else
	pthread_mutex_unlock( &m_handle );
#endif
}
