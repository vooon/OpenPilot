#include "cloggeros.h"
#include "../op_errors.h"
#if defined(QT_PROJECT)
# include "cloggeros_qt.h"
#elif defined(IOS_PROJECT) || defined(OSX_PROJECT)
# include "cloggeros_ios.h"
#elif defined(__ANDROID__)
# include "cloggeros_a.h"
#endif

CLoggerOs::CLoggerOs()
{
    m_refs = 0;
}

CLoggerOs::~CLoggerOs()
{
}

//===================================================================
// static functions
//===================================================================

int CLoggerOs::createLoggerOs( ILoggerOs** ppLoggerOs )
{
    if( !ppLoggerOs )
        return OPERR_INVALIDPARAM;
    *ppLoggerOs = 0;

    CLoggerOs* logger = 0;
#if defined(QT_PROJECT)
    logger = new CLoggerOs_Qt();
#elif defined (IOS_PROJECT) || defined (OSX_PROJECT)
    logger = new CLoggerOs_iOs();
#elif defined(__ANDROID__)
    logger = new CLoggerOs_A();
#else
    logger = new CLoggerOs();
#endif
    if( !logger )
        return OPERR_OUTOFMEMORY;
    logger->addRef();
    *ppLoggerOs = logger;
    return OP_OK;
}

//===================================================================
//  m e m b e r   f u n c t i o n s
//===================================================================

opuint32 CLoggerOs::addRef()
{
    return ++m_refs;
}

// уменьшает время жизни
opuint32 CLoggerOs::release()
{
    if (!m_refs)
        return 0;
    if (!(--m_refs))
    {
        delete this;
        return 0;
    }
    return m_refs;
}

int CLoggerOs::openFile(const CString& path)
{
    if (m_file.isOpen())
        return OPERR_INVALIDCALL;
    m_fileName = path;
    m_file.setFileName( path );
    bool ret = m_file.open( CIODevice::WriteOnly | CIODevice::ReadOnly );
    if (!ret)
        return OPERR_FAIL;
    return OP_OK;
}

void CLoggerOs::closeFile()
{
    if (!m_file.isOpen())
        return;
    m_file.close();
}

CString CLoggerOs::fileName()
{
    return m_fileName;
}

void CLoggerOs::writeString(const CString& text)
{
    writeToFileImmediately(text);
}

void CLoggerOs::writeToFileImmediately(const CString& text)
{
    if (text.isEmpty())
        return;
    CString final = text;
//#if defined (__ANDROID__) || defined (IOS_PROJECT)
    final += "\r\n";
//#endif // __ANDROID__ || IOS_PROJECT
    m_file.write( final.data(), final.size() - 1 ); // without 0 terminating
    m_file.flush();
}
