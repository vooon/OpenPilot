/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup ManualControlModule Manual Control Module
 * @brief Provide manual control or allow it alter flight mode.
 * @{
 *
 * Reads in the ManualControlCommand FlightMode setting from receiver then either
 * pass the settings straght to ActuatorDesired object (manual mode) or to
 * AttitudeDesired object (stabilized mode)
 *
 * @file       manualcontrol.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      ManualControl module. Handles safety R/C link and flight mode.
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
#include "manualcontrol.h"
#include "manualcontrolsettings.h"
#include "stabilizationsettings.h"
#include "manualcontrolcommand.h"
#include "actuatordesired.h"
#include "stabilizationdesired.h"
#include "flighttelemetrystats.h"
#include "flightstatus.h"
#include "accessorydesired.h"

// Private constants
#if defined(PIOS_MANUAL_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_MANUAL_STACK_SIZE
#else
#define STACK_SIZE_BYTES 824
#endif

#define TASK_PRIORITY (tskIDLE_PRIORITY+4)
#define UPDATE_PERIOD_MS 20
#define THROTTLE_FAILSAFE -0.1
#define FLIGHT_MODE_LIMIT 1.0/3.0
#define ARMED_TIME_MS      1000
#define ARMED_THRESHOLD    0.50
//safe band to allow a bit of calibration error or trim offset (in microseconds)
#define CONNECTION_OFFSET 150

// Private types
typedef enum
{
	ARM_STATE_DISARMED,
	ARM_STATE_ARMING_MANUAL,
	ARM_STATE_ARMED,
	ARM_STATE_DISARMING_MANUAL,
	ARM_STATE_DISARMING_TIMEOUT
} ArmState_t;

// Private variables
static xTaskHandle taskHandle;
static ArmState_t armState;
static portTickType lastSysTime;

// Private functions
static void updateActuatorDesired(ManualControlCommandData * cmd);
static void updateStabilizationDesired(ManualControlCommandData * cmd, ManualControlSettingsData * settings);
static void processFlightMode(ManualControlSettingsData * settings, float flightMode);
static void processArm(ManualControlCommandData * cmd, ManualControlSettingsData * settings);

static void manualControlTask(void *parameters);
static float scaleChannel(int16_t value, int16_t max, int16_t min, int16_t neutral);
static uint32_t timeDifferenceMs(portTickType start_time, portTickType end_time);
static bool okToArm(void);
static bool validInputRange(int16_t min, int16_t max, uint16_t value);

#define assumptions (assumptions1 && assumptions3 && assumptions5 && assumptions7 && assumptions8 && assumptions_flightmode)

/**
 * Module starting
 */
int32_t ManualControlStart()
{
	// Start main task
	xTaskCreate(manualControlTask, (signed char *)"ManualControl", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &taskHandle);
	TaskMonitorAdd(TASKINFO_RUNNING_MANUALCONTROL, taskHandle);
	PIOS_WDG_RegisterFlag(PIOS_WDG_MANUAL);

	return 0;
}

/**
 * Module initialization
 */
int32_t ManualControlInitialize()
{

	/* Check the assumptions about uavobject enum's are correct */
	if(!assumptions)
		return -1;

	AccessoryDesiredInitialize();
	ManualControlCommandInitialize();
	FlightStatusInitialize();
	StabilizationDesiredInitialize();

	ManualControlSettingsInitialize();

	return 0;
}
MODULE_INITCALL(ManualControlInitialize, ManualControlStart)

/**
 * Module task
 */
static void manualControlTask(void *parameters)
{
	ManualControlSettingsData settings;
	ManualControlCommandData cmd;
	FlightStatusData flightStatus;
	float flightMode = 0;

	uint8_t disconnected_count = 0;
	uint8_t connected_count = 0;

	// For now manual instantiate extra instances of Accessory Desired.  In future should be done dynamically
	// this includes not even registering it if not used
	AccessoryDesiredCreateInstance();
	AccessoryDesiredCreateInstance();

	// Make sure unarmed on power up
	ManualControlCommandGet(&cmd);
	FlightStatusGet(&flightStatus);
	flightStatus.Armed = FLIGHTSTATUS_ARMED_DISARMED;
	armState = ARM_STATE_DISARMED;

	// Main task loop
	lastSysTime = xTaskGetTickCount();
	while (1) {
		float scaledChannel[MANUALCONTROLCOMMAND_CHANNEL_NUMELEM];

		// Wait until next update
		vTaskDelayUntil(&lastSysTime, UPDATE_PERIOD_MS / portTICK_RATE_MS);
		PIOS_WDG_UpdateFlag(PIOS_WDG_MANUAL);

		// Read settings
		ManualControlSettingsGet(&settings);

		if (ManualControlCommandReadOnly(&cmd)) {
			FlightTelemetryStatsData flightTelemStats;
			FlightTelemetryStatsGet(&flightTelemStats);
			if(flightTelemStats.Status != FLIGHTTELEMETRYSTATS_STATUS_CONNECTED) {
				/* trying to fly via GCS and lost connection.  fall back to transmitter */
				UAVObjMetadata metadata;
				UAVObjGetMetadata(&cmd, &metadata);
				metadata.access = ACCESS_READWRITE;
				UAVObjSetMetadata(&cmd, &metadata);
			}
		}

		if (!ManualControlCommandReadOnly(&cmd)) {

			// Read channel values in us
			for (int n = 0; n < MANUALCONTROLCOMMAND_CHANNEL_NUMELEM; ++n) {
				if (pios_rcvr_channel_to_id_map[n].id) {
					cmd.Channel[n] = PIOS_RCVR_Read(pios_rcvr_channel_to_id_map[n].id,
									pios_rcvr_channel_to_id_map[n].channel);
				} else {
					cmd.Channel[n] = -1;
				}
				scaledChannel[n] = scaleChannel(cmd.Channel[n], settings.ChannelMax[n],	settings.ChannelMin[n], settings.ChannelNeutral[n]);
			}

			// Check settings, if error raise alarm
			if (settings.Roll >= MANUALCONTROLSETTINGS_ROLL_NONE ||
			    settings.Pitch >= MANUALCONTROLSETTINGS_PITCH_NONE ||
			    settings.Yaw >= MANUALCONTROLSETTINGS_YAW_NONE ||
			    settings.Throttle >= MANUALCONTROLSETTINGS_THROTTLE_NONE ||
			    settings.FlightMode >= MANUALCONTROLSETTINGS_FLIGHTMODE_NONE) {
				AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_CRITICAL);
				cmd.Connected = MANUALCONTROLCOMMAND_CONNECTED_FALSE;
				ManualControlCommandSet(&cmd);
				continue;
			}

			// decide if we have valid manual input or not
			bool valid_input_detected = validInputRange(settings.ChannelMin[settings.Throttle], settings.ChannelMax[settings.Throttle], cmd.Channel[settings.Throttle]) &&
			     validInputRange(settings.ChannelMin[settings.Roll], settings.ChannelMax[settings.Roll], cmd.Channel[settings.Roll]) &&
			     validInputRange(settings.ChannelMin[settings.Yaw], settings.ChannelMax[settings.Yaw], cmd.Channel[settings.Yaw]) &&
			     validInputRange(settings.ChannelMin[settings.Pitch], settings.ChannelMax[settings.Pitch], cmd.Channel[settings.Pitch]);

			// Implement hysteresis loop on connection status
			if (valid_input_detected && (++connected_count > 10)) {
				cmd.Connected = MANUALCONTROLCOMMAND_CONNECTED_TRUE;
				connected_count = 0;
				disconnected_count = 0;
			} else if (!valid_input_detected && (++disconnected_count > 10)) {
				cmd.Connected = MANUALCONTROLCOMMAND_CONNECTED_FALSE;
				connected_count = 0;
				disconnected_count = 0;
			}

			if (cmd.Connected == MANUALCONTROLCOMMAND_CONNECTED_FALSE) {
				cmd.Throttle = -1;	// Shut down engine with no control
				cmd.Roll = 0;
				cmd.Yaw = 0;
				cmd.Pitch = 0;

				// This will follow the low throttle path to disarming. Needed for recievers that disconnect when TX is off. Once MANUALCONTROLCOMMAND_FLIGHTMODE_AUTO is implemented this will probably have to change a bit
				processArm(&cmd, &settings);

				//cmd.FlightMode = MANUALCONTROLCOMMAND_FLIGHTMODE_AUTO; // don't do until AUTO implemented and functioning
				// Important: Throttle < 0 will reset Stabilization coefficients among other things. Either change this,
				// or leave throttle at IDLE speed or above when going into AUTO-failsafe.
				AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_WARNING);
				ManualControlCommandSet(&cmd);
			} else {
				AlarmsClear(SYSTEMALARMS_ALARM_MANUALCONTROL);

				// Scale channels to -1 -> +1 range
				cmd.Roll           = scaledChannel[settings.Roll];
				cmd.Pitch          = scaledChannel[settings.Pitch];
				cmd.Yaw            = scaledChannel[settings.Yaw];
				cmd.Throttle       = scaledChannel[settings.Throttle];
				flightMode         = scaledChannel[settings.FlightMode];

				AccessoryDesiredData accessory;
				// Set Accessory 0
				if(settings.Accessory0 != MANUALCONTROLSETTINGS_ACCESSORY0_NONE) {
					accessory.AccessoryVal = scaledChannel[settings.Accessory0];
					if(AccessoryDesiredInstSet(0, &accessory) != 0)
						AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_WARNING);
				}
				// Set Accessory 1
				if(settings.Accessory1 != MANUALCONTROLSETTINGS_ACCESSORY1_NONE) {
					accessory.AccessoryVal = scaledChannel[settings.Accessory1];
					if(AccessoryDesiredInstSet(1, &accessory) != 0)
						AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_WARNING);
				}
				// Set Accsesory 2
				if(settings.Accessory2 != MANUALCONTROLSETTINGS_ACCESSORY2_NONE) {
					accessory.AccessoryVal = scaledChannel[settings.Accessory2];
					if(AccessoryDesiredInstSet(2, &accessory) != 0)
						AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_WARNING);
				}


				processFlightMode(&settings, flightMode);
				processArm(&cmd, &settings);

				// Update cmd object
				ManualControlCommandSet(&cmd);

			}

		} else {
			ManualControlCommandGet(&cmd);	/* Under GCS control */
		}


		FlightStatusGet(&flightStatus);

		// Depending on the mode update the Stabilization or Actuator objects
		switch(PARSE_FLIGHT_MODE(flightStatus.FlightMode)) {
			case FLIGHTMODE_UNDEFINED:
				// This reflects a bug in the code architecture!
				AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_CRITICAL);
				break;
			case FLIGHTMODE_MANUAL:
				updateActuatorDesired(&cmd);
				break;
			case FLIGHTMODE_STABILIZED:
				updateStabilizationDesired(&cmd, &settings);
				break;
			case FLIGHTMODE_GUIDANCE:
				// TODO: Implement
				break;
		}
	}
}

