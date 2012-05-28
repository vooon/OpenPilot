/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{ 
 * @addtogroup ???
 * @brief Communicate with generic I2C sensor and update @ref GenericI2C "GenericI2C UAV Object"
 * @{ 
 *
 * @file       generici2csensor.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Reads generic i2c sensor
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
 * Output object: generici2csensor
 *
 * This module will periodically update the value of the mcp3424sensor UAVobject.
 *
 */

#include "openpilot.h"
#include "hwsettings.h"
#include "generici2csensormodule.h"
#include "generici2csensor.h" //UAV Object
#include "generici2csensorsettings.h" //UAV Object
#include "pios_i2c.h"

// Private constants
#define STACK_SIZE_BYTES 370
#define TASK_PRIORITY (tskIDLE_PRIORITY+1)

// Private variables
static xTaskHandle taskHandle;
static bool genericI2CSensorModuleEnabled;

// Private functions
static void GenericI2CSensorTask(void *parameters);

/**
* Start the module, called on startup
*/
int32_t GenericI2CSensorModuleStart()
{
	if (genericI2CSensorModuleEnabled) {
		GenericI2CSensorSettingsInitialize();
		GenericI2CSensorInitialize();

		// Start main task
		xTaskCreate(GenericI2CSensorTask, (signed char *)"GenericI2CSensor", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &taskHandle);
		TaskMonitorAdd(TASKINFO_RUNNING_GENERICI2CSENSOR, taskHandle);
		return 0;
	}
	return -1;
}

/**
* Initialise the module, called on startup
*/
int32_t GenericI2CSensorModuleInitialize()
{
#ifdef MODULE_GenericI2CSensorModule_BUILTIN
	genericI2CSensorModuleEnabled = 1;
#else
	HwSettingsInitialize();
	uint8_t optionalModules[HWSETTINGS_OPTIONALMODULES_NUMELEM];
	HwSettingsOptionalModulesGet(optionalModules);
	if (optionalModules[HWSETTINGS_OPTIONALMODULES_GENERICI2CSENSOR] == HWSETTINGS_OPTIONALMODULES_ENABLED) {
		genericI2CSensorModuleEnabled = 1;
	} else {
		genericI2CSensorModuleEnabled = 0;
	}
#endif
	return 0;
}

MODULE_INITCALL(GenericI2CSensorModuleInitialize, GenericI2CSensorModuleStart)


static void GenericI2CSensorTask(void *parameters)
{
	// Main task loop
	while (1) {
		gen_i2c_vm_run();
	}
}

/**
  * @}
 * @}
 */
