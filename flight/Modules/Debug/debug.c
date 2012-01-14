/* -*- Mode: c; c-basic-offset: 2; tab-width: 2; indent-tabs-mode: t -*- */
/**
 ******************************************************************************
 *
 * @file       debug.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      The debug module adds a debug queue and a thread to print the
 *             debug queue to the debug serial port.
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

#include "openpilot.h"
#include "hwsettings.h"
#include "debugmod.h"
#include "debugmod_priv.h"

// Private constants
#define MAX_QUEUE_SIZE 2
#define STACK_SIZE configMINIMAL_STACK_SIZE
#define TASK_PRIORITY (tskIDLE_PRIORITY+1)

// Private types

// Private variables
static DebugModData *data = NULL;

// Private functions
static void debugTask(void *parameters);

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t DebugInitialize(void)
{
	bool debugEnabled;
	uint8_t optionalModules[HWSETTINGS_OPTIONALMODULES_NUMELEM];

	HwSettingsInitialize();
	HwSettingsOptionalModulesGet(optionalModules);

	if (optionalModules[HWSETTINGS_OPTIONALMODULES_DEBUG] == HWSETTINGS_OPTIONALMODULES_ENABLED)
		debugEnabled = true;
	else
		debugEnabled = false;
	debugEnabled = true;

	if (debugEnabled)
	{
		// Allocate the module data structure.
		data = pvPortMalloc(sizeof(DebugModData));

		// Create object queue
		data->queue = xQueueCreate(MAX_QUEUE_SIZE, DEBUG_QUEUE_BUF_SIZE);
	}

	return 0;
}

/**
 * Start the module, called on startup
 * \returns 0 on success or -1 if startup failed
 */
int32_t DebugStart(void)
{

	// Start main task if this module is enabled.
	if (data != NULL)
		xTaskCreate(debugTask, (signed char *)"DebugThread", STACK_SIZE, NULL, TASK_PRIORITY, &(data->taskHandle));

	return 0;
}

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
MODULE_INITCALL(DebugInitialize, DebugStart)

/**
 * Insert a string into the debug queue.
 * \returns 0 on success or -1 on failure
 */
int32_t InsertDebugQueue(const char *buf) {
	if(data != NULL)
		return (xQueueSend(data->queue, buf, 0) == pdTRUE) ? 0 : -1;
	else
		return -1;
}

/**
 * Module thread, should not return.
 */
static void debugTask(void *parameters)
{

	// Main task loop
	while (1) {

		// Wait for a message to be put on the queue.
		while (xQueueReceive(data->queue, data->buf, portMAX_DELAY) != pdTRUE) ;

		// Print the debug message to the DEBUG com port.
		PIOS_COM_SendString(PIOS_COM_DEBUG, data->buf);
	}
}
