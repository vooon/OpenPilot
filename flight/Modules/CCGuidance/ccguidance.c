/**
 ******************************************************************************
 *
 * @file       ccguidance.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      CCGuidance for CopterControl. Fixed wing only.
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
 * Input object: GPSPosition
 * Input object: ManualControlCommand
 * Output object: StabilizationDesired
 *
 * This module will periodically update the value of the AttitudeDesired object.
 *
 * The module executes in its own thread in this example.
 *
 * Modules have no API, all communication to other modules is done through UAVObjects.
 * However modules may use the API exposed by shared libraries.
 * See the OpenPilot wiki for more details.
 * http://www.openpilot.org/OpenPilot_Application_Architecture
 *
 */

#include "openpilot.h"
#include "ccguidancesettings.h"
#include "gpsposition.h"
#include "manualcontrol.h"
#include "manualcontrolcommand.h"
#include "flightstatus.h"
#include "stabilizationdesired.h"
#include "systemsettings.h"
#include "hwsettings.h"
#include "homelocation.h"
#include "positionactual.h"
#include "velocityactual.h"
#include "attitudeactual.h"

// Private constants
#define STACK_SIZE_BYTES 530
#define TASK_PRIORITY (tskIDLE_PRIORITY+1)
#define RAD2DEG (180.0/M_PI)
#define DEG2RAD (M_PI/180.0)
#define DELAY_ENABLE_FAILSAFE			4000	// In the event of a transmitter signal loss, delay before returning, in[ms]
#define SPEEDTOBASE_TRACK_ENABLE		1.0f	// at a rate less than this, the flight enabled tacks.
// The coefficient for the calculation of the maximum deviation range of the elevator to climb,
// depending on the speed of flight in the direction of the base.
// At a speed equal to SPEEDTOBASE_PITH_TO_NEUTRAL_SPEED,
// the elevator is set to ccguidanceSettings.Pitch [CCGUIDANCESETTINGS_PITCH_NEUTRAL]
#define SPEEDTOBASE_PITH_TO_NEUTRAL_KP		12
#define SPEEDTOBASE_PITH_TO_NEUTRAL_SPEED	2.0f

// Private types
static struct Integral {
	float EnergyError;
	float VelocityError;
	float AltitudeError;
	float CourseError;
} *integral;

// Private variables
static uint8_t positionHoldLast;
static xTaskHandle ccguidanceTaskHandle;
static bool PositionActualNew_flag = false;
static bool CCGuidanceEnabled = false;

// Private functions
static void PositionActualUpdatedCb(UAVObjEvent * objEv);
static void ccguidanceTask(void *parameters);
static float bound(float val, float min, float max);

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t CCGuidanceStart()
{
	// Start main task
	if (CCGuidanceEnabled) {
		// Start main task
		xTaskCreate(ccguidanceTask, (signed char *)"CCGuidance", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &ccguidanceTaskHandle);
		TaskMonitorAdd(TASKINFO_RUNNING_CCGUIDANCE, ccguidanceTaskHandle);

		// indicate error - can only run correctly if GPSPosition is ever set, which will change the alarm state.
		AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_CRITICAL);
	}

	return 0;
}

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t CCGuidanceInitialize()
{
	HwSettingsInitialize();
	SystemSettingsInitialize();

	uint8_t optionalModules[HWSETTINGS_OPTIONALMODULES_NUMELEM];
	PositionActualNew_flag=false;

	HwSettingsOptionalModulesGet(optionalModules);

	SystemSettingsData systemSettings;
	SystemSettingsGet(&systemSettings);

	if ((optionalModules[HWSETTINGS_OPTIONALMODULES_CCGUIDANCE] == HWSETTINGS_OPTIONALMODULES_ENABLED) &&
		((systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWING) ||
		(systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGELEVON) ||
		(systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGVTAIL) ||
		(systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_VTOL) ||
		(systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_QUADP) ||
		(systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_QUADX) ||
		(systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_HEXA) ))

		CCGuidanceEnabled = true;
	else
		CCGuidanceEnabled = false;

	if (CCGuidanceEnabled) {

		CCGuidanceSettingsInitialize();
//		GPSPositionInitialize();
//		HomeLocationInitialize();
		PositionActualInitialize();
		VelocityActualInitialize();

		integral = (struct Integral *) pvPortMalloc(sizeof(struct Integral));
		memset(integral, 0, sizeof(struct Integral));

		PositionActualConnectCallback(&PositionActualUpdatedCb);

		return 0;
	}
	return -1;
}
// no macro - optional module
MODULE_INITCALL(CCGuidanceInitialize, CCGuidanceStart)

