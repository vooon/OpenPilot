#include "rawhid.h"
#include "pjrc_rawhid.h"

RawHID::RawHID(pjrc_rawhid* rawHid)
{
    m_rawHid = rawHid;

    if (m_rawHid)
    {
        m_slotReceivedData = m_rawHid->signal_receivedData().connect(this, &RawHID::receivedData);
        m_slotDeviceClosed = m_rawHid->signal_deviceDetached().connect(this, &RawHID::deviceDetached);
    }
}

RawHID::~RawHID()
{
}

/////////////////////////////////////////////////////////////////////
//                            open/close                           //
/////////////////////////////////////////////////////////////////////

bool RawHID::open( opuint32 /*mode*/ )
{
    CMutexSection locker(&m_mutex);
    if (!m_rawHid)
        return false;
    return m_rawHid->open();
}

void RawHID::close()
{
    CMutexSection locker(&m_mutex);
    if (!m_rawHid)
        return;
    m_rawHid->close();
}

bool RawHID::isOpen() const
{
    CMutexSection locker(&m_mutex);
    if (!m_rawHid)
        return false;
    return m_rawHid->opened();
}

bool RawHID::isSequential() const
{
    return true;
}

/////////////////////////////////////////////////////////////////////
//                               io                                //
/////////////////////////////////////////////////////////////////////

int RawHID::write(const void* data, int len)
{
    if (m_rawHid)
        return m_rawHid->send(data, len);
    return -1;
}

int RawHID::read(void* data, int len) const
{
    if (m_rawHid)
        return m_rawHid->receive(data, len, 5000);
    return -1;
}

opint64 RawHID::bytesAvailable() const
{
    CMutexSection locker(&m_mutex);
    if (m_rawHid)
        return m_rawHid->bytesAvailable();
    return 0;
}

//===================================================================
//  p r i v a t e   f u n c t i o n s
//===================================================================

void RawHID::receivedData(pjrc_rawhid* rawHid)
{
    CMutexSection locker(&m_mutex);
    if (rawHid != m_rawHid)
        return;
    m_signalReadyRead.invoke(this);
}

void RawHID::deviceDetached(pjrc_rawhid* rawHid)
{
    CMutexSection locker(&m_mutex);
    if (rawHid != m_rawHid)
        return;
    close();
    m_rawHid = 0;
    m_signalDeviceClosed.invoke(this);
}
