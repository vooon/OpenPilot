#include <QtDebug>
#include "cloggeros_qt.h"

CLoggerOs_Qt::CLoggerOs_Qt()
{
}

CLoggerOs_Qt::~CLoggerOs_Qt()
{
}

void CLoggerOs_Qt::writeString( const CString& text )
{
    qDebug() << TO_QSTRING( text );
    CLoggerOs::writeString(text);
}
