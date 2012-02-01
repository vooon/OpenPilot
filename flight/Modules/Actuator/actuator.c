/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup ActuatorModule Actuator Module
 * @brief Compute servo/motor settings based on @ref ActuatorDesired "desired actuator positions" and aircraft type.
 * This is where all the mixing of channels is computed.
 * @{
 *
 * @file       actuator.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Actuator module. Drives the actuators (servos, motors etc).
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
#include "accessorydesired.h"
#include "actuator.h"
#include "actuatorsettings.h"
#include "systemsettings.h"
#include "actuatordesired.h"
#include "actuatorcommand.h"
#include "flightstatus.h"
#include "mixersettings.h"
#include "mixerstatus.h"
#include "cameradesired.h"
#include "manualcontrolcommand.h"

// Private constants
#define MAX_QUEUE_SIZE 2

#if defined(PIOS_ACTUATOR_STACK_SIZE)
#define STACK_SIZE_BYTES PIOS_ACTUATOR_STACK_SIZE
#else
#define STACK_SIZE_BYTES 1312
#endif

#define TASK_PRIORITY (tskIDLE_PRIORITY+4)
#define FAILSAFE_TIMEOUT_MS 100
#define MAX_MIX_ACTUATORS ACTUATORCOMMAND_CHANNEL_NUMELEM

// Private types


// Private variables
static xQueueHandle queue;
static xTaskHandle taskHandle;

static float lastResult[MAX_MIX_ACTUATORS]={0,0,0,0,0,0,0,0};
static float lastFilteredResult[MAX_MIX_ACTUATORS]={0,0,0,0,0,0,0,0};
static float filterAccumulator[MAX_MIX_ACTUATORS]={0,0,0,0,0,0,0,0};


// Private functions
static void actuatorTask(void* parameters);
static void actuator_update_rate(UAVObjEvent *);
static int16_t scaleChannel(float value, int16_t max, int16_t min, int16_t neutral);
static void setFailsafe();
static float MixerCurve(const float throttle, const float* curve, uint8_t elements);
static bool set_channel(uint8_t mixer_channel, uint16_t value);
float ProcessMixer(const int index, const float curve1, const float curve2,
		   MixerSettingsData* mixerSettings, ActuatorDesiredData* desired,
		   const float period);

//this structure is equivalent to the UAVObjects for one mixer.
typedef struct {
	uint8_t type;
	int8_t matrix[5];
} __attribute__((packed)) Mixer_t;

/**
 * @brief Module initialization
 * @return 0
 */
int32_t ActuatorStart()
{
	// Start main task
	xTaskCreate(actuatorTask, (signed char*)"Actuator", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &taskHandle);
	TaskMonitorAdd(TASKINFO_RUNNING_ACTUATOR, taskHandle);
	PIOS_WDG_RegisterFlag(PIOS_WDG_ACTUATOR);

	return 0;
}

/**
 * @brief Module initialization
 * @return 0
 */
int32_t ActuatorInitialize()
{
	// Create object queue
	queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(UAVObjEvent));

	ActuatorSettingsInitialize();
	ActuatorDesiredInitialize();
	MixerSettingsInitialize();
	ActuatorCommandInitialize();
#if defined(DIAGNOSTICS)
	MixerStatusInitialize();
#endif

	// Listen for ExampleObject1 updates
	ActuatorDesiredConnectQueue(queue);

	// If settings change, update the output rate
	ActuatorSettingsConnectCallback(actuator_update_rate);

	return 0;
}
MODULE_INITCALL(ActuatorInitialize, ActuatorStart)

/**
 * @brief Main Actuator module task
 *
 * Universal matrix based mixer for VTOL, helis and fixed wing.
 * Converts desired roll,pitch,yaw and throttle to servo/ESC outputs.
 *
 * Because of how the Throttle ranges from 0 to 1, the motors should too!
 *
 * Note this code depends on the UAVObjects for the mixers being all being the same
 * and in sequence. If you change the object definition, make sure you check the code!
 *
 * @return -1 if error, 0 if success
 */