/**
 * Main module callback.
 */
static void ccguidanceTask(void *parameters)
{
	CCGuidanceSettingsData ccguidanceSettings;
	FlightStatusData flightStatus;
	StabilizationDesiredData stabDesired;
	PositionActualData positionActual;
	VelocityActualData velocityActual;

	portTickType lastUpdateTime;
	float positionDesiredNorth  = 0.0f; // values ​​in meters
	float positionDesiredEast   = 0.0f;
	float positionDesiredDown   = 0.0f;
	float DistanceToBaseNorth   = 0.0f;
	float DistanceToBaseEast    = 0.0f;
	float DistanceToBaseOld     = 0.0f;

	float SpeedToRTB            = 0.0f;
	float CourseJob             = 0.0f;
	float CourseError           = 0.0f;
	float attitudeActualYaw     = 0.0f;
	float headingActual         = 0.0f;
	float DistanceToBase        = 0.0f;
	float ThrottleStep          = 0.0f;
	float ThrottleJob           = 0.0f;
	float dT                    = 0.0f;
	float EnergyError           = 0.0f;
	float VelocityError         = 0.0f;
	float VelocityActualNE      = 0.0f;
	float AltitudeError         = 0.0f;
	float dT_gps                = 0.0f;
	float thisTimeHoldCourse    = 0.0f;
	float thisTimeTacks         = 0.0f;
	float thisTimeSpeedToRTB    = 0.0f;

	uint16_t TimeEnableFailSafe = 0;
	uint8_t GPSstatus           = GPSPOSITION_STATUS_NOFIX;

	bool firstRunSetCourse      = TRUE;
	bool TacksAngleRight        = FALSE;
	bool throttleManualControl  = TRUE;

	// Main task loop
	lastUpdateTime = xTaskGetTickCount();
	while (1) {
		CCGuidanceSettingsGet(&ccguidanceSettings);

		vTaskDelayUntil(&lastUpdateTime, ccguidanceSettings.UpdatePeriod / portTICK_RATE_MS);

		dT = (float)ccguidanceSettings.UpdatePeriod * 0.001f;				//change from milliseconds to seconds.

		StabilizationDesiredGet(&stabDesired);
		FlightStatusGet(&flightStatus);

		//Activate failsafe mode, activate return to base
		if ((AlarmsGet(SYSTEMALARMS_ALARM_MANUALCONTROL) != SYSTEMALARMS_ALARM_OK) &&
			(ccguidanceSettings.FailSafeRTB == TRUE))	{
			TimeEnableFailSafe += ccguidanceSettings.UpdatePeriod;
			if (TimeEnableFailSafe >= DELAY_ENABLE_FAILSAFE) {
				TimeEnableFailSafe = 0;
				flightStatus.FlightMode = FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE;
				FlightStatusSet(&flightStatus);
			}
		} else TimeEnableFailSafe = 0;

		//Check that Guidance is enabled
		if (PARSE_FLIGHT_MODE(flightStatus.FlightMode) != FLIGHTMODE_GUIDANCE) {
			// reset navigation mode
			positionHoldLast = 0;
			AlarmsClear(SYSTEMALARMS_ALARM_GUIDANCE);
			continue;
		}

		dT_gps += dT;

		AttitudeActualYawGet (&attitudeActualYaw);
		GPSPositionStatusGet(&GPSstatus);

		if( PositionActualNew_flag == true){

			PositionActualGet(&positionActual);
			uint8_t HomeLocationSet;
			HomeLocationSetGet(&HomeLocationSet);

			// Setting the target coordinates to the selected mode of flight navigation.
			if(positionHoldLast != 1 && (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD2 ||
			  (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE && HomeLocationSet == FALSE) ||
			  (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_FLIGHTTOTARGET && ccguidanceSettings.TargetLocationSet == FALSE)))
			  {
				/* When entering position hold mode, use current position as set point*/
				positionDesiredNorth = positionActual.North;
				positionDesiredEast = positionActual.East;
				positionDesiredDown = positionActual.Down;
				positionHoldLast = 1;
				firstRunSetCourse = TRUE;
			} else if (positionHoldLast != 2 && (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE && HomeLocationSet == TRUE)) {
				/* When we RTB, safe home position */
				positionDesiredNorth = 0;
				positionDesiredEast = 0;
				positionDesiredDown = ccguidanceSettings.AltitudeOffsetHome;
				positionHoldLast = 2;
				firstRunSetCourse = TRUE;
			} else 	if (positionHoldLast != 3 &&
				((flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_FLIGHTTOTARGET) && ( HomeLocationSet == TRUE))) {
				/* Sets target position */
				positionDesiredNorth = ccguidanceSettings.TargetLocationNorth;
				positionDesiredEast = ccguidanceSettings.TargetLocationEast;
				positionDesiredDown = ccguidanceSettings.TargetLocationDown;
				positionHoldLast = 3;
				firstRunSetCourse = TRUE;
			} // The end of the coordinates of a destination of the selected navigation mode.

			stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_ROLL] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
			stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_PITCH] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
			stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_YAW] = STABILIZATIONDESIRED_STABILIZATIONMODE_RATE;

			thisTimeHoldCourse += dT_gps;
			thisTimeTacks      += dT_gps;
			thisTimeSpeedToRTB += dT_gps;

			/* safety */
			if (GPSstatus == GPSPOSITION_STATUS_FIX3D) {
				// The calculation of flight speed in the plane.
				VelocityActualGet(&velocityActual);
				VelocityActualNE = sqrt(velocityActual.North*velocityActual.North + velocityActual.East*velocityActual.East);

				// First activate RTB
				if (firstRunSetCourse == TRUE) {
					firstRunSetCourse = FALSE;
					// Save current location to HomeLocation if flightStatus = DISARMED
					/*if (flightStatus.Armed == FLIGHTSTATUS_ARMED_DISARMED &&
						StateSaveCurrentPositionToRTB == FALSE) {
						HomeLocationSet = FALSE;
						HomeLocationSetSet(&HomeLocationSet);
					}
					*/

					// initialize variables
					CourseJob          = attitudeActualYaw;
					SpeedToRTB         = VelocityActualNE;
					TacksAngleRight    = FALSE;
					thisTimeHoldCourse = 0.0f;
					thisTimeTacks      = 0.0f;
					thisTimeSpeedToRTB = 0.0f;
					DistanceToBaseOld  = 0.0f;
					dT_gps             = 0.0f;
				}

				// Flight of a fixed rate over time TimeHoldCourse
				if (thisTimeHoldCourse >= ccguidanceSettings.TimeHoldCourse) {
					/* 1. Calculate course */
					// calculation of rectangular coordinates
					DistanceToBaseNorth = positionDesiredNorth - positionActual.North;
					DistanceToBaseEast = positionDesiredEast - positionActual.East;
					// square of the distance to the base
					DistanceToBase = sqrt((DistanceToBaseNorth * DistanceToBaseNorth) + (DistanceToBaseEast * DistanceToBaseEast));
					// true direction of the base
					CourseJob = atan2(DistanceToBaseEast, DistanceToBaseNorth) * RAD2DEG;
					if (isnan(CourseJob)) CourseJob=0;

					if (DistanceToBase > ccguidanceSettings.RadiusBase) {
						if ((DistanceToBaseOld != 0) && (thisTimeSpeedToRTB != 0.0f)) {
							// Calculation ground speed to base
							SpeedToRTB = (DistanceToBaseOld - DistanceToBase) / thisTimeSpeedToRTB;
							thisTimeSpeedToRTB = 0.0f;
						}
						DistanceToBaseOld = DistanceToBase;

						// algorithm flight tacks
						if (ccguidanceSettings.TacksTime != 0.0f) {
							// TacksTime - during this time flown tack, left or right.
							if ((SpeedToRTB < SPEEDTOBASE_TRACK_ENABLE) && (thisTimeTacks >= ccguidanceSettings.TacksTime)) {
								thisTimeTacks = 0.0f;
								TacksAngleRight = !TacksAngleRight;	// Changing tack.
							}
							if (thisTimeTacks <= ccguidanceSettings.TacksTime) {
								// Adjust the rate for set tack.
								if (TacksAngleRight == TRUE) CourseJob += ccguidanceSettings.TacksAngle;	// Right track
								else CourseJob -= ccguidanceSettings.TacksAngle;							// Left track
							}
						}
						thisTimeHoldCourse = 0.0f;		// Turn on the retention of the new direction of flight.
					} else {
						/* Circular motion in the area between RadiusBase */
						if (ccguidanceSettings.CircleAroundBase == TRUE) {
							if (ccguidanceSettings.CircleAroundBaseAngle == 0.0f) {
								CourseJob += bound((180.0f-180.0f * (DistanceToBase / ccguidanceSettings.RadiusBase)), 0.0f, 100.0f);
							} else CourseJob += ccguidanceSettings.CircleAroundBaseAngle;
									//CourseJob += acos(DistanceToBase / ccguidanceSettings.RadiusBase) * RAD2DEG;
						}
						// reset variable in radius base.
						SpeedToRTB        = VelocityActualNE;
						TacksAngleRight   = FALSE;
						thisTimeTacks     = 0.0f;
						DistanceToBaseOld = 0.0f;
						thisTimeHoldCourse = ccguidanceSettings.TimeHoldCourse;
					}

					// Current heading
					headingActual = atan2f(velocityActual.East, velocityActual.North) * RAD2DEG;
					// Required course for the gyroscope attitudeYaw.
					// Here is compensated as wind drift and drift gyro Yaw.
					CourseJob = attitudeActualYaw + CourseJob - headingActual;
					while (CourseJob<-180.0f) CourseJob+=360.0f;
					while (CourseJob>180.0f)  CourseJob-=360.0f;
				}


				/* Compute AltitudeError, VelocityActualNE, VelocityError, EnergyError. */
				AltitudeError = -(positionDesiredDown - positionActual.Down);
				VelocityActualNE = (SpeedToRTB > SPEEDTOBASE_PITH_TO_NEUTRAL_SPEED) ? VelocityActualNE : 0;
				VelocityError = ccguidanceSettings.VelocityDesired - VelocityActualNE;
				EnergyError = (ccguidanceSettings.VelocityDesired * ccguidanceSettings.VelocityDesired -
							positionDesiredDown * ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_GRAV2KP]) -
							(VelocityActualNE * VelocityActualNE -
							positionActual.Down * ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_GRAV2KP]);

				/* 2. Pitch */
				if (ccguidanceSettings.SpeedControlThePitch == TRUE) {
					if (ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KI] > 0.0f){
						//Integrate with saturation
						integral->VelocityError=bound(integral->VelocityError + VelocityError * dT_gps,
													-ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_ILIMIT] /
													ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KI],
													ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_ILIMIT] /
													ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KI]);
					}
					stabDesired.Pitch = bound(
						-(VelocityError * ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KP] +
						integral->VelocityError * ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KI]) +
						ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_NEUTRAL],
						ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_SINK],
						ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_CLIMB]
						);
				} else {
					if (ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KI] > 0.0f){
						//Integrate with saturation
						integral->AltitudeError=bound(integral->AltitudeError + AltitudeError * dT_gps,
													-ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_ILIMIT] /
													ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KI],
													ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_ILIMIT] /
													ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KI]);
					}
					stabDesired.Pitch = bound(
						AltitudeError * ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KP] +
						integral->AltitudeError * ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KI] +
						ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_NEUTRAL],
						ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_SINK],
						// decrease in the maximum deflection of the elevator to climb,
						// if the velocity of the target is less than SPEEDTOBASE_PITH_TO_NEUTRAL_SPEED
						bound((SpeedToRTB - SPEEDTOBASE_PITH_TO_NEUTRAL_SPEED) *
								SPEEDTOBASE_PITH_TO_NEUTRAL_KP,
								ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_NEUTRAL],
								ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_CLIMB])
						);
				}
				/* 3. Throttle */
				//Integrate with bound. Make integral leaky for better performance. Approximately 30s time constant.
				if (ccguidanceSettings.ThrottleControl == TRUE) {
					if (ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_KI] > 0.0f){
						integral->EnergyError=bound(integral->EnergyError + EnergyError * dT_gps,
													-ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_ILIMIT] /
													ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_KI],
													ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_ILIMIT] /
													ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_KI]) *
													(1.0f-1.0f/(1.0f+30.0f/dT_gps));
					}
					ThrottleJob = EnergyError * ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_KP] +
									integral->EnergyError * ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_KI];

					throttleManualControl = FALSE;
				} else {
					/* Throttle (manual) */
					throttleManualControl = TRUE;
				}
				AlarmsClear(SYSTEMALARMS_ALARM_GUIDANCE);
			}
			PositionActualNew_flag = false;
			dT_gps = 0;
		}

		if (GPSstatus == GPSPOSITION_STATUS_FIX3D) {
			/*--- Hold the desired course. ---*/
			CourseError = CourseJob - attitudeActualYaw;
			if (CourseError<-180.0f) CourseError+=360.0f;
			if (CourseError>180.0f)  CourseError-=360.0f;

			if (ccguidanceSettings.CoursePI[CCGUIDANCESETTINGS_COURSEPI_KI] > 0.0f){
				//Integrate with saturation
				integral->CourseError=bound(integral->CourseError + CourseError * dT,
											-ccguidanceSettings.CoursePI[CCGUIDANCESETTINGS_COURSEPI_ILIMIT] /
											ccguidanceSettings.CoursePI[CCGUIDANCESETTINGS_COURSEPI_KI],
											ccguidanceSettings.CoursePI[CCGUIDANCESETTINGS_COURSEPI_ILIMIT] /
											ccguidanceSettings.CoursePI[CCGUIDANCESETTINGS_COURSEPI_KI]);
			}

			float stabDesiredCourse = bound(
				CourseError * ccguidanceSettings.CoursePI[CCGUIDANCESETTINGS_COURSEPI_KP] +
				integral->CourseError * ccguidanceSettings.CoursePI[CCGUIDANCESETTINGS_COURSEPI_KI] +
				ccguidanceSettings.YawRoll[CCGUIDANCESETTINGS_YAWROLL_NEUTRAL],
				-ccguidanceSettings.YawRoll[CCGUIDANCESETTINGS_YAWROLL_MAX],
				ccguidanceSettings.YawRoll[CCGUIDANCESETTINGS_YAWROLL_MAX]
				);
			if (ccguidanceSettings.AileronsControlCourse == TRUE) {
				stabDesired.Roll = stabDesiredCourse;
				stabDesired.Yaw = 0;
			} else {
				stabDesired.Yaw = stabDesiredCourse;
				stabDesired.Roll = 0;
			}

		} else {
			/* Fallback, no position data! */
			stabDesired.Yaw = ccguidanceSettings.YawRoll[CCGUIDANCESETTINGS_YAWROLL_NEUTRAL];
			stabDesired.Roll = ccguidanceSettings.YawRoll[CCGUIDANCESETTINGS_YAWROLL_MAX];
			stabDesired.Pitch = ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_CLIMB];
			throttleManualControl = TRUE; //  Throttle (manual)
			AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_WARNING);
		}

		// Smooth increase or decrease the engine speed
		if(throttleManualControl == TRUE) {
			float manualControlThrottle;
			ManualControlCommandThrottleGet(&manualControlThrottle);
			stabDesired.Throttle = manualControlThrottle;
		} else {
			ThrottleStep = dT/ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_DAMPER];
			if (fabs(ThrottleJob - stabDesired.Throttle) > ThrottleStep)
				stabDesired.Throttle += (ThrottleJob > stabDesired.Throttle) ? ThrottleStep : -ThrottleStep;
			else stabDesired.Throttle = ThrottleJob;

			stabDesired.Throttle = bound(
				stabDesired.Throttle,
				ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MIN],
				ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MAX]
				);
		}
		StabilizationDesiredSet(&stabDesired);

	}
}

/**
 * Bound input value between limits
 */

static float bound(float val, float min, float max)
{
	if (val < min) {
		val = min;
	} else if (val > max) {
		val = max;
	}
	return val;
}

/**
 * Expect updates position
 */
static void PositionActualUpdatedCb(UAVObjEvent * objEv)
{
	PositionActualNew_flag=true;
}
