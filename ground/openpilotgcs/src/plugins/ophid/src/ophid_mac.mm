/**
 ******************************************************************************
 *
 * @file       rawhid.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup opHIDPlugin HID Plugin
 * @{
 * @brief Impliments a HID USB connection to the flight hardware as a QIODevice
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#import <QtDebug>
#import <QtGlobal>
#import <QWaitCondition>
#import "ophid.h"
#import "ophid_usbmon.h"

#define MAX_PACKET_SIZE 64

/**
 *  This class is nearly the same than RawHIDReadThread but for writing
 */
class RawHIDWriteThread : public QThread {
public:
	RawHIDWriteThread(RawHID *hid);
	virtual ~RawHIDWriteThread();

	/** Add some data to be written without waiting */
	int pushDataToWrite(const char *data, int size);

	/** Return the number of bytes buffered */
	qint64 getBytesToWrite();

public slots:
	void terminate() {
		terminate(false);
	}

	void terminate(bool wait)
	{
		m_running = false;
		if (wait) {
			if (QThread::wait(10000) == false) {
				qDebug() << "Cannot terminate RawHIDReadThread";
				assert(false);
			}
		}
	}

protected:
	void run();

	/** QByteArray might not be the most efficient way to implement
	   a circular buffer but it's good enough and very simple */
	QByteArray m_writeBuffer;

	/** A mutex to protect read buffer */
	QMutex m_writeBufMtx;

	/** Synchronize task with data arival */
	QWaitCondition m_newDataToWrite;

	RawHID *m_hid;

	bool m_running;
};

RawHIDWriteThread::RawHIDWriteThread(RawHID *hid)
	: m_hid(hid),
	m_running(true)
{}

// *********************************************************************************

RawHIDWriteThread::~RawHIDWriteThread()
{
	terminate(true);
	m_running = false;
}

void RawHIDWriteThread::run()
{
	while (m_running) {
		char buffer[MAX_PACKET_SIZE] = { 0 };

		m_writeBufMtx.lock();
		int size = qMin(MAX_PACKET_SIZE - 2, m_writeBuffer.size());
		while (size <= 0) {
			// wait on new data to write condition, the timeout
			// enable the thread to shutdown properly
			m_newDataToWrite.wait(&m_writeBufMtx, 200);
			if (!m_running)
				return;

			size = qMin(MAX_PACKET_SIZE - 2, m_writeBuffer.size());
		}

		// NOTE: data size is limited to 2 bytes less than the
		// usb packet size (64 bytes for interrupt) to make room
		// for the reportID and valid data length
		memcpy(&buffer[2], m_writeBuffer.constData(), size);
		buffer[1] = size; // valid data length
		buffer[0] = 2; // reportID
		m_writeBufMtx.unlock();

		// must hold lock through the send to know how much was sent
		IOReturn ret = kIOReturnInvalid;
		if (m_hid->m_handle) {
			ret = IOHIDDeviceSetReport(m_hid->m_handle, kIOHIDReportTypeOutput, 2, (const uint8_t*)buffer, MAX_PACKET_SIZE);
		}
		if (ret != kIOReturnSuccess) {
			// TODO! make proper error handling, this only quick hack for unplug freeze
			m_running = false;
			qDebug() << "Error writing to device (" << ret << ")";
			break;
		} else {
			// only remove the size actually written to the device
			{
				QMutexLocker lock(&m_writeBufMtx);
				m_writeBuffer.remove(0, size);
			}
			emit m_hid->bytesWritten(size);
		}
	}
}

int RawHIDWriteThread::pushDataToWrite(const char *data, int size)
{
	QMutexLocker lock(&m_writeBufMtx);

	m_writeBuffer.append(data, size);
	m_newDataToWrite.wakeOne(); // signal that new data arrived

	return size;
}

qint64 RawHIDWriteThread::getBytesToWrite()
{
	// QMutexLocker lock(&m_writeBufMtx);
	return m_writeBuffer.size();
}



RawHID::RawHID()
{
	assert(false);
}

RawHID::RawHID(const QString &deviceName)
{
	serialNumber = deviceName;
	m_handle = 0;

	m_deviceNo = 0;
	device_open = false;

	m_readThread = 0;
	m_writeThread = 0;

	m_mutex = new QMutex(QMutex::Recursive);
	m_startedMutex = 0;

	m_writeThread = new RawHIDWriteThread(this);

	openDevice();
}

RawHID::~RawHID()
{
	close();
}

bool RawHID::open(OpenMode mode)
{
	return QIODevice::open(mode);
}