static void updateActuatorDesired(ManualControlCommandData * cmd)
{
	ActuatorDesiredData actuator;
	ActuatorDesiredGet(&actuator);
	actuator.Roll = cmd->Roll;
	actuator.Pitch = cmd->Pitch;
	actuator.Yaw = cmd->Yaw;
	actuator.Throttle = (cmd->Throttle < 0) ? -1 : cmd->Throttle;
	ActuatorDesiredSet(&actuator);
}

static void updateStabilizationDesired(ManualControlCommandData * cmd, ManualControlSettingsData * settings)
{
	StabilizationDesiredData stabilization;
	StabilizationDesiredGet(&stabilization);

	StabilizationSettingsData stabSettings;
	StabilizationSettingsGet(&stabSettings);

	uint8_t * stab_settings;
	FlightStatusData flightStatus;
	FlightStatusGet(&flightStatus);
	switch(flightStatus.FlightMode) {
		case FLIGHTSTATUS_FLIGHTMODE_STABILIZED1:
			stab_settings = settings->Stabilization1Settings;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_STABILIZED2:
			stab_settings = settings->Stabilization2Settings;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_STABILIZED3:
			stab_settings = settings->Stabilization3Settings;
			break;
		default:
			// Major error, this should not occur because only enter this block when one of these is true
			AlarmsSet(SYSTEMALARMS_ALARM_MANUALCONTROL, SYSTEMALARMS_ALARM_CRITICAL);
			return;
	}

	// TOOD: Add assumption about order of stabilization desired and manual control stabilization mode fields having same order
	stabilization.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_ROLL]  = stab_settings[0];
	stabilization.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_PITCH] = stab_settings[1];
	stabilization.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_YAW]   = stab_settings[2];

	stabilization.Roll = (stab_settings[0] == STABILIZATIONDESIRED_STABILIZATIONMODE_NONE) ? cmd->Roll :
	     (stab_settings[0] == STABILIZATIONDESIRED_STABILIZATIONMODE_RATE) ? cmd->Roll * stabSettings.ManualRate[STABILIZATIONSETTINGS_MANUALRATE_ROLL] :
	     (stab_settings[0] == STABILIZATIONDESIRED_STABILIZATIONMODE_WEAKLEVELING) ? cmd->Roll * stabSettings.ManualRate[STABILIZATIONSETTINGS_MANUALRATE_ROLL] :
	     (stab_settings[0] == STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE) ? cmd->Roll * stabSettings.RollMax :
	     (stab_settings[0] == STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK) ? cmd->Roll * stabSettings.ManualRate[STABILIZATIONSETTINGS_MANUALRATE_ROLL] :
	     0; // this is an invalid mode
					      ;
	stabilization.Pitch = (stab_settings[1] == STABILIZATIONDESIRED_STABILIZATIONMODE_NONE) ? cmd->Pitch :
	     (stab_settings[1] == STABILIZATIONDESIRED_STABILIZATIONMODE_RATE) ? cmd->Pitch * stabSettings.ManualRate[STABILIZATIONSETTINGS_MANUALRATE_PITCH] :
	     (stab_settings[1] == STABILIZATIONDESIRED_STABILIZATIONMODE_WEAKLEVELING) ? cmd->Pitch * stabSettings.ManualRate[STABILIZATIONSETTINGS_MANUALRATE_PITCH] :
	     (stab_settings[1] == STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE) ? cmd->Pitch * stabSettings.PitchMax :
	     (stab_settings[1] == STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK) ? cmd->Pitch * stabSettings.ManualRate[STABILIZATIONSETTINGS_MANUALRATE_PITCH] :
	     0; // this is an invalid mode

	stabilization.Yaw = (stab_settings[2] == STABILIZATIONDESIRED_STABILIZATIONMODE_NONE) ? cmd->Yaw :
	     (stab_settings[2] == STABILIZATIONDESIRED_STABILIZATIONMODE_RATE) ? cmd->Yaw * stabSettings.ManualRate[STABILIZATIONSETTINGS_MANUALRATE_YAW] :
	     (stab_settings[2] == STABILIZATIONDESIRED_STABILIZATIONMODE_WEAKLEVELING) ? cmd->Yaw * stabSettings.ManualRate[STABILIZATIONSETTINGS_MANUALRATE_YAW] :
	     (stab_settings[2] == STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE) ? cmd->Yaw * stabSettings.YawMax :
	     (stab_settings[2] == STABILIZATIONDESIRED_STABILIZATIONMODE_AXISLOCK) ? cmd->Yaw * stabSettings.ManualRate[STABILIZATIONSETTINGS_MANUALRATE_YAW] :
	     0; // this is an invalid mode

	stabilization.Throttle = (cmd->Throttle < 0) ? -1 : cmd->Throttle;
	StabilizationDesiredSet(&stabilization);
}