static void actuatorTask(void* parameters)
{
	UAVObjEvent ev;
	portTickType lastSysTime;
	portTickType thisSysTime;
	float dT = 0.0f;

	ActuatorCommandData command;
	MixerSettingsData mixerSettings;
	ActuatorDesiredData desired;
	MixerStatusData mixerStatus;
	FlightStatusData flightStatus;

	uint8_t MotorsSpinWhileArmed;
	int16_t ChannelMax[ACTUATORCOMMAND_CHANNEL_NUMELEM];
	int16_t ChannelMin[ACTUATORCOMMAND_CHANNEL_NUMELEM];
	int16_t ChannelNeutral[ACTUATORCOMMAND_CHANNEL_NUMELEM];
	uint16_t ChannelUpdateFreq[ACTUATORSETTINGS_CHANNELUPDATEFREQ_NUMELEM];
	ActuatorSettingsChannelUpdateFreqGet(ChannelUpdateFreq);
	PIOS_Servo_SetHz(&ChannelUpdateFreq[0], ACTUATORSETTINGS_CHANNELUPDATEFREQ_NUMELEM);

	float * status = (float *)&mixerStatus; //access status objects as an array of floats

	// Go to the neutral (failsafe) values until an ActuatorDesired update is received
	setFailsafe();

	// Main task loop
	lastSysTime = xTaskGetTickCount();
	while (1)
	{
		PIOS_WDG_UpdateFlag(PIOS_WDG_ACTUATOR);

		// Wait until the ActuatorDesired object is updated, if a timeout then go to failsafe
		if ( xQueueReceive(queue, &ev, FAILSAFE_TIMEOUT_MS / portTICK_RATE_MS) != pdTRUE )
		{
			setFailsafe();
			continue;
		}

		// Check how long since last update
		thisSysTime = xTaskGetTickCount();
		if(thisSysTime > lastSysTime) // reuse dt in case of wraparound
			dT = (thisSysTime - lastSysTime) / portTICK_RATE_MS / 1000.0f;
		lastSysTime = thisSysTime;

		FlightStatusGet(&flightStatus);
		MixerSettingsGet (&mixerSettings);
		ActuatorDesiredGet(&desired);
		ActuatorCommandGet(&command);

#if defined(DIAGNOSTICS)
		MixerStatusGet(&mixerStatus);
#endif
		ActuatorSettingsMotorsSpinWhileArmedGet(&MotorsSpinWhileArmed);
		ActuatorSettingsChannelMaxGet(ChannelMax);
		ActuatorSettingsChannelMinGet(ChannelMin);
		ActuatorSettingsChannelNeutralGet(ChannelNeutral);

		int nMixers = 0;
		Mixer_t * mixers = (Mixer_t *)&mixerSettings.Mixer1Type;
		for(int ct=0; ct < MAX_MIX_ACTUATORS; ct++)
		{
			if(mixers[ct].type != MIXERSETTINGS_MIXER1TYPE_DISABLED)
			{
				nMixers ++;
			}
		}
		if((nMixers < 2) && !ActuatorCommandReadOnly()) //Nothing can fly with less than two mixers.
		{
			setFailsafe(); // So that channels like PWM buzzer keep working
			continue;
		}

		AlarmsClear(SYSTEMALARMS_ALARM_ACTUATOR);

		bool armed = flightStatus.Armed == FLIGHTSTATUS_ARMED_ARMED;
		bool positiveThrottle = desired.Throttle >= 0.00f;
		bool spinWhileArmed = MotorsSpinWhileArmed == ACTUATORSETTINGS_MOTORSSPINWHILEARMED_TRUE;

		float curve1 = MixerCurve(desired.Throttle,mixerSettings.ThrottleCurve1,MIXERSETTINGS_THROTTLECURVE1_NUMELEM);
		
		//The source for the secondary curve is selectable
		float curve2 = 0;
		AccessoryDesiredData accessory;
		switch(mixerSettings.Curve2Source) {
			case MIXERSETTINGS_CURVE2SOURCE_THROTTLE:
				curve2 = MixerCurve(desired.Throttle,mixerSettings.ThrottleCurve2,MIXERSETTINGS_THROTTLECURVE2_NUMELEM);
				break;
			case MIXERSETTINGS_CURVE2SOURCE_ROLL:
				curve2 = MixerCurve(desired.Roll,mixerSettings.ThrottleCurve2,MIXERSETTINGS_THROTTLECURVE2_NUMELEM);
				break;
			case MIXERSETTINGS_CURVE2SOURCE_PITCH:
				curve2 = MixerCurve(desired.Pitch,mixerSettings.ThrottleCurve2,
				MIXERSETTINGS_THROTTLECURVE2_NUMELEM);
				break;
			case MIXERSETTINGS_CURVE2SOURCE_YAW:
				curve2 = MixerCurve(desired.Yaw,mixerSettings.ThrottleCurve2,MIXERSETTINGS_THROTTLECURVE2_NUMELEM);
				break;
			case MIXERSETTINGS_CURVE2SOURCE_COLLECTIVE:
				ManualControlCommandCollectiveGet(&curve2);
				curve2 = MixerCurve(curve2,mixerSettings.ThrottleCurve2,
				MIXERSETTINGS_THROTTLECURVE2_NUMELEM);
				break;
			case MIXERSETTINGS_CURVE2SOURCE_ACCESSORY0:
			case MIXERSETTINGS_CURVE2SOURCE_ACCESSORY1:
			case MIXERSETTINGS_CURVE2SOURCE_ACCESSORY2:
			case MIXERSETTINGS_CURVE2SOURCE_ACCESSORY3:
			case MIXERSETTINGS_CURVE2SOURCE_ACCESSORY4:
			case MIXERSETTINGS_CURVE2SOURCE_ACCESSORY5:
				if(AccessoryDesiredInstGet(mixerSettings.Curve2Source - MIXERSETTINGS_CURVE2SOURCE_ACCESSORY0,&accessory) == 0)
					curve2 = MixerCurve(accessory.AccessoryVal,mixerSettings.ThrottleCurve2,MIXERSETTINGS_THROTTLECURVE2_NUMELEM);
				else
					curve2 = 0;
				break;
		}

		for(int ct=0; ct < MAX_MIX_ACTUATORS; ct++)
		{
			if(mixers[ct].type == MIXERSETTINGS_MIXER1TYPE_DISABLED) {
				// Set to minimum if disabled.  This is not the same as saying PWM pulse = 0 us
				status[ct] = -1;
				command.Channel[ct] = 0;
				continue;
			}

			if((mixers[ct].type == MIXERSETTINGS_MIXER1TYPE_MOTOR) || (mixers[ct].type == MIXERSETTINGS_MIXER1TYPE_SERVO))
				status[ct] = ProcessMixer(ct, curve1, curve2, &mixerSettings, &desired, dT);
			else
				status[ct] = -1;



			// Motors have additional protection for when to be on
			if(mixers[ct].type == MIXERSETTINGS_MIXER1TYPE_MOTOR) {

				// If not armed or motors aren't meant to spin all the time
				if( !armed ||
				   (!spinWhileArmed && !positiveThrottle))
				{
					filterAccumulator[ct] = 0;
					lastResult[ct] = 0;
					status[ct] = -1;  //force min throttle
				}
				// If armed meant to keep spinning,
				else if ((spinWhileArmed && !positiveThrottle) ||
					 (status[ct] < 0) )
					status[ct] = 0;
			}

			// If an accessory channel is selected for direct bypass mode
			// In this configuration the accessory channel is scaled and mapped
			// directly to output.  Note: THERE IS NO SAFETY CHECK HERE FOR ARMING
			// these also will not be updated in failsafe mode.  I'm not sure what
			// the correct behavior is since it seems domain specific.  I don't love
			// this code
			if( (mixers[ct].type >= MIXERSETTINGS_MIXER1TYPE_ACCESSORY0) &&
			   (mixers[ct].type <= MIXERSETTINGS_MIXER1TYPE_ACCESSORY5))
			{
				if(AccessoryDesiredInstGet(mixers[ct].type - MIXERSETTINGS_MIXER1TYPE_ACCESSORY0,&accessory) == 0)
					status[ct] = accessory.AccessoryVal;
				else
					status[ct] = -1;
			}
			if( (mixers[ct].type >= MIXERSETTINGS_MIXER1TYPE_CAMERAROLL) &&
			   (mixers[ct].type <= MIXERSETTINGS_MIXER1TYPE_CAMERAYAW))
			{
				CameraDesiredData cameraDesired;
				if( CameraDesiredGet(&cameraDesired) == 0 ) {
					switch(mixers[ct].type) {
						case MIXERSETTINGS_MIXER1TYPE_CAMERAROLL:
							status[ct] = cameraDesired.Roll;
							break;
						case MIXERSETTINGS_MIXER1TYPE_CAMERAPITCH:
							status[ct] = cameraDesired.Pitch;
							break;
						case MIXERSETTINGS_MIXER1TYPE_CAMERAYAW:
							status[ct] = cameraDesired.Yaw;
							break;
						default:
							break;
					}
				}
				else
					status[ct] = -1;
			}
		}
		
		for(int i = 0; i < MAX_MIX_ACTUATORS; i++) 
			command.Channel[i] = scaleChannel(status[i],
							   ChannelMax[i],
							   ChannelMin[i],
							   ChannelNeutral[i]);
			
		// Store update time
		command.UpdateTime = 1000.0f*dT;
		if(1000.0f*dT > command.MaxUpdateTime)
			command.MaxUpdateTime = 1000.0f*dT;
		
		// Update output object
		ActuatorCommandSet(&command);
		// Update in case read only (eg. during servo configuration)
		ActuatorCommandGet(&command);

#if defined(DIAGNOSTICS)
		MixerStatusSet(&mixerStatus);
#endif
		

		// Update servo outputs
		bool success = true;

		for (int n = 0; n < ACTUATORCOMMAND_CHANNEL_NUMELEM; ++n)
		{
			success &= set_channel(n, command.Channel[n]);
		}

		if(!success) {
			command.NumFailedUpdates++;
			ActuatorCommandSet(&command);
			AlarmsSet(SYSTEMALARMS_ALARM_ACTUATOR, SYSTEMALARMS_ALARM_CRITICAL);
		}

	}
}



