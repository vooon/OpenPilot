/*
**  ClanLib SDK
**  Copyright (c) 1997-2011 The ClanLib Team
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
**  Note: Some of the libraries ClanLib may link to may have additional
**  requirements or restrictions.
**
**  File Author(s):
**
**    Magnus Norddahl
*/
/*
 * ( c ) 2010-2013  Original developer
 * ( c ) 2013 The OpenPilot
 */

#include "thread.h"
#include "runnable.h"
#include "../op_errors.h"
#ifdef WIN32
#include "Win32/thread_win32.h"
#else
#include "unix/thread_unix.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CL_Thread Construction:

CThread::CThread()
#ifdef WIN32
: impl(new CL_Thread_Win32)
#else
: impl(new CL_Thread_Unix)
#endif
{
}

CThread::~CThread()
{
}

/////////////////////////////////////////////////////////////////////////////
// CL_Thread Attributes:

/////////////////////////////////////////////////////////////////////////////
// CL_Thread Operations:

void CThread::start(CL_Runnable *runnable)
{
	if (runnable == 0)
        throw CException(OPERR_FAIL, "Invalid runnable pointer");

	impl->start(runnable);
}

void CThread::join()
{
	impl->join();
}

void CThread::kill()
{
	impl->kill();
}

#if defined(_MSC_VER)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;     // must be 0x1000
	LPCSTR szName;    // pointer to name (in user address space)
	DWORD dwThreadID; // thread ID (-1 = caller thread)
	DWORD dwFlags;    // reserved for future use, must be zero
} THREADNAME_INFO;
#endif

void CThread::set_thread_name(const char *name)
{
#if defined(_MSC_VER)
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = -1;
	info.dwFlags = 0;

	__try
	{
		RaiseException( 0x406D1388, 0, sizeof(info) / sizeof(DWORD), (const ULONG_PTR*) &info );
	}
	__except( EXCEPTION_CONTINUE_EXECUTION )
	{
	}
#else
    name = name;        // unused parameter
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CL_Thread Implementation:
