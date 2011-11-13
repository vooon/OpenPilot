/* -*- Mode: c; c-basic-offset: 2; tab-width: 2; indent-tabs-mode: t -*- */
/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{ 
 * @addtogroup TextStarModule TextStar Module
 * @brief Interface to TextStar serial LCD module
 * This module displays status on the TextStar serial LCD module, which can
 * be found here:
 *
 *  http://cats-whisker.com/web/node/7
 *
 * This module also supports changing system paramters (PIDs, etc) using the
 * input buttons on the TextStar
 *
 * @{ 
 *
 * @file       textstar.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Telemetry module, handles telemetry and UAVObject updates
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
#include "textstar.h"
#include "systemstats.h"
#include "attitudeactual.h"

// Private constants
#define MAX_QUEUE_SIZE 2
#define STATS_UPDATE_PERIOD_MS 1000
#define STACK_SIZE_BYTES 128
#define TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define PIOS_COM_TEXTSTAR PIOS_COM_TELEM_RF

// Private types
typedef enum { TS_MEMORY, TS_LOAD, TS_ATTITUDE, TS_END } TextStarMode;

// Private variables
static xQueueHandle queue;
static xTaskHandle updateTaskHandle;
static xTaskHandle receiveTaskHandle;
static TextStarMode ts_mode = TS_MEMORY;

// Private functions
static void updateTask(void *parameters);
static void receiveTask(void *parameters);
static void processEvent(UAVObjEvent * ev);
static void sendString(const char* buf);
static void clearScreen();
static void sendFormattedString(const char *format, ...);

/**
 * Initialise the telemetry module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t TextStarStart(void)
{

	// Start the update and receive tasks
	xTaskCreate(updateTask, (signed char *)"TextStarUpdate", STACK_SIZE_BYTES,
							NULL, TASK_PRIORITY, &updateTaskHandle);
	xTaskCreate(receiveTask, (signed char *)"TextStarRX", STACK_SIZE_BYTES,
							NULL, TASK_PRIORITY, &receiveTaskHandle);
	//TaskMonitorAdd(TASKINFO_RUNNING_TELEMETRYTX, telemetryTxTaskHandle);

	return 0;
}

/**
 * Initialise the telemetry module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t TextStarInitialize(void)
{
	UAVObjEvent ev;

	// Create our event queue
	queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(UAVObjEvent));

	// Create periodic event that will be used to update the TextStar
	memset(&ev, 0, sizeof(UAVObjEvent));
	EventPeriodicQueueCreate(&ev, queue, STATS_UPDATE_PERIOD_MS);

	return 0;
}

MODULE_INITCALL(TextStarInitialize, TextStarStart)

/**
 * Send a string to the TextStar
 */
static void sendString(const char* buf) {
	PIOS_COM_SendString(PIOS_COM_TEXTSTAR, buf);
}

/**
 * Clear the TextStar screen.
 */
static void clearScreen() {
	sendString("\x0c");
}

/**
 * Send a string using printf formatting.
 */
static void sendFormattedString(const char *format, ...) {
	char buffer[17]; // There's only 16 characters on a line.
	va_list args;
	va_start(args, format);
	vsprintf((char *)buffer, format, args);
	sendString(buffer);
}

/**
 * Processes queue events
 */
static void processEvent(UAVObjEvent * ev)
{
	static uint8_t loop_cntr = 0;

	// Increment the loop counter and transition to a new display of required.
	if(loop_cntr++ == 5) {
		++ts_mode;
		loop_cntr = 0;
		if(ts_mode == TS_END)
			ts_mode = TS_MEMORY;
	}

	// Clear the screen.
	clearScreen();

	// Get the stats for display.
	switch(ts_mode) {
	case TS_MEMORY:
	case TS_LOAD:
		{
			SystemStatsData stats;
			SystemStatsGet(&stats);
			if(ts_mode == TS_MEMORY) {
				sendString(" Heap  IRQ Stack");
				sendFormattedString("%7d  %-7d", (unsigned int)stats.HeapRemaining,
								(unsigned int)stats.IRQStackRemaining);
			} else {
				sendString("  Load    Temp");
				sendFormattedString("   %-3d    %3d", (unsigned int)stats.CPULoad,
														(unsigned int)stats.CPUTemp);
			}
		}
		break;
	case TS_ATTITUDE:
		{
			AttitudeActualData att;
			AttitudeActualGet(&att);
			sendString("Roll  Pitch  Yaw");
			sendFormattedString("%4d  %4d %4d",
													(unsigned int)(att.Roll * 360 / M_PI),
													(unsigned int)(att.Pitch * 360 / M_PI),
													(unsigned int)(att.Yaw) * 360 / M_PI);
		}
		break;
	case TS_END:
		ts_mode = TS_MEMORY;
		break;
	}
}

/**
 * TextStar update task, regular priority
 */
static void updateTask(void *parameters)
{
	UAVObjEvent ev;

	// Loop forever
	while (1) {
		// Wait for queue message
		if (xQueueReceive(queue, &ev, portMAX_DELAY) == pdTRUE) {
			// Process event
			processEvent(&ev);
		}
	}
}

/**
 * TextStar receive task, regular priority
 */
static void receiveTask(void *parameters)
{
	uint32_t inputPort = PIOS_COM_TEXTSTAR;

	// Task loop
	while (1) {
		// Block until data are available
		uint8_t serial_data[1];
		uint16_t bytes_to_process;

		bytes_to_process = PIOS_COM_ReceiveBuffer(inputPort, serial_data, sizeof(serial_data), 500);
		if (bytes_to_process > 0) {
			char buf[10];
			PIOS_COM_SendString(PIOS_COM_DEBUG, "Received\n\r");
			sprintf(buf, "%c\n\r", serial_data[0]);
			PIOS_COM_SendString(PIOS_COM_DEBUG, buf);
		}
	}
}

/**
  * @}
  * @}
  */
