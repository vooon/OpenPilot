/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{ 
 * @addtogroup AirspeedModule Airspeed Module
 * @brief Communicate with the EagleTree V3 Airspeed sensor (ETASV3) and update @ref BaroAirspeed "BaroAirspeed UAV Object"
 * @{ 
 *
 * @file       airspeed.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Airspeed module, handles airspeed readings from ETASV3
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
\
/**
 * Output object: BaroAirspeed
 *
 * This module will periodically update the value of the BaroAirspeed object.
 *
 */

#include "openpilot.h"
#include "hwsettings.h"
#include "airspeed.h"
#include "baroairspeed.h"	// object that will be updated by the module
#include "baroairspeedsettings.h"

// Private constants
#define STACK_SIZE_BYTES 270
#define TASK_PRIORITY (tskIDLE_PRIORITY+1)

int32_t AirspeedInitialize();

// Private types

// Private variables
static xTaskHandle taskHandle;
static bool airspeedEnabled = false;

// Private functions
static void airspeedTask(void *parameters);

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AirspeedStart()
{	
	if (airspeedEnabled == false) {
		return -1;
	}
	// Start main task
	xTaskCreate(airspeedTask, (signed char *)"Airspeed", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &taskHandle);
	TaskMonitorAdd(TASKINFO_RUNNING_AIRSPEED, taskHandle);

	return 0;
}

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AirspeedInitialize()
{
	#ifdef MODULE_AIRSPEED_BUILTIN
		airspeedEnabled = true;
	#else

		HwSettingsInitialize();
		uint8_t optionalModules[HWSETTINGS_OPTIONALMODULES_NUMELEM];
		HwSettingsOptionalModulesGet(optionalModules);

		if (optionalModules[HWSETTINGS_OPTIONALMODULES_AIRSPEED] == HWSETTINGS_OPTIONALMODULES_ENABLED) {
			airspeedEnabled = true;
		} else {
			airspeedEnabled = false;
			return -1;
		}
	#endif
	
	//Initialize the UAVObjects
	BaroAirspeedInitialize();
	BaroAirspeedSettingsInitialize();

	return 0;
}
MODULE_INITCALL(AirspeedInitialize, AirspeedStart)
/**
 * Module thread, should not return.
 */
static void airspeedTask(void *parameters)
{
	BaroAirspeedData data;
	BaroAirspeedSettingsData settings;
	
	uint8_t calibrationCount = 0;
	uint32_t calibrationSum = 0;
	uint8_t calibrationIdle = 0;
	uint8_t calibrationIdle_old = 0;
	uint8_t calibrationSamples = 0;
	uint8_t calibrationSamples_old = 0;
	
	// Main task loop
	while (1)
	{
		BaroAirspeedGet(&data);
		BaroAirspeedSettingsGet(&settings);

		// Update the airspeed
		vTaskDelay(settings.Sample_Period_ms);

		data.SensorValue = PIOS_ETASV3_ReadAirspeed();
		if ((int16_t)data.SensorValue==-1) {
			data.Connected = BAROAIRSPEED_CONNECTED_FALSE;
			data.Airspeed = 0;
			BaroAirspeedSet(&data);
			continue;
		}

		data.Connected = BAROAIRSPEED_CONNECTED_TRUE;

		//store old values
		calibrationIdle_old = calibrationIdle;
		calibrationSamples_old = calibrationIdle;

		calibrationIdle = settings.CalibrationIdle;
		calibrationSamples = settings.CalibrationSamples;

		//if settings have changed, start calibration again
		if ((calibrationIdle != calibrationIdle_old) || (calibrationSamples != calibrationSamples_old)) {
			calibrationCount = 0;
		}

		if (calibrationCount < calibrationIdle) {
			calibrationCount++;
		} else if (calibrationCount < calibrationIdle + calibrationSamples) {
			calibrationCount++;
			calibrationSum +=  data.SensorValue;
			if (calibrationCount == calibrationIdle + calibrationSamples) {
				data.ZeroPoint = calibrationSum / calibrationSamples;
			}
		}

		if (settings.CalibrationMode == BAROAIRSPEEDSETTINGS_CALIBRATIONMODE_AUTO)
			data.Airspeed = settings.Scale * sqrtf((float)abs(data.SensorValue - data.ZeroPoint));
		else
			data.Airspeed = settings.Scale * sqrtf((float)abs(data.SensorValue - settings.Zero));
	
		// Update the AirspeedActual UAVObject
		BaroAirspeedSet(&data);
	}
}

/**
 * @}
 * @}
 */
