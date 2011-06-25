/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{ 
 * @addtogroup SonarModule Sonar Module
 * @brief Communicate with HCSR04 and update @ref SonarAltitude "SonarAltitude UAV Object"
 * @{ 
 *
 * @file       sonar.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Sonar module, reads HCSR04
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
 * Output object: SonarAltitude
 *
 * This module will periodically update the value of the SonarAltitude object.
 *
 */

#include "openpilot.h"
#if defined(PIOS_INCLUDE_HCSR04)
#include "sonaraltitude.h"	// object that will be updated by the module
#endif

// Private constants
#define STACK_SIZE_BYTES 186
#define TASK_PRIORITY (tskIDLE_PRIORITY+1)
//#define UPDATE_PERIOD 100
#define UPDATE_PERIOD 25

// Private types

// Private variables
static xTaskHandle taskHandle;

        // Private functions
static void sonarTask(void *parameters);

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t SonarInitialize()
{
	// Start main task
	xTaskCreate(sonarTask, (signed char *)"Altitude", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &taskHandle);
	TaskMonitorAdd(TASKINFO_RUNNING_ALTITUDE, taskHandle);

	// init down-sampling data

	return 0;
}

/**
 * Module thread, should not return.
 */
static void sonarTask(void *parameters)
{
	portTickType lastSysTime;

	SonarAltitudeData sonardata;
	uint16_t value=0;
	uint8_t timeout=5;
#define COEFF 0.25
	float height_out=0,height_in=0;
	PIOS_HCSR04_Init();
	PIOS_HCSR04_Trigger();

	// Main task loop
	lastSysTime = xTaskGetTickCount();
	while (1)
	{
		// Compute the current altitude
		if(PIOS_HCSR04_Completed())
		{
			value = PIOS_HCSR04_Get();
			if((value>200) && (value < 30000)) //from 3.4cm to 5.1m
			{
				height_in = value*340/2.0/1000000;
				height_out = (height_out * (1 - COEFF)) + (height_in * COEFF);
				sonardata.Altitude = height_out; // m/us
			}

			// Update the SonarAltitude UAVObject
			SonarAltitudeSet(&sonardata);
			timeout=5;
			PIOS_HCSR04_Trigger();
		}
		if(!(timeout--))
		{
			//retrigger
			timeout=5;
			PIOS_HCSR04_Trigger();
		}
		// Delay until it is time to read the next sample
		vTaskDelayUntil(&lastSysTime, UPDATE_PERIOD / portTICK_RATE_MS);
	}
}

/**
  * @}
 * @}
 */
