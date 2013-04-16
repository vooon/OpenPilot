#import <Foundation/Foundation.h>
#import "usbmonitor.h"
#import "Logger"
#import <assert.h>

static bool HID_GetStrProperty(IOHIDDeviceRef dev, CFStringRef property, CString& value);
static bool HID_GetIntProperty(IOHIDDeviceRef dev, CFStringRef property, int * value);
bool cfstring2cstring(CFStringRef str, CString& result);

UsbMonitor::UsbMonitor()
{
    m_loop       = 0;
    m_hidManager = NULL;

    m_terminate = false;

    m_thread.start(this, &UsbMonitor::run);
}

UsbMonitor::~UsbMonitor()
{
    m_terminate = true;
    m_thread.join();
}

void UsbMonitor::lock() const
{
    m_listMutex.lock();
}

void UsbMonitor::unlock() const
{
    m_listMutex.unlock();
}

const opvector<pjrc_rawhid>& UsbMonitor::knownDevices() const
{
    return m_knownDevices;
}

void UsbMonitor::onUpdate()
{
    CMutexSection locker(&m_listMutex);
    for (auto it = m_knownDevices.begin();it<m_knownDevices.end();it++)
    {
        (*it).onUpdate();
    }
}

//===================================================================
//  p r i v a t e   f u n c t i o n s
//===================================================================

void UsbMonitor::attach_callback(void *context, IOReturn /*r*/, void */*hid_mgr*/, IOHIDDeviceRef dev)
{
    UsbMonitor* monitor = static_cast<UsbMonitor*>(context);

    bool got_properties = true;

    pjrc_rawhid deviceInfo;
    deviceInfo.d->m_monitor = monitor;

    deviceInfo.d->m_devHandle = dev;

    Logger() << "USBMonitor: Device attached event";

    // Populate the device info structure
    got_properties &= HID_GetIntProperty(dev, CFSTR(kIOHIDVendorIDKey), &deviceInfo.d->m_vendorID);
    if (deviceInfo.d->m_vendorID != idVendor_OpenPilot)
        return;             // we are wait only OpenPilot devices
    got_properties &= HID_GetIntProperty(dev, CFSTR(kIOHIDProductIDKey), &deviceInfo.d->m_productID);
    got_properties &= HID_GetIntProperty(dev, CFSTR(kIOHIDVersionNumberKey), &deviceInfo.d->m_bcdDevice);
    got_properties &= HID_GetStrProperty(dev, CFSTR(kIOHIDSerialNumberKey), deviceInfo.d->m_serialNumber);
    got_properties &= HID_GetStrProperty(dev, CFSTR(kIOHIDProductKey), deviceInfo.d->m_product);
    got_properties &= HID_GetStrProperty(dev, CFSTR(kIOHIDManufacturerKey), deviceInfo.d->m_manufacturer);
    // TOOD: Eventually want to take array of usages if devices start needing that
    got_properties &= HID_GetIntProperty(dev, CFSTR(kIOHIDPrimaryUsageKey), &deviceInfo.d->m_usage);
    got_properties &= HID_GetIntProperty(dev, CFSTR(kIOHIDPrimaryUsagePageKey), &deviceInfo.d->m_usagePage);

    // Currently only enumerating objects that have the complete list of properties
    if(got_properties) {
        Logger() << "USBMonitor: Adding device" << deviceInfo.d->m_product;
        monitor->addDevice(deviceInfo);
    }
}

void UsbMonitor::detach_callback(void *context, IOReturn r, void *hid_mgr, IOHIDDeviceRef dev)
{
    r       = r;
    hid_mgr = hid_mgr;

    UsbMonitor* monitor = static_cast<UsbMonitor*>(context);

    Logger() << "USBMonitor: Device detached event";
    monitor->removeDevice(dev);
}

