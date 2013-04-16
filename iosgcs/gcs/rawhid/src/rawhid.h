#ifndef RAWHID_H
#define RAWHID_H

#include "io/CIODevice"
#include "signals/Signals"
#include "thread/CMutex"

class pjrc_rawhid;

class RawHID : public CIODevice
{
public:
    RawHID(pjrc_rawhid* rawHid);
    virtual ~RawHID();

// open/close
    virtual bool open(opuint32 mode);
    virtual void close();
    virtual bool isOpen() const;
    virtual bool isSequential() const;

// io
    virtual int write( const void* data, int len );
    virtual int read( void* data, int len ) const;
    virtual opint64 bytesAvailable() const;

private:
    pjrc_rawhid    * m_rawHid;
    mutable CMutex   m_mutex;

    CL_Slot m_slotReceivedData;
    CL_Slot m_slotDeviceClosed;

    void receivedData(pjrc_rawhid* rawHid);
    void deviceDetached(pjrc_rawhid* rawHid);
};

#endif // RAWHID_H
