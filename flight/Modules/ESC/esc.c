/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup ESC ESC Configuration and monitoring interface
 * @brief Communicates with ESCs for configuration and monitoring
 * @{
 *
 * @file       esc.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Module to handle all comms to the AHRS on a periodic basis.
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************/
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
 * Input objects: @ref EscSettings
 * Output objects: @ref EscStatus, multiple instances
 *
 * This updates the configuration on the ESCs when it changes and monitors
 * their status
 *
 * The module executes in its own thread.
 *
 * UAVObjects are automatically generated by the UAVObjectGenerator from
 * the object definition XML file.
 *
 * Modules have no API, all communication to other modules is done through UAVObjects.
 * However modules may use the API exposed by shared libraries.
 * See the OpenPilot wiki for more details.
 * http://www.openpilot.org/OpenPilot_Application_Architecture
 *
 */

#include "pios.h"
#include "esc.h"
#include "escsettings.h"
#include "escstatus.h"

// Private constants
#define STACK_SIZE_BYTES 540
#define TASK_PRIORITY (tskIDLE_PRIORITY+3)
// Private types

#define ESC_SYNC_BYTE 0x85
//! The set of commands the ESC understands
enum esc_serial_command {
	ESC_COMMAND_SET_CONFIG = 0,
	ESC_COMMAND_GET_CONFIG = 1,
	ESC_COMMAND_SAVE_CONFIG = 2,
	ESC_COMMAND_ENABLE_SERIAL_LOGGING = 3,
	ESC_COMMAND_DISABLE_SERIAL_LOGGING = 4,
	ESC_COMMAND_REBOOT_BL = 5,
	ESC_COMMAND_ENABLE_SERIAL_CONTROL = 6,
	ESC_COMMAND_DISABLE_SERIAL_CONTROL = 7,
	ESC_COMMAND_SET_SPEED = 8,
	ESC_COMMAND_WHOAMI = 9,
	ESC_COMMAND_ENABLE_ADC_LOGGING = 10,
	ESC_COMMAND_GET_ADC_LOG = 11,
	ESC_COMMAND_SET_PWM_FREQ = 12,
	ESC_COMMAND_GET_STATUS = 13,
	ESC_COMMAND_LAST = 14,
};


// Private variables
static xTaskHandle taskHandle;

// Private functions
static void EscTask(void *parameters);

// Functions for communicating
static int32_t escSendSettings(uint8_t id);
static int32_t escGetStatus(uint8_t id);

// Communication method specific sending/get functions
static int32_t escSendMessageSerial(uint8_t id, enum esc_serial_command, uint8_t * optional);
static int32_t escSendQuerySerial(uint8_t id, enum esc_serial_command command, uint8_t * optional);

static void settingsUpdatedCb(UAVObjEvent * objEv);

// External global variables
extern uint32_t pios_com_esc_id;

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t EscStart(void)
{

	// Start main task
	xTaskCreate(EscTask, (signed char *)"ESC", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &taskHandle);

	return 0;
}

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t EscInitialize(void)
{
	EscSettingsInitialize();
	EscStatusInitialize();

	EscSettingsConnectCallback(&settingsUpdatedCb);

	return 0;
}

MODULE_INITCALL(EscInitialize, EscStart)

/**
 * Module thread, should not return.
 */
static void EscTask(void *parameters)
{
	// Force settings update to make sure rotation loaded
	settingsUpdatedCb(EscSettingsHandle());

	// Main task loop
	while (1) {
		escGetStatus(0);
		vTaskDelay(100);
	}
}

/**
 * When the settings are updated, send them to each of the ESCs
 */
static void settingsUpdatedCb(UAVObjEvent * objEv) {
	escSendSettings(0);
}

/**
 * Send settings to a particular ESC id
 */
static int32_t escSendSettings(uint8_t id)
{
	EscSettingsData escSettings;
	EscSettingsGet(&escSettings);

	// TODO: Case statement for the communication method
	if(escSendMessageSerial(0, ESC_COMMAND_SET_CONFIG, (uint8_t *) &escSettings) < 0)
		return -1;

	return 0;
}

/**
 * Gets the settings from an ESC id and sets that UAVO instance
 */
static int32_t escGetStatus(uint8_t id)
{
	EscStatusData escStatus;
	EscStatusInstGet(id, &escStatus);

	// TODO: Case statement for the communication method
	if(escSendQuerySerial(id, ESC_COMMAND_GET_STATUS, (uint8_t *) &escStatus) < 0)
		return -1;

	EscStatusInstSet(id, &escStatus);
	return 0;
}

/**
 * Sends a message to an esc via serial port
 * @param[in] id the ESC device to send message to
 * @param[in] command what command to send to the ESC
 * @param[in] optional pointer to optional data that can be sent
 * @returns -1 for failure or zero for success.
 */
static int32_t escSendMessageSerial(uint8_t id, enum esc_serial_command command, uint8_t * optional)
{
	//TODO: Check for permitted message types
	uint32_t message_size = 0;
	switch(command) {
		case ESC_COMMAND_SET_CONFIG:
			message_size = sizeof(EscSettingsData);
			break;
		default:
			// Any types without an explicit handling are errors
			return -1;
			break;
	}

	// It would be nice to check whole message will fit in send buffer but API doesn't have
	// a function to query free sending space
	uint8_t preamble[2] = {ESC_SYNC_BYTE, command};
	if(PIOS_COM_SendBuffer(pios_com_esc_id, preamble, 2) < 0)
		return -2;

	if (message_size != 0) {
		if(PIOS_COM_SendBuffer(pios_com_esc_id, optional, message_size) < 0)
			return -3;
	}
	
	return 0;
}

/**
 * Sends a query to ESC and returns response via serial port
 * @param[in] id the ESC device to send message to
 * @param[in] command what command to send to the ESC
 * @param[in] optional pointer to optional data that can be sent
 * @returns -1 for failure or zero for success.
 */
static int32_t escSendQuerySerial(uint8_t id, enum esc_serial_command command, uint8_t * optional)
{
	uint32_t message_size = 0;
	switch(command) {
		case ESC_COMMAND_SET_CONFIG:
			message_size = sizeof(EscSettingsData);
			break;
		default:
			return -1;
			break;
	}

	uint8_t preamble[2] = {ESC_SYNC_BYTE, command};
	if(PIOS_COM_SendBuffer(pios_com_esc_id, preamble, 2) < 0)
		return -2;

	if (message_size != 0) {
		if(PIOS_COM_SendBuffer(pios_com_esc_id, optional, message_size) < 0)
			return -3;
	}
	
	return 0;
}

/**
  * @}
  * @}
  */
