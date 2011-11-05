/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup GSPModule GPS Module
 * @brief Process GPS information
 * @{
 *
 * @file       GPS.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      GPS module, handles GPS and NMEA stream
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

// ****************

#include "openpilot.h"
#include "osdgen.h"
#include "attitudeactual.h"


// ****************
// Private functions

static void osdgenTask(void *parameters);

// ****************
// Private constants

#define STACK_SIZE_BYTES            400

#define TASK_PRIORITY               (tskIDLE_PRIORITY + 4)
#define UPDATE_PERIOD 100

// ****************
// Private variables

static xTaskHandle osdgenTaskHandle;



// ****************
/**
 * Initialise the gps module
 * \return -1 if initialisation failed
 * \return 0 on success
 */

int32_t osdgenStart(void)
{
	// Start gps task
	xTaskCreate(osdgenTask, (signed char *)"OSDGEN", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &osdgenTaskHandle);
	TaskMonitorAdd(TASKINFO_RUNNING_GPS, osdgenTaskHandle);

	return 0;
}
/**
 * Initialise the gps module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t osdgenInitialize(void)
{
	AttitudeActualInitialize();
	return 0;
}
MODULE_INITCALL(osdgenInitialize, osdgenStart)

// ****************
/**
 * Main gps task. It does not return.
 */

/*static void updateOnceEveryFrame() {
	AttitudeActualData attitude;
	AttitudeActualGet(&attitude);
	setAttitudeOsd((int16_t)attitude.Pitch,(int16_t)attitude.Roll);
	clearText();
	for (uint8_t i = 0; i < TEXT_LINES; ++i) {
	  updateText(i);
	}
	clearGraphics();
	updateGrapics();
}*/

static void osdgenTask(void *parameters)
{
	portTickType lastSysTime;
	// Loop forever
	lastSysTime = xTaskGetTickCount();
	while (1)
	{
		AttitudeActualData attitude;
		AttitudeActualGet(&attitude);
		setAttitudeOsd((int16_t)attitude.Pitch,(int16_t)attitude.Roll);
		if (gUpdateScreenData == 1) {
			gUpdateScreenData = 0;
			updateOnceEveryFrame();
		}
		while(gUpdateScreenData==0)
			vTaskDelayUntil(&lastSysTime, 1 / portTICK_RATE_MS);
	}
}


// ****************

/**
  * @}
  * @}
  */
