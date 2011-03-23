/**
 ******************************************************************************
 *
 * @file       takeoff.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      This module sequences automatic takeoff for fixed wing, based on
 *             barometric altitude
 *
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

/**
 * Input object: BaroAltitude
 * Input object: ManualControlCommand
 * Output object: PositionDesired
 * Output object: StabilizationDesired
 *
 * This module will periodically update the value of the StabilizationDesired
 * object.
 *
 * The module executes in its own thread in this example.
 *
 * Modules have no API, all communication to other modules is done through UAVObjects.
 * However modules may use the API exposed by shared libraries.
 * See the OpenPilot wiki for more details.
 * http://www.openpilot.org/OpenPilot_Application_Architecture
 *
 */

#include "openpilot.h"
#include "takeoff.h"
#include "takeoffsettings.h"
#include "attituderaw.h"
#include "attitudeactual.h"
#include "stabilizationdesired.h"
#include "positiondesired.h"
#include "baroaltitude.h"
#include "manualcontrolcommand.h"
#include "systemsettings.h"
#include "CoordinateConversions.h"

// Private constants
#define MAX_QUEUE_SIZE 1
#define STACK_SIZE_BYTES 1024
#define TASK_PRIORITY (tskIDLE_PRIORITY+2)
// Private types

// Private variables
static xTaskHandle takeOffTaskHandle;
static xQueueHandle queue;

// Private functions
static void takeOffTask(void *parameters);
static float bound(float val, float min, float max);

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t TakeOffInitialize()
{
	// Create object queue
	queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(UAVObjEvent));
	
	// Listen for updates.
	BaroAltitudeConnectQueue(queue);
	
	// Start main task
	xTaskCreate(takeOffTask, (signed char *)"TakeOff", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &takeOffTaskHandle);
	TaskMonitorAdd(TASKINFO_RUNNING_TAKEOFF, takeOffTaskHandle);

	return 0;
}

/**
 * Module thread, should not return.
 */
static void takeOffTask(void *parameters)
{
	SystemSettingsData systemSettings;
	TakeOffSettingsData takeOffSettings;
	ManualControlCommandData manualControl;

	portTickType thisTime;
	portTickType lastUpdateTime;
	UAVObjEvent ev;
	
	// Main task loop
	lastUpdateTime = xTaskGetTickCount();
	while (1) {
		TakeOffSettingsGet(&takeOffSettings);

		// Wait until the AttitudeRaw object is updated, if a timeout then go to failsafe
		if ( xQueueReceive(queue, &ev, takeOffSettings.UpdatePeriod / portTICK_RATE_MS) != pdTRUE )
		{
			AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_WARNING);
		} else {
			AlarmsClear(SYSTEMALARMS_ALARM_GUIDANCE);
		}
				
		// Continue collecting data if not enough time
		thisTime = xTaskGetTickCount();
		if( (thisTime - lastUpdateTime) < (takeOffSettings.UpdatePeriod / portTICK_RATE_MS) )
			continue;
		
		
		if ((manualControl.FlightMode == MANUALCONTROLCOMMAND_FLIGHTMODE_TAKEOFF) &&
		    ((systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWING) ||
		     (systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGELEVON) ||
		     (systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGVTAIL) ))
		{
			// do stuff
			bound(1,0,1);
		}
		
	}
}


/**
 * Bound input value between limits
 */
static float bound(float val, float min, float max)
{
	if (val < min) {
		val = min;
	} else if (val > max) {
		val = max;
	}
	return val;
}