/**
 * Convert channel from servo pulse duration (microseconds) to scaled -1/+1 range.
 */
static float scaleChannel(int16_t value, int16_t max, int16_t min, int16_t neutral)
{
	float valueScaled;

	// Scale
	if ((max > min && value >= neutral) || (min > max && value <= neutral))
	{
		if (max != neutral)
			valueScaled = (float)(value - neutral) / (float)(max - neutral);
		else
			valueScaled = 0;
	}
	else
	{
		if (min != neutral)
			valueScaled = (float)(value - neutral) / (float)(neutral - min);
		else
			valueScaled = 0;
	}

	// Bound
	if (valueScaled >  1.0) valueScaled =  1.0;
	else
	if (valueScaled < -1.0) valueScaled = -1.0;

	return valueScaled;
}

static uint32_t timeDifferenceMs(portTickType start_time, portTickType end_time) {
	if(end_time > start_time)
		return (end_time - start_time) * portTICK_RATE_MS;
	return ((((portTICK_RATE_MS) -1) - start_time) + end_time) * portTICK_RATE_MS;
}

/**
 * @brief Determine if the aircraft is safe to arm
 * @returns True if safe to arm, false otherwise
 */
static bool okToArm(void)
{
	// read alarms
	SystemAlarmsData alarms;
	SystemAlarmsGet(&alarms);


	// Check each alarm
	for (int i = 0; i < SYSTEMALARMS_ALARM_NUMELEM; i++)
	{
		if (alarms.Alarm[i] >= SYSTEMALARMS_ALARM_ERROR)
		{	// found an alarm thats set
			if (i == SYSTEMALARMS_ALARM_GPS || i == SYSTEMALARMS_ALARM_TELEMETRY)
				continue;

			return false;
		}
	}

	return true;
}

