/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{ 
 * @addtogroup FlightRecorderModule FlightRecorder Module
 * @brief Main flightrecorder module
 * Starts three tasks (RX, TX, and priority TX) that watch event queues
 * and handle all the telemetry of the UAVobjects
 * @{ 
 *
 * @file       flightrecorder.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      FlightRecorder module, logs UAVObjects
 * @see        The GNU Public License (GPL) Version 3
 *
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

#include "openpilot.h"
#include "flightrecordersettings.h"

// Private constants
#define MAX_QUEUE_SIZE   TELEM_QUEUE_SIZE
#define STACK_SIZE_BYTES PIOS_FLIGHTREC_STACK_SIZE
#define TASK_PRIORITY_TX (tskIDLE_PRIORITY + 2)
#define REQ_TIMEOUT_MS 250

// Private types

// Private variables
static xQueueHandle queue;

static xTaskHandle flightRecorderTaskHandle;
static FlightRecorderSettingsData settings;
static UAVTalkConnection uavTalkCon;

// Private functions
static void flightRecorderTask(void *parameters);
static int32_t transmitData(uint8_t * data, int32_t length);
static void registerObject(UAVObjHandle obj);
static void updateObject(UAVObjHandle obj);
static int32_t addObject(UAVObjHandle obj);
static int32_t setUpdatePeriod(UAVObjHandle obj, int32_t updatePeriodMs);
static void processObjEvent(UAVObjEvent * ev);
static void updateSettings();

/**
 * Initialise the flightRecorder module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t FlightRecorderInitialize(void)
{
	// Create object queues
	queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(UAVObjEvent));
	
	// Get flightRecorder settings object
	updateSettings();

	// Initialise UAVTalk
	UAVTalkInitialize(&uavTalkCon, &transmitData);

	// Process all registered objects and connect queue for updates
	UAVObjIterate(&registerObject);

	// Listen to objects of interest
	FlightRecorderSettingsConnectQueue(queue);

	// Start flightRecorder tasks
	xTaskCreate(flightRecorderTask, (signed char *)"FlightRecorder", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY_TX, &flightRecorderTaskHandle);
	TaskMonitorAdd(TASKINFO_RUNNING_FLIGHTRECORDER, flightRecorderTaskHandle);
	
	return 0;
}

/**
 * Register a new object, adds object to local list and connects the queue depending on the object's
 * flightRecorder settings.
 * \param[in] obj Object to connect
 */
static void registerObject(UAVObjHandle obj)
{
	// Setup object for periodic updates
	addObject(obj);

	// Setup object for flightRecorder updates
	updateObject(obj);
}

/**
 * Update object's queue connections and timer, depending on object's settings
 * \param[in] obj Object to updates
 */
static void updateObject(UAVObjHandle obj)
{
	UAVObjMetadata metadata;
	int32_t eventMask;

	// Get metadata
	UAVObjGetMetadata(obj, &metadata);

	// Setup object depending on update mode
	if (metadata.loggingUpdateMode == UPDATEMODE_PERIODIC) {
		// Set update period
		setUpdatePeriod(obj, metadata.loggingUpdatePeriod);
		// Connect queue
		eventMask = EV_UPDATED_MANUAL | EV_UPDATE_REQ;
		if (UAVObjIsMetaobject(obj)) {
			eventMask |= EV_UNPACKED;	// we also need to act on remote updates (unpack events)
		}
		UAVObjConnectQueue(obj, queue, eventMask);
	} else if (metadata.loggingUpdateMode == UPDATEMODE_ONCHANGE) {
		// Set update period
		setUpdatePeriod(obj, 0);
		// Connect queue
		eventMask = EV_UPDATED | EV_UPDATED_MANUAL | EV_UPDATE_REQ;
		if (UAVObjIsMetaobject(obj)) {
			eventMask |= EV_UNPACKED;	// we also need to act on remote updates (unpack events)
		}
		UAVObjConnectQueue(obj, queue, eventMask);
	} else if (metadata.loggingUpdateMode == UPDATEMODE_MANUAL) {
		// Set update period
		setUpdatePeriod(obj, 0);
		// Connect queue
		eventMask = EV_UPDATED_MANUAL | EV_UPDATE_REQ;
		if (UAVObjIsMetaobject(obj)) {
			eventMask |= EV_UNPACKED;	// we also need to act on remote updates (unpack events)
		}
		UAVObjConnectQueue(obj, queue, eventMask);
	} else if (metadata.loggingUpdateMode == UPDATEMODE_NEVER) {
		// Set update period
		setUpdatePeriod(obj, 0);
		// Disconnect queue
		UAVObjDisconnectQueue(obj, queue);
	}
}

/**
 * Processes queue events
 */
static void processObjEvent(UAVObjEvent * ev)
{
	UAVObjMetadata metadata;

	if (ev->obj != 0) {
		if (ev->obj == FlightRecorderSettingsHandle())  updateSettings();
		// Get object metadata
		UAVObjGetMetadata(ev->obj, &metadata);
		// Act on event
		if (ev->event == EV_UPDATED || ev->event == EV_UPDATED_MANUAL) {
			// Only invoke UAVTalk if we actually are logging. No overhead if not required.
			//if (settings.Enabled==FLIGHTRECORDERSETTINGS_ENABLED_TRUE) {
				UAVTalkSendObject(&uavTalkCon, ev->obj, ev->instId, 0, REQ_TIMEOUT_MS);	// do not wait for "ack"
			//}
		}
		// If this is a metaobject then make necessary updates
		if (UAVObjIsMetaobject(ev->obj)) {
			updateObject(UAVObjGetLinkedObj(ev->obj));	// linked object will be the actual object the metadata are for
		}
	}
}

/**
 * FlightRecorder transmit task, regular priority
 */
static void flightRecorderTask(void *parameters)
{
	UAVObjEvent ev;

	// Loop forever
	while (1) {
		// Wait for queue message
		if (xQueueReceive(queue, &ev, portMAX_DELAY) == pdTRUE) {
			// Process event
			processObjEvent(&ev);
		}
	}
}

/**
 * Transmit data buffer to disk - prepend time stamp for log file
 * \param[in] data Data buffer to send
 * \param[in] length Length of buffer
 * \return 0 Success
 */


static int32_t transmitData(uint8_t * data, int32_t length)
{

#if defined(PIOS_INCLUDE_SDCARD)

	static uint32_t startTime = 0;
	static uint8_t logfile = 0;
	static uint8_t logging = 0;
	static FILEINFO fileinfo;

	char filename[]="logXXX.opl";
	uint32_t systemTime = xTaskGetTickCount()*portTICK_RATE_MS;
	uint32_t saveTime;
	uint64_t datalength = length;
	uint32_t written = 0;

	if (settings.Enabled==FLIGHTRECORDERSETTINGS_ENABLED_TRUE) {
		if (!logging) {
			// new log file - assign
			logging = 1;
			logfile = settings.Filename + 1;
			// filename - crude code to turn a number into a decimal string
			filename[3] = 48 + (logfile/100);
			filename[4] = 48 + ((logfile%100)/10);
			filename[5] = 48 + (logfile%10);
			// create new file
			if(PIOS_FOPEN_WRITE(filename, &fileinfo)) {
				logging=0;
			}
			// store new filename
			FlightRecorderSettingsFilenameSet(&logfile);
			// save UAVObject to disk
			UAVObjSave(FlightRecorderSettingsHandle(),0);


			//set start time
			startTime = systemTime;
		}
		if (logging) {

			saveTime = systemTime - startTime;
			PIOS_FWRITE(&fileinfo, &saveTime, 4,
				  &written);
			PIOS_FWRITE(&fileinfo, &datalength, 8,
				  &written);
			PIOS_FWRITE(&fileinfo, data, length,
				  &written);
		}
	} else {
		if (logging) {
			logging = 0;
			PIOS_FCLOSE(&fileinfo);

			// save UAVObject to disk
			UAVObjSave(FlightRecorderSettingsHandle(),0);
		}
	}

#endif // we need SDcard - or something equivalent

	return length;

}

/**
 * Setup object for periodic updates.
 * \param[in] obj The object to update
 * \return 0 Success
 * \return -1 Failure
 */
static int32_t addObject(UAVObjHandle obj)
{
	UAVObjEvent ev;

	// Add object for periodic updates
	ev.obj = obj;
	ev.instId = UAVOBJ_ALL_INSTANCES;
	ev.event = EV_UPDATED_MANUAL;
	return EventPeriodicQueueCreate(&ev, queue, 0);
}

/**
 * Set update period of object (it must be already setup for periodic updates)
 * \param[in] obj The object to update
 * \param[in] updatePeriodMs The update period in ms, if zero then periodic updates are disabled
 * \return 0 Success
 * \return -1 Failure
 */
static int32_t setUpdatePeriod(UAVObjHandle obj, int32_t updatePeriodMs)
{
	UAVObjEvent ev;

	// Add object for periodic updates
	ev.obj = obj;
	ev.instId = UAVOBJ_ALL_INSTANCES;
	ev.event = EV_UPDATED_MANUAL;
	return EventPeriodicQueueUpdate(&ev, queue, updatePeriodMs);
}

/**
 * Update the flightRecorder settings, called on startup and
 * each time the settings object is updated
 */
static void updateSettings()
{
    // Retrieve settings
    FlightRecorderSettingsGet(&settings);
}

/**
  * @}
  * @}
  */