/**
 *Process mixing for one actuator
 */
float ProcessMixer(const int index, const float curve1, const float curve2,
		   MixerSettingsData* mixerSettings, ActuatorDesiredData* desired, const float period)
{
	Mixer_t * mixers = (Mixer_t *)&mixerSettings->Mixer1Type; //pointer to array of mixers in UAVObjects
	Mixer_t * mixer = &mixers[index];
	float result = (((float)mixer->matrix[MIXERSETTINGS_MIXER1VECTOR_THROTTLECURVE1] / 128.0f) * curve1) +
	(((float)mixer->matrix[MIXERSETTINGS_MIXER1VECTOR_THROTTLECURVE2] / 128.0f) * curve2) +
	(((float)mixer->matrix[MIXERSETTINGS_MIXER1VECTOR_ROLL] / 128.0f) * desired->Roll) +
	(((float)mixer->matrix[MIXERSETTINGS_MIXER1VECTOR_PITCH] / 128.0f) * desired->Pitch) +
	(((float)mixer->matrix[MIXERSETTINGS_MIXER1VECTOR_YAW] / 128.0f) * desired->Yaw);
	if(mixer->type == MIXERSETTINGS_MIXER1TYPE_MOTOR)
	{
		if(result < 0.0f) //idle throttle
		{
			result = 0.0f;
		}

		//feed forward
		float accumulator = filterAccumulator[index];
		accumulator += (result - lastResult[index]) * mixerSettings->FeedForward;
		lastResult[index] = result;
		result += accumulator;
		if(period !=0)
		{
			if(accumulator > 0.0f)
			{
				float filter = mixerSettings->AccelTime / period;
				if(filter <1)
				{
					filter = 1;
				}
				accumulator -= accumulator / filter;
			}else
			{
				float filter = mixerSettings->DecelTime / period;
				if(filter <1)
				{
					filter = 1;
				}
				accumulator -= accumulator / filter;
			}
		}
		filterAccumulator[index] = accumulator;
		result += accumulator;

		//acceleration limit
		float dt = result - lastFilteredResult[index];
		float maxDt = mixerSettings->MaxAccel * period;
		if(dt > maxDt) //we are accelerating too hard
		{
			result = lastFilteredResult[index] + maxDt;
		}
		lastFilteredResult[index] = result;
	}

	return(result);
}