/**
 * @brief Update the flightStatus object only if value changed.  Reduces callbacks
 * @param[in] val The new value
 */
static void setArmedIfChanged(uint8_t val) {
	FlightStatusData flightStatus;
	FlightStatusGet(&flightStatus);

	if(flightStatus.Armed != val) {
		flightStatus.Armed = val;
		FlightStatusSet(&flightStatus);
	}
}

/**
 * @brief Process the inputs and determine whether to arm or not
 * @param[out] cmd The structure to set the armed in
 * @param[in] settings Settings indicating the necessary position
 */
static void processArm(ManualControlCommandData * cmd, ManualControlSettingsData * settings)
{

	bool lowThrottle = cmd->Throttle <= 0;

	if (settings->Arming == MANUALCONTROLSETTINGS_ARMING_ALWAYSDISARMED) {
		// In this configuration we always disarm
		setArmedIfChanged(FLIGHTSTATUS_ARMED_DISARMED);
	} else {
		// The throttle is not low, in case we where arming or disarming, abort
	 	if (!lowThrottle) {
			switch(armState) {
				case ARM_STATE_DISARMING_MANUAL:
				case ARM_STATE_DISARMING_TIMEOUT:
					armState = ARM_STATE_ARMED;
					break;
				case ARM_STATE_ARMING_MANUAL:
					armState = ARM_STATE_DISARMED;
					break;
				default:
					// Nothing needs to be done in the other states
					break;
			}
			return;
		}

		// The rest of these cases throttle is low
		if (settings->Arming == MANUALCONTROLSETTINGS_ARMING_ALWAYSARMED) {
			// In this configuration, we go into armed state as soon as the throttle is low, never disarm
			setArmedIfChanged(FLIGHTSTATUS_ARMED_ARMED);
			return;
		}


		// When the configuration is not "Always armed" and no "Always disarmed",
		// the state will not be changed when the throttle is not low
		static portTickType armedDisarmStart;
		float armingInputLevel = 0;

		// Calc channel see assumptions7
		int8_t sign = ((settings->Arming-MANUALCONTROLSETTINGS_ARMING_ROLLLEFT)%2) ? -1 : 1;
		switch ( (settings->Arming-MANUALCONTROLSETTINGS_ARMING_ROLLLEFT)/2 ) {
			case ARMING_CHANNEL_ROLL:    armingInputLevel = sign * cmd->Roll;    break;
			case ARMING_CHANNEL_PITCH:   armingInputLevel = sign * cmd->Pitch;   break;
			case ARMING_CHANNEL_YAW:     armingInputLevel = sign * cmd->Yaw;     break;
		}

		bool manualArm = false;
		bool manualDisarm = false;

		if (armingInputLevel <= -ARMED_THRESHOLD)
			manualArm = true;
		else if (armingInputLevel >= +ARMED_THRESHOLD)
			manualDisarm = true;

		switch(armState) {
			case ARM_STATE_DISARMED:
				setArmedIfChanged(FLIGHTSTATUS_ARMED_DISARMED);

				// only allow arming if it's OK too
				if (manualArm && okToArm()) {
					armedDisarmStart = lastSysTime;
					armState = ARM_STATE_ARMING_MANUAL;
				}
				break;

			case ARM_STATE_ARMING_MANUAL:
				setArmedIfChanged(FLIGHTSTATUS_ARMED_ARMING);

				if (manualArm && (timeDifferenceMs(armedDisarmStart, lastSysTime) > ARMED_TIME_MS))
					armState = ARM_STATE_ARMED;
				else if (!manualArm)
					armState = ARM_STATE_DISARMED;
				break;

			case ARM_STATE_ARMED:
				// When we get here, the throttle is low,
				// we go immediately to disarming due to timeout, also when the disarming mechanism is not enabled
				armedDisarmStart = lastSysTime;
				armState = ARM_STATE_DISARMING_TIMEOUT;
				setArmedIfChanged(FLIGHTSTATUS_ARMED_ARMED);
				break;

			case ARM_STATE_DISARMING_TIMEOUT:
				// We get here when armed while throttle low, even when the arming timeout is not enabled
				if ((settings->ArmedTimeout != 0) && (timeDifferenceMs(armedDisarmStart, lastSysTime) > settings->ArmedTimeout))
					armState = ARM_STATE_DISARMED;

				// Switch to disarming due to manual control when needed
				if (manualDisarm) {
					armedDisarmStart = lastSysTime;
					armState = ARM_STATE_DISARMING_MANUAL;
				}
				break;

			case ARM_STATE_DISARMING_MANUAL:
				if (manualDisarm &&(timeDifferenceMs(armedDisarmStart, lastSysTime) > ARMED_TIME_MS))
					armState = ARM_STATE_DISARMED;
				else if (!manualDisarm)
					armState = ARM_STATE_ARMED;
				break;
		}	// End Switch
	}
}

