#ifndef CLOGGEROS_QT_H
#define CLOGGEROS_QT_H

#include "cloggeros.h"

class CLoggerOs_Qt : public CLoggerOs
{
public:
    CLoggerOs_Qt();
    virtual ~CLoggerOs_Qt();

    virtual void writeString(const CString& text);
};

#endif // CLOGGEROS_QT_H
