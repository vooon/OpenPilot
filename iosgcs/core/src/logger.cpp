#include "logger.h"
#include "os/cloggeros.h"
#include "op_errors.h"
#include "core/cvariant.h"
#include "core/cbytearray.h"
#include "text/cstringlist.h"

ILoggerOs* Logger::m_loggerOs = 0;

Logger::Logger()
{
}

Logger::~Logger()
{
    writeToLog(m_text);
}

void Logger::init()
{
    ILoggerOs* os;
    int err = CLoggerOs::createLoggerOs( &os );
    if (err || !os)
        return;
    setLoggerOs(os);
    SAFE_RELEASE(os);
    return;
}

void Logger::setLoggerOs(ILoggerOs* loggerOs)
{
    SAFE_RELEASE( m_loggerOs );
    m_loggerOs = loggerOs;
    if( m_loggerOs )
        m_loggerOs->addRef();
}

void Logger::deinit()
{
    SAFE_RELEASE(m_loggerOs);
}

int Logger::openFile( const CString& fileName )
{
    if( m_loggerOs )
        return m_loggerOs->openFile( fileName );
    return OPERR_FAIL;
}

void Logger::writeToLog(const CString &text )
{
    if( m_loggerOs )
        m_loggerOs->writeString( text );
}

void Logger::writeToLogImmediatly( const CString& text )
{
    if( m_loggerOs )
        m_loggerOs->writeToFileImmediately( text );
}

Logger& Logger::operator<<(const char* str)
{
    m_text << " " << str;
    return *this;
}

Logger& Logger::operator<<(const CString& str)
{
    m_text << " " << str;
    return *this;
}

Logger& Logger::operator<<(const CByteArray& data)
{
    if (data.isEmpty())
        return *this;
    std::ostringstream stream;
    stream << std::hex;
    unsigned char* ptr = (unsigned char*)data.data();
    for( int i = 0;i<int(data.size());i++ )
    {
        if( *ptr == 0 )
            stream << "00 ";
        else if( *ptr <= 0x0f )
            stream << "0" << std::hex << (int)*ptr << " ";
        else
            stream << std::hex << (int)*ptr << " ";
        ptr++;
    }
    m_text << stream.str().c_str();
    return *this;
}

Logger& Logger::operator<<(const CStringList& list)
{
    CStringList::const_iterator it;
    int i = 0, size = int(list.size());
    for( it = list.begin();it!=list.end();it++ )
    {
        i++;
        m_text << (*it);
        if( i != size )
            m_text << "\r\n";
    }
    return *this;
}

Logger& Logger::operator<<(const CVariant& v)
{
    switch (v.type())
    {
    case CVariant::String:
        m_text << v.toString();
        break;
    case CVariant::Int:
        operator<<(opint32(v.toInt()));
        break;
    case CVariant::UInt:
        operator<<(opuint32(v.toUInt()));
        break;
    case CVariant::Double:
        operator<<(v.toDouble());
        break;
    case CVariant::Bool:
        m_text << (v.toBool() ? "TRUE" : "FALSE");
        break;
    default:
        break;
    }
    return *this;
}