/**
 *Interpolate a throttle curve. Throttle input should be in the range 0 to 1.
 *Output is in the range 0 to 1.
 */
static float MixerCurve(const float throttle, const float* curve, uint8_t elements)
{
	float scale = throttle * (float) (elements - 1);
	int idx1 = scale;
	scale -= (float)idx1; //remainder
	if(curve[0] < -1)
	{
		return(throttle);
	}
	if (idx1 < 0)
	{
		idx1 = 0; //clamp to lowest entry in table
		scale = 0;
	}
	int idx2 = idx1 + 1;
	if(idx2 >= elements)
	{
		idx2 = elements -1; //clamp to highest entry in table
		if(idx1 >= elements)
		{
			idx1 = elements -1;
		}
	}
	return curve[idx1] * (1.0f - scale) + curve[idx2] * scale;
}


/**
 * Convert channel from -1/+1 to servo pulse duration in microseconds
 */
static int16_t scaleChannel(float value, int16_t max, int16_t min, int16_t neutral)
{
	int16_t valueScaled;
	// Scale
	if ( value >= 0.0f)
	{
		valueScaled = (int16_t)(value*((float)(max-neutral))) + neutral;
	}
	else
	{
		valueScaled = (int16_t)(value*((float)(neutral-min))) + neutral;
	}

	if (max>min)
	{
		if( valueScaled > max ) valueScaled = max;
		if( valueScaled < min ) valueScaled = min;
	}
	else
	{
		if( valueScaled < max ) valueScaled = max;
		if( valueScaled > min ) valueScaled = min;
	}

	return valueScaled;
}

