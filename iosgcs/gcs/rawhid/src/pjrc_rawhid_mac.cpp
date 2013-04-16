#include "pjrc_rawhid.h"
#include "usbmonitor.h"
#include "Logger"
#include "system/CSystem"
#include "system/CTime"
#include <assert.h>

bool pjrc_rawhid::open()
{
    if (d->m_bOpened)
        return true;
    assert(d->m_devHandle != 0);
    assert(d->m_monitor != 0);

    if (d->m_monitor == 0 || d->m_devHandle == 0)
        return false;
    if (IOHIDDeviceOpen(d->m_devHandle, kIOHIDOptionsTypeNone) != kIOReturnSuccess)
        return false;
    IOHIDDeviceScheduleWithRunLoop(d->m_devHandle, d->m_monitor->m_loop, CFSTR(OP_LOOPMODE_NAME_MAC));
    IOHIDDeviceRegisterInputReportCallback(d->m_devHandle, d->m_buffer, sizeof(d->m_buffer), &pjrc_rawhid::input_callback, this);
    d->m_bOpened = true;
    return true;
}

bool pjrc_rawhid::close()
{
    if (!d->m_bOpened)
        return false;
    IOHIDDeviceClose(d->m_devHandle, kIOHIDOptionsTypeNone);
    IOHIDDeviceRegisterInputReportCallback(d->m_devHandle, d->m_buffer, sizeof(d->m_buffer), NULL, NULL);
    IOHIDDeviceUnscheduleFromRunLoop(d->m_devHandle, d->m_monitor->m_loop, CFSTR(OP_LOOPMODE_NAME_MAC));
    d->m_bOpened = false;
    return true;
}

int pjrc_rawhid::receive(void* buf, int len, int timeout)
{
    int recv = 0;
    int size = 0;
    CTime startTime;
    while(len)
    {
        {
            CMutexSection locker(&d->m_rcvMutex);
            if (len > int (d->m_rcvData.size()))
                size = int (d->m_rcvData.size());
            else
                size = len;
            if (buf) {
                memcpy(buf, d->m_rcvData.data(), size);
            }
            len -= size;
            recv += size;
            d->m_rcvData.remove(0, size);
        }
        if (len) {
            // wait new data from device
            CSystem::sleep(10);
            if (timeout != -1 && CTime()-startTime >= timeout)
                return recv;
        }
    }
    return recv;
}

#define MAX_PACKET_SIZE 64

int pjrc_rawhid::send(const void* buf, int len, int /*timeout*/)
{
    if (d->m_bTerminate)
        return -1;
    d->m_bSending = true;
    if (!buf || !len)
        return 0;
    int start = 0;
    int sended = 0;
    while (len)
    {
        char buffer[MAX_PACKET_SIZE] = {0};

        int size = std::min(MAX_PACKET_SIZE-2, len);
        buffer[0] = 2;
        buffer[1] = (char)size;
        memcpy(buffer+2, (char*)buf+start, size);
        IOReturn ret = IOHIDDeviceSetReport(d->m_devHandle, kIOHIDReportTypeOutput, 2, (const uint8_t*)buffer, MAX_PACKET_SIZE);
        if (ret != kIOReturnSuccess)
            return 0;           // error sending data to device
        if (d->m_bTerminate) {
            d->m_bSending = false;
            return -1;
        }
        sended += size;
        start += size;
        len -= size;
    }
    d->m_bSending = false;
    return sended;
}

// amount of data in the receiver buffer
int pjrc_rawhid::bytesAvailable() const
{
    CMutexSection locker(&d->m_rcvMutex);
    return int (d->m_rcvData.size());
}

//===================================================================
//  p r i v a t e   f u n c t i o n s
//===================================================================

//! Callback for the HID driver on an input report
void pjrc_rawhid::input_callback(void *c, IOReturn ret, void* /*sender*/, IOHIDReportType /*type*/, uint32_t /*id*/, uint8_t *data, CFIndex len)
{
    if (ret != kIOReturnSuccess || len < 1)
        return;

    pjrc_rawhid *context = (pjrc_rawhid *) c;
    context->input(data, len);
}

/**
 * @brief input Called to add input data to the buffer
 * @param[in] data The data buffer
 * @param[in] len The report length
 */
void pjrc_rawhid::input(uint8_t *data, CFIndex len)
{
    assert(d->m_bOpened);
    if (!d->m_bOpened)
        return;

    assert(len <= BUFFER_LENGTH);

    CMutexSection locker(&d->m_rcvMutex);
    d->m_rcvData.append((const char*)data, len);

//    d->m_signalReceivedData.invoke(this);
    d->addNotify(d->m_signalReceivedData.invokeQueue(this));
}

void pjrc_rawhid::deviceDetached()
{
    d->m_bOpened = false;
//    close();              // device will close automatically from usb monitor
    d->m_signalDeviceDetached.invoke(this);
}
