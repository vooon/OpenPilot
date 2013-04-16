#ifndef CUSBMONITOR_H
#define CUSBMONITOR_H

#include "signals/Signals"
#include "thread/CThread"
#include "thread/CMutex"
#include "text/CString"
#include "OPVector"
#include "pjrc_rawhid.h"

#define OP_LOOPMODE_NAME_MAC "Open_Pilot_Loop_Mode"

class UsbMonitor
{
friend class pjrc_rawhid;

public:
    enum RunState {
        Bootloader = 0x01,
        Running = 0x02
    };

    enum USBConstants {
        idVendor_OpenPilot = 0x20a0,
        idProduct_OpenPilot = 0x415a,
        idProduct_CopterControl = 0x415b,
        idProduct_PipXtreme = 0x415c
    };

public:
    UsbMonitor();
    ~UsbMonitor();

    void lock() const;
    void unlock() const;

    const opvector<pjrc_rawhid>& knownDevices() const;
    
    void onUpdate();

// signals
    CL_Signal_v1<const pjrc_rawhid&>& signal_deviceDiscovered() {
        return m_signalDeviceDiscovered;
    }
    CL_Signal_v1<const pjrc_rawhid&>& signal_deviceRemoved() {
        return m_signalDeviceRemoved;
    }

private:
    CThread m_thread;
    bool    m_terminate;

    mutable CMutex        m_listMutex;
    opvector<pjrc_rawhid> m_knownDevices;

    CL_Signal_v1<const pjrc_rawhid&> m_signalDeviceDiscovered;
    CL_Signal_v1<const pjrc_rawhid&> m_signalDeviceRemoved;

    // Depending on the OS, we'll need different things:
#if defined(__APPLE__) //&& !defined(IOS_PROJECT)
    static void attach_callback(void *context, IOReturn r, void *hid_mgr, IOHIDDeviceRef dev);
    static void detach_callback(void *context, IOReturn r, void *hid_mgr, IOHIDDeviceRef dev);
    void addDevice(const pjrc_rawhid& info);
    void removeDevice(IOHIDDeviceRef dev);

    CFRunLoopRef    m_loop;
    IOHIDManagerRef m_hidManager;
#endif

    void run();
};

#endif // CUSBMONITOR_H
