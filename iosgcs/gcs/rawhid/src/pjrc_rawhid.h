#ifndef PJRC_RAWHID_H
#define PJRC_RAWHID_H

#include "text/CString"
#include "core/CByteArray"
#include "system/SharedPtr"
#include "thread/CMutex"
#include "signals/Signals"
#include "system/CSystem"
#include "OPVector"
#include <assert.h>

// Depending on the OS, we'll need different things:
#if defined(__APPLE__) //&& !defined(IOS_PROJECT)
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
#elif defined(Q_OS_UNIX)

#include <libudev.h>
#include <QSocketNotifier>

#elif defined (Q_OS_WIN32)
#ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0500
#endif
#ifndef _WIN32_WINDOWS
    #define _WIN32_WINDOWS 0x0500
#endif
#ifndef WINVER
    #define WINVER 0x0500
#endif
#include <windows.h>
#include <dbt.h>
#include <setupapi.h>
#include <ddk/hidsdi.h>
#include <ddk/hidclass.h>
#endif

class UsbMonitor;

#define BUFFER_LENGTH 128

class pjrc_rawhid;

class pjrc_rawhidPrivate
{
friend class pjrc_rawhid;
friend class UsbMonitor;
public:
    pjrc_rawhidPrivate() {
        m_monitor = 0;
        m_bOpened = false;

#if defined(__APPLE__) //&& !defined(IOS_PROJECT)
        m_devHandle = 0;
#endif
        m_usagePage = 0;
        m_usage     = 0;
        m_vendorID  = 0;
        m_productID = 0;
        m_bcdDevice = 0;

        m_bTerminate = false;
        m_bSending   = false;
    }
    ~pjrc_rawhidPrivate() {
        assert(m_bOpened == false);
        m_bTerminate = true;
        while (m_bSending) {
            CSystem::sleep(10);
        }
    }
    
	void addNotify(const CL_SignalQueue& queue) {
        CMutexSection locker(&m_mutexNotify);
        m_notifies.push_back(queue);
    }
	void runAllNotifies() {
        CMutexSection locker( &m_mutexNotify );
        for (auto i = 0; i < m_notifies.size(); i++)
        {
            CL_SignalQueue queue = m_notifies[i];
            locker.unlock();
            queue.invoke();
            locker.lock();
        }
        m_notifies.clear();
    }

private:
    UsbMonitor   * m_monitor;

    bool           m_bOpened;

    CString        m_serialNumber; // As a string as it can be anything, really...
    CString        m_manufacturer;
    CString        m_product;
#if defined(_WIN32)
    CString        m_devicePath; //only has meaning on windows
#elif  defined(__APPLE__) //&& !defined(IOS_PROJECT)
    IOHIDDeviceRef m_devHandle;
#endif
    int            m_usagePage;
    int            m_usage;
    int            m_vendorID;       ///< Vendor ID.
    int            m_productID;      ///< Product ID
    int            m_bcdDevice;

    uint8_t        m_buffer[BUFFER_LENGTH];
    CByteArray     m_rcvData;                   // recieved data from usb
    CMutex         m_rcvMutex;

    bool m_bTerminate;
    bool m_bSending;

    CL_Signal_v1<pjrc_rawhid*> m_signalReceivedData;
    CL_Signal_v1<pjrc_rawhid*> m_signalDeviceDetached;
    
	opvector<CL_SignalQueue> m_notifies;
	CMutex                   m_mutexNotify;
};

class pjrc_rawhid
{
friend class UsbMonitor;
public:
    pjrc_rawhid();
    pjrc_rawhid(const pjrc_rawhid& other);
    ~pjrc_rawhid();

    pjrc_rawhid& operator=(const pjrc_rawhid& other);

    bool operator==(const pjrc_rawhid& other) const;

    bool open();
    bool close();
    bool opened() const;

// device params
    CString product() const;
    CString serial() const;

    int receive(void* buf, int len, int timeout = -1);
    int send(const void* buf, int len, int timeout = -1);
    int bytesAvailable() const;           // amount of data in the receiver buffer

    void onUpdate();

// signals
    CL_Signal_v1<pjrc_rawhid*>& signal_receivedData();
    CL_Signal_v1<pjrc_rawhid*>& signal_deviceDetached();

private:
    CL_SharedPtr<pjrc_rawhidPrivate> d;

#if defined(__APPLE__) //&& !defined(IOS_PROJECT)
    //! Callback for the HID driver on an input report
    static void input_callback(void *c, IOReturn ret, void *sender, IOHIDReportType type, uint32_t id, uint8_t *data, CFIndex len);
    void input(uint8_t *data, CFIndex len);
#endif

    void deviceDetached();
};

#endif // PJRC_RAWHID_H
