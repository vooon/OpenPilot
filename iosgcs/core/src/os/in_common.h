#ifndef IN_COMMON_H
#define IN_COMMON_H

#include "../text/cstring.h"

class ILoggerOs
{
public:
    virtual opuint32 addRef() = 0;
    virtual opuint32 release() = 0;

    virtual int openFile(const CString& path) = 0;
    virtual void closeFile() = 0;
    virtual CString fileName() = 0;

    virtual void writeString(const CString& text) = 0;
    virtual void writeToFileImmediately(const CString& text) = 0;
};

#endif // IN_COMMON_H