/**
 * Set actuator output to the neutral values (failsafe)
 */
static void setFailsafe()
{
	/* grab only the modules parts that we are going to use */
	int16_t ChannelMin[ACTUATORCOMMAND_CHANNEL_NUMELEM];
	ActuatorSettingsChannelMinGet(ChannelMin);
	int16_t ChannelNeutral[ACTUATORCOMMAND_CHANNEL_NUMELEM];
	ActuatorSettingsChannelNeutralGet(ChannelNeutral);
	int16_t Channel[ACTUATORCOMMAND_CHANNEL_NUMELEM];
	ActuatorCommandChannelGet(Channel);

	MixerSettingsData mixerSettings;
	MixerSettingsGet (&mixerSettings);
	Mixer_t * mixers = (Mixer_t *)&mixerSettings.Mixer1Type; //pointer to array of mixers in UAVObjects

	// Reset ActuatorCommand to safe values
	for (int n = 0; n < ACTUATORCOMMAND_CHANNEL_NUMELEM; ++n)
	{

		if(mixers[n].type == MIXERSETTINGS_MIXER1TYPE_MOTOR)
		{
			Channel[n] = ChannelMin[n];
		}
		else if(mixers[n].type == MIXERSETTINGS_MIXER1TYPE_SERVO)
		{
			Channel[n] = ChannelNeutral[n];
		}
		else
		{
			Channel[n] = 0;
		}
		
		
	}

	// Set alarm
	AlarmsSet(SYSTEMALARMS_ALARM_ACTUATOR, SYSTEMALARMS_ALARM_CRITICAL);

	// Update servo outputs
	for (int n = 0; n < ACTUATORCOMMAND_CHANNEL_NUMELEM; ++n)
	{
		set_channel(n, Channel[n]);
	}

	// Update output object's parts that we changed
	ActuatorCommandChannelSet(Channel);
}