void UsbMonitor::addDevice(const pjrc_rawhid& info)
{
    CMutexSection locker(&m_listMutex);
    for (opvector<pjrc_rawhid>::const_iterator it = m_knownDevices.begin();it<m_knownDevices.end();it++) {
        if (info.d->m_devHandle == (*it).d->m_devHandle) {
            return;
        }
    }
    m_knownDevices.push_back(info);
    m_signalDeviceDiscovered.invoke(info);
}

void UsbMonitor::removeDevice(IOHIDDeviceRef dev)
{
    CMutexSection locker(&m_listMutex);
    for (int i = 0; i < int (m_knownDevices.size()); i++)
    {
        pjrc_rawhid port = m_knownDevices.at(i);
        if (port.d->m_devHandle == dev)
        {
            port.deviceDetached();
            m_knownDevices.erase(m_knownDevices.begin()+i);
            m_signalDeviceRemoved.invoke(port);
            return;
        }
    }
}

void UsbMonitor::run()
{
    IOReturn ret;

    m_hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (m_hidManager == NULL || CFGetTypeID(m_hidManager) != IOHIDManagerGetTypeID()) {
        if (m_hidManager) {
            CFRelease(m_hidManager);
        }
        assert(0);
    }

    // No matching filter
    IOHIDManagerSetDeviceMatching(m_hidManager, NULL);

    m_loop = CFRunLoopGetCurrent();
    // set up a callbacks for device attach & detach
    IOHIDManagerScheduleWithRunLoop(m_hidManager, m_loop, CFSTR(OP_LOOPMODE_NAME_MAC));
    IOHIDManagerRegisterDeviceMatchingCallback(m_hidManager, attach_callback, this);
    IOHIDManagerRegisterDeviceRemovalCallback(m_hidManager, detach_callback, this);
    ret = IOHIDManagerOpen(m_hidManager, kIOHIDOptionsTypeNone);
    if (ret != kIOReturnSuccess) {
        IOHIDManagerUnscheduleFromRunLoop(m_hidManager, m_loop, CFSTR("Open_Pilot_Loop_Mode"));
        CFRelease(m_hidManager);
        return;
    }
    while(!m_terminate) {
        CFRunLoopRunInMode(CFSTR("Open_Pilot_Loop_Mode"), 1, false);
    }
    IOHIDManagerUnscheduleFromRunLoop(m_hidManager, m_loop, CFSTR("Open_Pilot_Loop_Mode"));
    CFRelease(m_hidManager);
}


/**
  * @brief Helper function get get a HID integer property
  * @param[in] dev Device reference
  * @param[in] property The property to get (constants defined in IOKIT)
  * @param[out] value Pointer to integer to set
  * @return True if successful, false otherwise
  */
static bool HID_GetIntProperty(IOHIDDeviceRef dev, CFStringRef property, int * value) {
    CFTypeRef prop = IOHIDDeviceGetProperty(dev, property);
    if (prop) {
        if (CFNumberGetTypeID() == CFGetTypeID(prop)) { // if a number
            CFNumberGetValue((CFNumberRef) prop, kCFNumberSInt32Type, value);
            return true;
        }
    }
    return false;
}

/**
  * @brief Helper function get get a HID string property
  * @param[in] dev Device reference
  * @param[in] property The property to get (constants defined in IOKIT)
  * @param[out] value The QString to set
  * @return True if successful, false otherwise
  */
static bool HID_GetStrProperty(IOHIDDeviceRef dev, CFStringRef property, CString& value) {
    CFTypeRef prop = IOHIDDeviceGetProperty(dev, property);
    if (prop) {
        if (CFStringGetTypeID() == CFGetTypeID(prop)) { // if a string
            bool success = cfstring2cstring((CFStringRef)prop, value);
            return success;
        }
    }
    return false;
}


bool cfstring2cstring(CFStringRef str, CString& result)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    NSString* nsStr = (NSString*)str;

    const char* cstr = [nsStr UTF8String];
    result = cstr;
    [pool release];
    return cstr != 0 ? true : false;
}
