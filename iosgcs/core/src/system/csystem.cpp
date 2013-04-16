#include "csystem.h"

#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(HAVE_SYS_SYSCTL_H) && \
    !defined(_SC_NPROCESSORS_ONLN) && !defined(_SC_NPROC_ONLN)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if !defined(__APPLE__) && !defined(__ANDROID__)
#include <execinfo.h>
#endif
#include <cxxabi.h>
#include <cstring>
#include <cstdlib>
#endif

CSystem::CSystem()
{
}

void CSystem::sleep( int ms )
{
#ifdef _WIN32
    Sleep( ms );
#else
    usleep( ms * 1000 );
#endif
}