/**
 * @brief Update the servo update rate
 */
static void actuator_update_rate(UAVObjEvent * ev)
{
	uint16_t ChannelUpdateFreq[ACTUATORSETTINGS_CHANNELUPDATEFREQ_NUMELEM];
	if ( ev->obj == ActuatorSettingsHandle() ) {
		ActuatorSettingsChannelUpdateFreqGet(ChannelUpdateFreq);
		PIOS_Servo_SetHz(&ChannelUpdateFreq[0], ACTUATORSETTINGS_CHANNELUPDATEFREQ_NUMELEM);
	}
}

#if defined(ARCH_POSIX) || defined(ARCH_WIN32)
static bool set_channel(uint8_t mixer_channel, uint16_t value) {
	return true;
}
#else
static bool set_channel(uint8_t mixer_channel, uint16_t value) {

	ActuatorSettingsData settings;
	ActuatorSettingsGet(&settings);

	switch(settings.ChannelType[mixer_channel]) {
		case ACTUATORSETTINGS_CHANNELTYPE_PWMALARMBUZZER: {
			// This is for buzzers that take a PWM input

			static uint32_t currBuzzTune = 0;
			static uint32_t currBuzzTuneState;
			uint32_t bewBuzzTune;

			// Decide what tune to play
			if (AlarmsGet(SYSTEMALARMS_ALARM_BATTERY) > SYSTEMALARMS_ALARM_WARNING) {
				bewBuzzTune = 0b11110110110000;	// pause, short, short, short, long
			} else if (AlarmsGet(SYSTEMALARMS_ALARM_GPS) >= SYSTEMALARMS_ALARM_WARNING) {
				bewBuzzTune = 0x80000000;			// pause, short
			} else {
				bewBuzzTune = 0;
			}

			// Do we need to change tune?
			if (bewBuzzTune != currBuzzTune) {
				currBuzzTune = bewBuzzTune;
				currBuzzTuneState = currBuzzTune;
			}


			// Play tune
			bool buzzOn = false;
			static portTickType lastSysTime = 0;
			portTickType thisSysTime = xTaskGetTickCount();
			portTickType dT = 0;

			// For now, only look at the battery alarm, because functions like AlarmsHasCritical() can block for some time; to be discussed
			if (currBuzzTune) {
				if(thisSysTime > lastSysTime)
					dT = thisSysTime - lastSysTime;
				buzzOn = (currBuzzTuneState&1);	// Buzz when the LS bit is 1
				if (dT > 80) {
					// Go to next bit in alarm_seq_state
					currBuzzTuneState >>= 1;
					if (currBuzzTuneState == 0)
						currBuzzTuneState = currBuzzTune;	// All done, re-start the tune
					lastSysTime = thisSysTime;
				}
			}
			PIOS_Servo_Set(	settings.ChannelAddr[mixer_channel],
							buzzOn?settings.ChannelMax[mixer_channel]:settings.ChannelMin[mixer_channel]);
			return true;
		}
		case ACTUATORSETTINGS_CHANNELTYPE_PWM:
			PIOS_Servo_Set(settings.ChannelAddr[mixer_channel], value);
			return true;
#if defined(PIOS_INCLUDE_I2C_ESC)
		case ACTUATORSETTINGS_CHANNELTYPE_MK:
			return PIOS_SetMKSpeed(settings.ChannelAddr[mixer_channel],value);
		case ACTUATORSETTINGS_CHANNELTYPE_ASTEC4:
			return PIOS_SetAstec4Speed(settings.ChannelAddr[mixer_channel],value);
			break;
#endif
		default:
			return false;
	}

	return false;

}
#endif


/**
 * @}
 * @}
 */
