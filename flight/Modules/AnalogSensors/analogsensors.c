/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup ET_EGT_Sensor EagleTree EGT Sensor Module
 * @brief Read ET EGT temperature sensors @ref ETEGTSensor "ETEGTSensor UAV Object"
 * @{
 *
 * @file       et_egt_sensor.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Reads dual thermocouple temperature sensors via EagleTree EGT expander
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
 * Output object: GasEngine
 *
 * This module will periodically update the value of the GasEngine UAVobject.
 *
 */

#include "openpilot.h"
#include "analogsensors.h"
#include "mcp3424.h"
#include "mcp9804.h"
#include "analogsensorsvalues.h"	// UAVobject that will be updated by the module
#include "analogsensorssettings.h" // UAVobject used to modify module settings
//#include "pios_i2c.h"

// Private constants
#define STACK_SIZE_BYTES 330
#define TASK_PRIORITY (tskIDLE_PRIORITY+1)
#define UPDATE_PERIOD 500

//static double_t DegCPerVolt = 24813.8957; //volts per celcius for K-type thermocouple
// Private types

// Private variables
static xTaskHandle taskHandle;

// Private functions
static void AnalogSensorsTask(void *parameters);

/**
* Start the module, called on startup
*/
int32_t AnalogSensorsStart()
{
	// Start main task
	xTaskCreate(AnalogSensorsTask, (signed char *)"AnalogSensors", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &taskHandle);
	TaskMonitorAdd(TASKINFO_RUNNING_ANALOGSENSORS, taskHandle);
	return 0;
}

/**
* Initialise the module, called on startup
*/
int32_t AnalogSensorsInitialize()
{
	AnalogSensorsValuesInitialize(); //Initialise the UAVObject used for transferring sensor readings to GCS
	AnalogSensorsSettingsInitialize(); //Initialise the UAVObject used for changing sensor settings

	return 0;
}

MODULE_INITCALL(AnalogSensorsInitialize, AnalogSensorsStart)

/**
 * Module thread, should not return.
 */
static void AnalogSensorsTask(void *parameters)
{
	uint8_t buf[8] = {0};
	buf[0] = 5;

	uint16_t I2CAddress = 0x00;
	uint8_t gain, resolution = 0;
	double_t analogValue = 0.0;

	//Assume Attopilot voltage and current sensor is being used.
	// Specifically, the full scale voltage is 51.8V = 3.3V
	// Full scale current is 90A = 3.3V
	double_t attoPilotVscale = 1 / 0.06369; //From data sheet
	double_t attoPilotIscale = 1 / 0.0366; //From data sheet

	portTickType lastSysTime;

	//UAVObject data structure
	AnalogSensorsValuesData d1;

	//UAVObject settings data
	AnalogSensorsSettingsData s1;

	// Main task loop
	lastSysTime = xTaskGetTickCount();

	while(1) {

		//get any updated settings
		AnalogSensorsSettingsGet(&s1);
		I2CAddress = s1.I2CAddress;

		/******************
		 * Read channel 1
		 ******************/
		gain = MCP3424_GetGain(s1.Channel1Gain);
		resolution = MCP3424_GetResolution(s1.Channel1Resolution);

		if (MCP3424_GetAnalogValue(I2CAddress, 1, buf, resolution, gain, &analogValue))
		{
			//TODO: switch conversion factor based on channel input option
			d1.Sensor1 = analogValue * attoPilotVscale;
		}
		else
		{
			d1.Sensor1 = -99;
		}

		/*******************
		 * Read channel 2
		 ********************/
		gain = MCP3424_GetGain(s1.Channel2Gain);
		resolution = MCP3424_GetResolution(s1.Channel2Resolution);

		//Read thermocouple connected to channel 1 of MCP3424 IC via I2C
		if (MCP3424_GetAnalogValue(I2CAddress, 2, buf, resolution, gain, &analogValue))
		{
			d1.Sensor2 = analogValue * attoPilotIscale;
		}
		else
			d1.Sensor2 = -99;

		/******************
		 * Read channel 3
		 *******************/
		gain = MCP3424_GetGain(s1.Channel3Gain);
		resolution = MCP3424_GetResolution(s1.Channel3Resolution);

		if (MCP3424_GetAnalogValue(I2CAddress, 3, buf, resolution, gain, &analogValue))
		{
			d1.Sensor3 = analogValue * attoPilotVscale;
		}
		else
			d1.Sensor3 = -99;

		/***************
		 * Read channel 4
		 ***************/
		gain = MCP3424_GetGain(s1.Channel4Gain);
		resolution = MCP3424_GetResolution(s1.Channel4Resolution);

		if (MCP3424_GetAnalogValue(I2CAddress, 4, buf, resolution, gain, &analogValue))
		{
			d1.Sensor4 = analogValue * attoPilotIscale;
		}
		else
			d1.Sensor4 = -99;

		/*
		 * Update UAVObject data
		 */
		AnalogSensorsValuesSet(&d1);

		// Delay until it is time to read the next sample
		vTaskDelayUntil(&lastSysTime, UPDATE_PERIOD / portTICK_RATE_MS);
	}
}


/**
  * @}
 * @}
 */
