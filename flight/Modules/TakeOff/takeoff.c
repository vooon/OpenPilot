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
static void updateDesiredAttitude();
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


static uint8_t takeOffModeLast = 0;
static int32_t takeOffTargetAltitude = 0;
static float verticalVelIntegral = 0;

#define OLD_ALTITUDE_UNSET -10000
#define OLD_ALTITUDE_TEST_UNSET -9000

static float oldAltitude = OLD_ALTITUDE_UNSET;

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
			// Initialize when mode gets engaged
			if (takeOffModeLast == 0) {
				BaroAltitudeData baroAltitude;

				takeOffModeLast = 1;
				BaroAltitudeGet(&baroAltitude);
				takeOffTargetAltitude = baroAltitude.Altitude * 100 + takeOffSettings.TargetAltitude;

			}

			updateDesiredAttitude();

		} else {
			// reset
			takeOffModeLast = 0;
			takeOffTargetAltitude = 0;
			verticalVelIntegral = 0;
			oldAltitude = OLD_ALTITUDE_UNSET;

		}
		
	}
}

/**
 * Actual TakeOff Sequence
 */
static void updateDesiredAttitude()
{

	static portTickType lastSysTime;
	portTickType thisSysTime = xTaskGetTickCount();
	float dT;
	
	BaroAltitudeData baroAltitude;
	TakeOffSettingsData takeOffSettings;
	StabilizationDesiredData stabDesired;

	float altitudeError;
	float altitudeDelta;
	float verticalVelocity;
	float verticalError;


	// Check how long since last update
	if(thisSysTime > lastSysTime) // reuse dt in case of wraparound
		dT = (thisSysTime - lastSysTime) / portTICK_RATE_MS / 1000.0f;		
	lastSysTime = thisSysTime;

	// Read out UAVObjects
	TakeOffSettingsGet(&takeOffSettings);
	BaroAltitudeGet(&baroAltitude);
	StabilizationDesiredGet(&stabDesired);

	// Compute vertical velocity from barometric altitude
	// NOTE: no filtering right now, altitude module already does filtering
	if (oldAltitude < OLD_ALTITUDE_TEST_UNSET) {
		oldAltitude = baroAltitude.Altitude * 100;
		altitudeDelta = 0;
	} else {
		altitudeDelta = ((baroAltitude.Altitude * 100) - oldAltitude) / dT;
	}

	// Compute desired vertical velocity (proportional with max)
	altitudeError = takeOffTargetAltitude - baroAltitude.Altitude * 100;
	verticalVelocity = bound(
		altitudeError * takeOffSettings.AltitudeP[TAKEOFFSETTINGS_ALTITUDEP_KP],
		- takeOffSettings.AltitudeP[TAKEOFFSETTINGS_ALTITUDEP_MAX],
		takeOffSettings.AltitudeP[TAKEOFFSETTINGS_ALTITUDEP_MAX]
		);
	
	// Compute desired pitch (PI loop)
	verticalError = verticalVelocity - altitudeDelta;
	verticalVelIntegral = bound (
		verticalVelIntegral + verticalError * dT * takeOffSettings.VerticalPI[TAKEOFFSETTINGS_VERTICALPI_KI],
		takeOffSettings.VerticalPI[TAKEOFFSETTINGS_VERTICALPI_ILIMIT],
		-takeOffSettings.VerticalPI[TAKEOFFSETTINGS_VERTICALPI_ILIMIT]
		);
	stabDesired.Pitch = bound (
		verticalError * takeOffSettings.VerticalPI[TAKEOFFSETTINGS_VERTICALPI_KP] + verticalVelIntegral,
		- takeOffSettings.MaxPitch,
		takeOffSettings.MaxPitch
		);

	// Compute desired throttle
	// throttle is takeoff throttle for 90% of climb
	// and hold throttle if within the last 10%
	if ( (baroAltitude.Altitude * 100) < takeOffTargetAltitude - (takeOffSettings.TargetAltitude / 10) ) {
		stabDesired.Throttle = takeOffSettings.Throttle[TAKEOFFSETTINGS_THROTTLE_CLIMB];
	} else {
		stabDesired.Throttle = takeOffSettings.Throttle[TAKEOFFSETTINGS_THROTTLE_HOLD];
	}

printf("TakeOffMode:\n");
printf("Altitude: %i	- Desired: %i\n",(int)(baroAltitude.Altitude*100),takeOffTargetAltitude);
printf("V-Speed: %f	- Desired: %f\n",altitudeDelta,verticalVelocity);
printf("Pitch: %f\n",stabDesired.Pitch);

	// fly straight
	stabDesired.Yaw = 0;

	// fly level
	stabDesired.Roll = 0;

	stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_ROLL] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
	stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_PITCH] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
	stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_YAW] = STABILIZATIONDESIRED_STABILIZATIONMODE_RATE;
	
	StabilizationDesiredSet(&stabDesired);
	
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
