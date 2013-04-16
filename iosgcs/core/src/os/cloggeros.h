#ifndef CLOGGEROS_H
#define CLOGGEROS_H

#include "in_common.h"
#include "../io/cfile.h"

class CLoggerOs : public ILoggerOs
{
public:
    CLoggerOs();
    virtual ~CLoggerOs();

// static functions
    static int createLoggerOs(ILoggerOs** ppLoggerOs);

    virtual opuint32 addRef();
    virtual opuint32 release();

    virtual int openFile(const CString& path);
    virtual void closeFile();
    virtual CString fileName();

    virtual void writeString(const CString& text);
    void writeToFileImmediately(const CString& text);

protected:
    opuint32 m_refs;
    CString  m_fileName;
    CFile    m_file;
};

#endif // CLOGGEROS_H
