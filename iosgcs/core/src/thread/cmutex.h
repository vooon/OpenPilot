
#ifndef __OpenPilotGCS__cmutex__
#define __OpenPilotGCS__cmutex__

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#endif

class CMutex
{
public:
	CMutex();
	~CMutex();

	void lock();
	bool tryLock();
	void unlock();
    
private:
#ifdef _WIN32
	CRITICAL_SECTION m_handle;
#else
	pthread_mutex_t m_handle;
#endif
};

class CMutexSection
{
public:
	/// \brief Constructs a mutex section.
	CMutexSection( CMutex *mutex, bool lockMutex = true )
	{
		m_mutex     = mutex;
		m_lockCount = 0;
		if( lockMutex )
			lock();
	}
    
	~CMutexSection()
	{
		if( m_lockCount > 0 && m_mutex )
			m_mutex->unlock();
		m_lockCount = 0;
	}
    
	int lockCount() const
	{
		return m_lockCount;
	}
    
	void lock()
	{
		if( m_mutex )
			m_mutex->lock();
		m_lockCount++;
	}
	bool tryLock()
	{
		if( m_mutex == 0 || m_mutex->tryLock() )
		{
			m_lockCount++;
			return true;
		}
		return false;
	}
	void unlock()
	{
		if( m_lockCount <= 0 )
			return;
		if( m_mutex )
			m_mutex->unlock();
		m_lockCount--;
	}
    
private:
	CMutex * m_mutex;
	int      m_lockCount;
};

#endif /* defined(__OpenPilotGCS__cmutex__) */
