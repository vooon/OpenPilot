#ifndef LOGGER_H
#define LOGGER_H

#include "text/cstring.h"
#include "os/in_common.h"
#include <fstream>
#include <string>
#include <sstream>

class CStringList;
class CVariant;

enum manipulator {
    HEX,
    DEC
};

class Logger
{
public:
    Logger();
    ~Logger();

    static void init();
    static void setLoggerOs(ILoggerOs* loggerOs);
    static void deinit();

    static int  openFile(const CString& fileName);
    static void writeToLog(const CString& text);
    static void writeToLogImmediatly(const CString& text);

    Logger& operator<<(const char* str);
    Logger& operator<<(const CString& str);
    Logger& operator<<(const CByteArray& data);
    Logger& operator<<(const CStringList& list);
    Logger& operator<<(const CVariant& v);
    template<typename T>
    Logger& operator<<(const T& val) {
        if( HEX == m_man )
        {
            std::ostringstream stream;
            stream << std::hex << val;
            m_text << stream.str().c_str();
        }
        else
            m_text << " " << CString(val);
        return *this;
    }

private:
    static ILoggerOs * m_loggerOs;
    CString            m_text;
    manipulator        m_man;
};

#endif // LOGGER_H