void RawHID::close()
{
	closeDevice();

	QIODevice::close();
}

bool RawHID::isSequential() const
{
	return true;
}

void RawHID::onDeviceUnplugged(int num)
{
	Q_UNUSED(num);
	assert(false);
}

//===================================================================
//  p r o t e c t e d   f u n c t i o n s
//===================================================================

qint64 RawHID::readData(char *data, qint64 maxSize)
{
	int recv = 0;
	int size = 0;
	while(maxSize)
	{
		if (!device_open)
			return 0;
		{
			QMutexLocker locker(m_mutex);
			if (maxSize > int (m_rcvData.size()))
				size = int (m_rcvData.size());
			else
				size = maxSize;
			if (data) {
				memcpy(data, m_rcvData.data(), size);
			}
			maxSize -= size;
			recv += size;
			m_rcvData.remove(0, size);
			if (m_rcvData.size() == 0) {
				return recv;
			}
		}
	}
	return recv;
}

qint64 RawHID::writeData(const char *data, qint64 maxSize)
{
	if (!device_open)
		return -1;
	QMutexLocker locker(m_mutex);

	if (!m_writeThread || !data) {
		return -1;
	}
	return m_writeThread->pushDataToWrite(data, maxSize);
}

qint64 RawHID::bytesAvailable() const
{
	QMutexLocker locker(m_mutex);
	return m_rcvData.size() + QIODevice::bytesAvailable();
}

qint64 RawHID::bytesToWrite() const
{
	return m_writeThread->getBytesToWrite() + QIODevice::bytesToWrite();
}

// Callback from the read thread to open the device
bool RawHID::openDevice()
{
	assert(device_open == false);
	if (device_open) {
		return false;
	}
	USBMonitor* monitor = USBMonitor::instance();
	if (!monitor) {
		return false;
	}
	connect(monitor, SIGNAL(deviceDiscovered(QString)), this, SLOT(deviceDetached(QString)), Qt::DirectConnection);
	USBPortInfo port = monitor->deviceBySerialNumber(serialNumber);
	if (port.serialNumber.isEmpty() || port.serialNumber != serialNumber) {
		return false;
	}
	m_handle = port.dev_handle;
	IOReturn err = IOHIDDeviceOpen(m_handle, kIOHIDOptionsTypeNone);
	if (err != kIOReturnSuccess)
	{
		qDebug() << __PRETTY_FUNCTION__ << ": error opening device. err = " << err;
		return false;
	}
	qDebug() << __PRETTY_FUNCTION__ << ": sucessfully opening device";
	IOHIDDeviceScheduleWithRunLoop(m_handle, monitor->m_loop, kCFRunLoopDefaultMode);
	IOHIDDeviceRegisterInputReportCallback(m_handle, m_buffer, sizeof(m_buffer), &RawHID::input_callback, this);
	device_open = true;

	m_writeThread->start();

	return true;
}

// Callback from teh read thread to close the device
bool RawHID::closeDevice()
{
	if (!device_open) {
		return false;
	}
	m_writeThread->terminate(true);

	QMutexLocker lockerMutex(m_mutex);

	USBMonitor* monitor = USBMonitor::instance();
	assert(monitor != 0);
	IOHIDDeviceClose(m_handle, kIOHIDOptionsTypeNone);
	IOHIDDeviceRegisterInputReportCallback(m_handle, m_buffer, sizeof(m_buffer), NULL, NULL);
	IOHIDDeviceUnscheduleFromRunLoop(m_handle, monitor->m_loop, kCFRunLoopDefaultMode);
	device_open = false;
	return true;
}

//! Callback for the HID driver on an input report
void RawHID::input_callback(void *c, IOReturn ret, void* /*sender*/, IOHIDReportType /*type*/, uint32_t /*id*/, uint8_t* data, CFIndex len)
{
	if (ret != kIOReturnSuccess || len < 1) {
		return;
	}

	RawHID* context = (RawHID*)c;
	context->input(data, len);
}

void RawHID::input(uint8_t *data, CFIndex len)
{
	assert(device_open);
	if (!device_open) {
		return;
	}

	assert(len <= BUFFER_LENGTH);

	QMutexLocker locker(m_mutex);
	m_rcvData.append((const char*)(data+2), data[1]);

	emit this->readyRead();
//	QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
}

void RawHID::deviceDetached(const QString& serialNumber)
{
	if (serialNumber != this->serialNumber) {
		return;
	}
	closeDevice();
}
