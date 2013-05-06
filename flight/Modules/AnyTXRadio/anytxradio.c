/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @brief The OpenPilot Modules do the majority of the control in OpenPilot.  The
 * @ref AnyTXRadioModule The AnyTXRadio Module is the equivalanet of the System
 * Module for the PipXtreme modem.  it starts all the other modules.
 #  This is done through the @ref PIOS "PIOS Hardware abstraction layer",
 # which then contains hardware specific implementations
 * (currently only STM32 supported)
 *
 * @{ 
 * @addtogroup AnyTXRadioModule AnyTXRadio Module
 * @brief Initializes PIOS and other modules runs monitoring
 * After initializing all the modules runs basic monitoring and
 * alarms.
 * @{ 
 *
 * @file       anytxradio.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      System module
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

#include <openpilot.h>
#include <pios_board_info.h>
#include "anytxradio.h"
#include "protocol/interface.h"
#include "manualcontrolcommand.h"
#include "accessorydesired.h"
#include "anytxcontrolsettings.h"
#include "flightbatterystate.h"

s16 Channels[NUM_CHANNELS] = {0};
// Private constants
#define SYSTEM_UPDATE_PERIOD_MS 11
#define LED_BLINK_RATE_HZ 5

#if defined(PIOS_SYSTEM_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_SYSTEM_STACK_SIZE
#else
#define STACK_SIZE_BYTES 2400
#endif

#define TASK_PRIORITY (tskIDLE_PRIORITY+2)
#define LONG_TIME 0xffff
xSemaphoreHandle anytxSemaphore = NULL;

// Private variables
static uint32_t idleCounter;
static uint32_t idleCounterClear;
static xTaskHandle systemTaskHandle;
static bool stackOverflow;
static bool mallocFailed;
struct Telemetry Telemetry;

// Private functions
static void anytxradioTask(void *parameters);

/**
 * Create the module task.
 * \returns 0 on success or -1 if initialization failed
 */
int32_t AnyTXRadioStart(void)
{
	// Initialize vars
	stackOverflow = false;
	mallocFailed = false;
	// Create pipxtreme system task
	vSemaphoreCreateBinary(anytxSemaphore);
	xTaskCreate(anytxradioTask, (signed char *)"AnyTXRadio", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &systemTaskHandle);
	// Register task
	TaskMonitorAdd(TASKINFO_RUNNING_ACTUATOR, systemTaskHandle);

	return 0;
}

/**
 * Initialize the module, called on startup.
 * \returns 0 on success or -1 if initialization failed
 */
int32_t AnyTXRadioInitialize(void)
{

	// Call the module start function.
	ManualControlCommandInitialize();
	AccessoryDesiredInitialize();
	AnyTXControlSettingsInitialize();
	FlightBatteryStateInitialize();
	return 0;
}

MODULE_INITCALL(AnyTXRadioInitialize, AnyTXRadioStart)

/**
 * System task, periodically executes every SYSTEM_UPDATE_PERIOD_MS
 */
static void anytxradioTask(__attribute__((unused)) void *parameters)
{
	uint16_t timer;
	ManualControlCommandData cmd;
	AccessoryDesiredData accessory;
	AnyTXControlSettingsData settings;
	FlightBatteryStateData battery;
	//portTickType lastSysTime;

	// Initialize vars
	idleCounter = 0;
	idleCounterClear = 0;
	//lastSysTime = xTaskGetTickCount();

	PIOS_CYRFTMR_Set(10000);
	PIOS_CYRFTMR_Start();
	// Main system loop
	while (1) {
#if 0
		while(1)
		{
			if( xSemaphoreTake(anytxSemaphore, LONG_TIME ) == pdTRUE )
			{
				PIOS_CYRFTMR_Stop();
				timer = 20000;
				PIOS_CYRFTMR_Set(timer);
				PIOS_CYRFTMR_Start();
			}
		}
#endif

		if( xSemaphoreTake(anytxSemaphore, LONG_TIME ) == pdTRUE )
		{
			PIOS_CYRFTMR_Stop();
			// Read settings
			AnyTXControlSettingsGet(&settings);
			ManualControlCommandGet(&cmd);
			FlightBatteryStateGet(&battery);
			if(cmd.Connected == MANUALCONTROLCOMMAND_CONNECTED_TRUE) {
				if(settings.Protocol == ANYTXCONTROLSETTINGS_PROTOCOL_DSM2) {
					Channels[0]=cmd.Throttle * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_THROTTLE]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					Channels[1]=cmd.Roll * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_ROLL]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					Channels[2]=cmd.Pitch * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_PITCH]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					Channels[3]=cmd.Yaw * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_YAW]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );

					if (AccessoryDesiredInstGet(0, &accessory) == 0) {
						Channels[6]=accessory.AccessoryVal * 10000;
					}
					if (AccessoryDesiredInstGet(1, &accessory) == 0) {
						Channels[5]=accessory.AccessoryVal * 10000;
					}
					if (AccessoryDesiredInstGet(2, &accessory) == 0) {
						Channels[4]=accessory.AccessoryVal * 10000;
					}
					timer = dsm2_cb();
				} else if(settings.Protocol == ANYTXCONTROLSETTINGS_PROTOCOL_DEVO) {
					Channels[2]=cmd.Throttle * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_THROTTLE]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					Channels[1]=cmd.Roll * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_ROLL]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					Channels[0]=cmd.Pitch * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_PITCH]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					Channels[3]=cmd.Yaw * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_YAW]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					if (AccessoryDesiredInstGet(0, &accessory) == 0) {
						Channels[6]=accessory.AccessoryVal * 10000;
					}
					if (AccessoryDesiredInstGet(1, &accessory) == 0) {
						Channels[5]=accessory.AccessoryVal * 10000;
					}
					if (AccessoryDesiredInstGet(2, &accessory) == 0) {
						Channels[4]=accessory.AccessoryVal * 10000;
					}
					//timer = devo_cb();
					timer = devo_telemetry_cb();
					battery.Voltage = Telemetry.volt[0]/10.0f;
					battery.Current = Telemetry.volt[1]/10.0f;
					battery.PeakCurrent = Telemetry.volt[2]/10.0f;
					FlightBatteryStateSet(&battery);
				}  else if(settings.Protocol == ANYTXCONTROLSETTINGS_PROTOCOL_WK2401 || settings.Protocol == ANYTXCONTROLSETTINGS_PROTOCOL_WK2601 || settings.Protocol == ANYTXCONTROLSETTINGS_PROTOCOL_WK2801) {
					Channels[2]=cmd.Throttle * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_THROTTLE]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					Channels[1]=cmd.Roll * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_ROLL]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					Channels[0]=cmd.Pitch * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_PITCH]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					Channels[3]=cmd.Yaw * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_YAW]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					if (AccessoryDesiredInstGet(0, &accessory) == 0) {
						Channels[6]=accessory.AccessoryVal * 10000;
					}
					if (AccessoryDesiredInstGet(1, &accessory) == 0) {
						Channels[5]=accessory.AccessoryVal * 10000;
					}
					if (AccessoryDesiredInstGet(2, &accessory) == 0) {
						Channels[4]=accessory.AccessoryVal * 10000;
					}
					timer = wk_cb();
				}else if(settings.Protocol == ANYTXCONTROLSETTINGS_PROTOCOL_J6PRO) {
					Channels[2]=cmd.Throttle * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_THROTTLE]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					Channels[1]=cmd.Roll * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_ROLL]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					Channels[0]=cmd.Pitch * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_PITCH]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					Channels[3]=cmd.Yaw * 10000 *
							(settings.OutputDirectionInvert[ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_YAW]==ANYTXCONTROLSETTINGS_OUTPUTDIRECTIONINVERT_TRUE ? -1 : 1 );
					if (AccessoryDesiredInstGet(0, &accessory) == 0) {
						Channels[6]=accessory.AccessoryVal * 10000;
					}
					if (AccessoryDesiredInstGet(1, &accessory) == 0) {
						Channels[5]=accessory.AccessoryVal * 10000;
					}
					if (AccessoryDesiredInstGet(2, &accessory) == 0) {
						Channels[4]=accessory.AccessoryVal * 10000;
					}
					timer = j6pro_cb();
				}

				else {
					timer = 10000;
				}
			} else {
				timer = 10000;
			}
			PIOS_CYRFTMR_Set(timer);
			PIOS_CYRFTMR_Start();
		}
		//vTaskDelayUntil(&lastSysTime, timer/1000 / portTICK_RATE_MS);
	}
}

/**
  * @}
  * @}
  */