/**
 * @brief Determine which of three positions the flight mode switch is in and set flight mode accordingly
 * @param[out] cmd Pointer to the command structure to set the flight mode in
 * @param[in] settings The settings which indicate which position is which mode
 * @param[in] flightMode the value of the switch position
 */
static void processFlightMode(ManualControlSettingsData * settings, float flightMode)
{
	FlightStatusData flightStatus;
	FlightStatusGet(&flightStatus);

	uint8_t newMode;
	// Note here the code is ass
	if (flightMode < -FLIGHT_MODE_LIMIT)
		newMode = settings->FlightModePosition[0];
	else if (flightMode > FLIGHT_MODE_LIMIT)
		newMode = settings->FlightModePosition[2];
	else
		newMode = settings->FlightModePosition[1];

	if(flightStatus.FlightMode != newMode) {
		flightStatus.FlightMode = newMode;
		FlightStatusSet(&flightStatus);
	}

}

/**
 * @brief Determine if the manual input value is within acceptable limits
 * @returns return TRUE if so, otherwise return FALSE
 */
bool validInputRange(int16_t min, int16_t max, uint16_t value)
{
	if (min > max)
	{
		int16_t tmp = min;
		min = max;
		max = tmp;
	}
	return (value >= min - CONNECTION_OFFSET && value <= max + CONNECTION_OFFSET);
}

/**
  * @}
  * @}
  */
