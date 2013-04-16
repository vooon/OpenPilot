#include "pjrc_rawhid.h"
#include "system/CSystem"

pjrc_rawhid::pjrc_rawhid() :
    d(new pjrc_rawhidPrivate())
{
}

pjrc_rawhid::pjrc_rawhid(const pjrc_rawhid& other) :
    d(other.d)
{

}

pjrc_rawhid::~pjrc_rawhid()
{
}

pjrc_rawhid& pjrc_rawhid::operator=(const pjrc_rawhid& other)
{
    d = other.d;
    return *this;
}

bool pjrc_rawhid::operator==(const pjrc_rawhid& other) const
{
    if (d == other.d)
        return true;
#if defined(__APPLE__) && !defined(IOS_PROJECT)
    if (d->m_devHandle != other.d->m_devHandle)
        return false;
#endif
    return true;
}

bool pjrc_rawhid::opened() const
{
    return d->m_bOpened;
}

/////////////////////////////////////////////////////////////////////
//                           device params                         //
/////////////////////////////////////////////////////////////////////

CString pjrc_rawhid::product() const
{
    return d->m_product;
}

CString pjrc_rawhid::serial() const
{
    return d->m_serialNumber;
}

void pjrc_rawhid::onUpdate()
{
    d->runAllNotifies();
}

/////////////////////////////////////////////////////////////////////
//                              signals                            //
/////////////////////////////////////////////////////////////////////

CL_Signal_v1<pjrc_rawhid*>& pjrc_rawhid::signal_receivedData()
{
    return d->m_signalReceivedData;
}

CL_Signal_v1<pjrc_rawhid*>& pjrc_rawhid::signal_deviceDetached()
{
    return d->m_signalDeviceDetached;
}
