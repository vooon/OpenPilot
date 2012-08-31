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
#include "attitudeactual.h"
#include "hwsettings.h"
#include "homelocation.h"

// Private constants
#define STACK_SIZE_BYTES 720
#define TASK_PRIORITY (tskIDLE_PRIORITY+1)
#define RAD2DEG (180.0/M_PI)
#define DEG2RAD (M_PI/180.0)
#define DELAY_ENABLE_FAILSAFE			4000	// In the event of a transmitter signal loss, delay before returning, in[ms]
#define SIMPLE_COURSE_CALCULATION				// A simplified calculation of the rate.
#define SPEEDTOBASE_TRACK_ENABLE		1		// at a rate less than this, the flight enabled tacks.
// The coefficient for the calculation of the maximum deviation range of the elevator to climb,
// depending on the speed of flight in the direction of the base.
// At a speed equal to ccguidanceSettings.GroundSpeedMinForCorrectBiasYaw,
// the elevator is set to ccguidanceSettings.Pitch [CCGUIDANCESETTINGS_PITCH_NEUTRAL]
#define SPEEDTOBASE_PITH_TO_NEUTRAL_KP	12

// Private types
static struct Integral {
	float EnergyError;
	float GroundSpeedError;
	float AltitudeError;
} *integral;

// Private variables
static uint8_t positionHoldLast;
static xTaskHandle ccguidanceTaskHandle;
static bool gpsNew_flag = false;
static bool CCGuidanceEnabled = false;

// Private functions
static void GPSPositionUpdatedCb(UAVObjEvent * objEv);
static void ccguidanceTask(void *parameters);
static float bound(float val, float min, float max);
static float meanDiffHeadingYaw(float diffHeadingYaw, bool firstMeanDiffHeadingYaw);
#if !defined(SIMPLE_COURSE_CALCULATION)
static float sphereDistance(float lat1,float long1,float lat2,float long2);
static float sphereCourse(float lat1,float long1,float lat2,float long2, float zeta);
#endif

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
	HomeLocationInitialize();
	HwSettingsInitialize();
	SystemSettingsInitialize();

	uint8_t optionalModules[HWSETTINGS_OPTIONALMODULES_NUMELEM];
	gpsNew_flag=false;

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
		GPSPositionInitialize();

		integral = (struct Integral *) pvPortMalloc(sizeof(struct Integral));
		memset(integral, 0, sizeof(struct Integral));

		GPSPositionConnectCallback(&GPSPositionUpdatedCb);

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
	ManualControlCommandData manualControl;
	FlightStatusData flightStatus;
	AttitudeActualData attitudeActual;
	StabilizationDesiredData stabDesired;
	GPSPositionData positionActual;
	HomeLocationData HomeLocation;

	portTickType lastUpdateTime;
	float positionDesiredLat    = 0.0f;
	float positionDesiredLon    = 0.0f;
	float positionDesiredAlt    = 0.0f;
	float diffHeadingYaw        = 0.0f;
	float DistanceToBaseOld     = 0.0f;
	float SpeedToRTB            = 0.0f;
	float positionActualLat     = 0.0f;
	float positionActualLon     = 0.0f;
	float course                = 0.0f;
	float DistanceToBase        = 0.0f;
	float ThrottleStep          = 0.0f;
	float ThrottleJob           = 0.0f;
	float dT                    = 0.0f;
	float EnergyError           = 0.0f;
	float GroundSpeedError      = 0.0f;
	float GroundSpeedActual     = 0.0f;
	float AltitudeError         = 0.0f;
	float dT_gps                = 0.0f;

	uint32_t thisTimesPeriodCorrectBiasYaw = 0;
	uint32_t TimeEnableFailSafe            = 0;
	uint8_t  TacksNumsRemain               = 0;

	bool firstRunSetCourse              = TRUE;
	bool StateSaveCurrentPositionToRTB  = FALSE;
	bool fixedHeading                   = TRUE;
	bool TacksAngleRight                = FALSE;
	bool firstMeanDiffHeadingYaw        = TRUE;
	bool Guidance_Run                   = FALSE;
	bool Throttle_manualControl         = TRUE;

// length of one degree of latitude parallel to the current, meters
	const float degreeLatLength = 40000000.0f/360.0f;
#if defined(SIMPLE_COURSE_CALCULATION)
	// length of one degree of longitude, meters
	float degreeLonLength		= 0.0f;
	float DistanceToBaseNorth	= 0.0f;
	float DistanceToBaseEast	= 0.0f;
#endif

	// Main task loop
	lastUpdateTime = xTaskGetTickCount();
	while (1) {
		CCGuidanceSettingsGet(&ccguidanceSettings);

		vTaskDelayUntil(&lastUpdateTime, ccguidanceSettings.UpdatePeriod / portTICK_RATE_MS);

		dT = (float)ccguidanceSettings.UpdatePeriod * 0.001f;				//change from milliseconds to seconds.
		dT_gps += dT;
		thisTimesPeriodCorrectBiasYaw += ccguidanceSettings.UpdatePeriod;

		StabilizationDesiredGet(&stabDesired);

		if( gpsNew_flag == true){
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
			Guidance_Run = (PARSE_FLIGHT_MODE(flightStatus.FlightMode) == FLIGHTMODE_GUIDANCE);
			if (Guidance_Run == TRUE) {

				GPSPositionGet(&positionActual);
				HomeLocationGet(&HomeLocation);

				if(positionHoldLast != 1 && (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD2 ||
				  (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE && HomeLocation.Set == FALSE) ||
				  (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_FLIGHTTOTARGET && ccguidanceSettings.TargetLocationSet == FALSE)))
				  {
					/* When entering position hold mode, use current position as set point*/
					positionDesiredLat = positionActual.Latitude * 1e-7;
					positionDesiredLon = positionActual.Longitude * 1e-7;
					positionDesiredAlt = positionActual.Altitude + positionActual.GeoidSeparation + 1;
					positionHoldLast = 1;
					firstRunSetCourse = TRUE;
		#if defined(SIMPLE_COURSE_CALCULATION)
					degreeLonLength = degreeLatLength * cos(positionDesiredLat * DEG2RAD);
		#endif
				} else if (positionHoldLast != 2 && (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE && HomeLocation.Set == TRUE)) {
					/* When we RTB, safe home position */
					positionDesiredLat = HomeLocation.Latitude * 1e-7;
					positionDesiredLon = HomeLocation.Longitude * 1e-7;
					positionDesiredAlt = HomeLocation.Altitude + ccguidanceSettings.ReturnTobaseAltitudeOffset ;
					positionHoldLast = 2;
					firstRunSetCourse = TRUE;
		#if defined(SIMPLE_COURSE_CALCULATION)
					degreeLonLength = degreeLatLength * cos(positionDesiredLat * DEG2RAD);
		#endif
				} else 	if (positionHoldLast != 3 &&
					((flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_FLIGHTTOTARGET) &&
					( HomeLocation.Set == TRUE))) {
					/* Sets target position */
					positionDesiredLat = ccguidanceSettings.TargetLocationLatitude * 1e-7;
					positionDesiredLon = ccguidanceSettings.TargetLocationLongitude * 1e-7;
					positionDesiredAlt = ccguidanceSettings.TargetLocationAltitude + ccguidanceSettings.ReturnTobaseAltitudeOffset ;
					positionHoldLast = 3;
					firstRunSetCourse = TRUE;
		#if defined(SIMPLE_COURSE_CALCULATION)
					degreeLonLength = degreeLatLength * cos(positionDesiredLat * DEG2RAD);
		#endif
				}

				stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_ROLL] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
				stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_PITCH] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
				stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_YAW] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;

				/* safety */
				if (positionActual.Status==GPSPOSITION_STATUS_FIX3D) {
					/* main position hold loop */
					// Calculation of the rate after the turn and the beginning of motion in a straight line.
					// Hold the current direction of flight
					if ((fixedHeading == TRUE) || (firstRunSetCourse == TRUE)) {
						AttitudeActualGet(&attitudeActual);
						// First activate RTB
						if (firstRunSetCourse == TRUE) {
							firstRunSetCourse = FALSE;
							// Save current location to HomeLocation if flightStatus = DISARMED
							if (flightStatus.Armed == FLIGHTSTATUS_ARMED_DISARMED &&
								StateSaveCurrentPositionToRTB == FALSE) {
								HomeLocation.Latitude = positionActual.Latitude;
								HomeLocation.Longitude = positionActual.Longitude;
								HomeLocation.Altitude = positionActual.Altitude;
								HomeLocation.Set = TRUE;
								HomeLocationSet(&HomeLocation);
								positionHoldLast = 0;
							}

							StateSaveCurrentPositionToRTB = !ccguidanceSettings.HomeLocationEnableRequestSet;
							if (stabDesired.Throttle < ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MIN]) stabDesired.Throttle = ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MIN];
							stabDesired.Yaw = attitudeActual.Yaw;

							// reset globals variable
							fixedHeading = TRUE;
							SpeedToRTB = positionActual.Groundspeed;
							TacksAngleRight = FALSE;
							TacksNumsRemain = 0;
							thisTimesPeriodCorrectBiasYaw = 0;
							DistanceToBaseOld = 0;
							firstMeanDiffHeadingYaw = TRUE;
						}

						// Calculation errors between the rate of the gyroscope and GPS at a speed not less than the minimum.
						if ((thisTimesPeriodCorrectBiasYaw >= ccguidanceSettings.PeriodCorrectBiasYaw) &&
							positionActual.Groundspeed >= ccguidanceSettings.GroundSpeedMinForCorrectBiasYaw) {

							if (ccguidanceSettings.EnableCorrectBiasYaw == TRUE) {
								diffHeadingYaw = attitudeActual.Yaw - positionActual.Heading;
								while (diffHeadingYaw<-180.) diffHeadingYaw+=360.;
								while (diffHeadingYaw>180.)  diffHeadingYaw-=360.;
								diffHeadingYaw = meanDiffHeadingYaw(diffHeadingYaw, firstMeanDiffHeadingYaw);
								firstMeanDiffHeadingYaw = FALSE;
							} else
								diffHeadingYaw = 0;

							// Ends hold the current direction of flight
							fixedHeading = FALSE;
						}

					} else {

						/* 1. Calculate course */
						positionActualLat = positionActual.Latitude * 1e-7;
						positionActualLon  = positionActual.Longitude * 1e-7;
						// calculate course to target
		#if defined(SIMPLE_COURSE_CALCULATION)
						// calculation Simple method of the course
						// calculation of rectangular coordinates
						DistanceToBaseNorth = (positionDesiredLat - positionActualLat) * degreeLatLength;
						DistanceToBaseEast = (positionDesiredLon - positionActualLon) * degreeLonLength;
						// square of the distance to the base
						DistanceToBase = sqrt((DistanceToBaseNorth * DistanceToBaseNorth) + (DistanceToBaseEast * DistanceToBaseEast));
						// true direction of the base
						course = atan2(DistanceToBaseEast, DistanceToBaseNorth) * RAD2DEG;
		#else
						// calculation method of the shortest course
						DistanceToBase = sphereDistance(
							positionActualLat,
							positionActualLon,
							positionDesiredLat,
							positionDesiredLon
						);
						course = sphereCourse(
							positionActualLat,
							positionActualLon,
							positionDesiredLat,
							positionDesiredLon,
							DistanceToBase
						);
						DistanceToBase = abs(DistanceToBase * degreeLatLength);
		#endif
						if (isnan(course)) course=0;

						// Correct course for set stabDesired.Yaw
						course += diffHeadingYaw;
						if (DistanceToBase > ccguidanceSettings.RadiusBase) {
							if (DistanceToBaseOld != 0) {
								// Calculation ground speed to base
								SpeedToRTB = ((DistanceToBaseOld - DistanceToBase) / thisTimesPeriodCorrectBiasYaw) * 1000;
							}
							DistanceToBaseOld = DistanceToBase;

							// algorithm flight tacks
							if (ccguidanceSettings.TacksNums != 0) {
								// TacksNumsRemain -- number of periods  PeriodCorrectBiasYaw, during this time flown tack, left or right.
								// If the number is 0 and the speed of the database is less than acceptable.
								if ((TacksNumsRemain == 0) && (SpeedToRTB < SPEEDTOBASE_TRACK_ENABLE)) {
									TacksNumsRemain = ccguidanceSettings.TacksNums;
									TacksAngleRight = !TacksAngleRight;	// Changing tack.
								}
								if (TacksNumsRemain > 0) {
									TacksNumsRemain--;
									// Adjust the rate for set tack.
									if (TacksAngleRight == TRUE)	course += ccguidanceSettings.TacksAngle;	// Right track
									else	course -= ccguidanceSettings.TacksAngle;							// Left track
								}
							}

							while (course<-180.) course+=360.;
							while (course>180.)  course-=360.;
							stabDesired.Yaw = course; 	// Set new course
							fixedHeading = TRUE;		// Turn on the retention of the new direction of flight.
						} else {
							/* Circular motion in the area between RadiusBase */
							if (ccguidanceSettings.CircleAroundBase == TRUE) {
								if (ccguidanceSettings.CircleAroundBaseAngle == 0.0f) {
									course += bound((180.0f-180.0f * (DistanceToBase / ccguidanceSettings.RadiusBase)), 0.0f, 100.0f);
								} else course += ccguidanceSettings.CircleAroundBaseAngle;
										//course += acos(DistanceToBase / ccguidanceSettings.RadiusBase) * RAD2DEG;
								while (course<-180.) course+=360.;
								while (course>180.)  course-=360.;
								stabDesired.Yaw = course;	//Set new course
							}
							// reset variable in radius base.
							SpeedToRTB = positionActual.Groundspeed;
							TacksAngleRight = FALSE;
							TacksNumsRemain = 0;
							DistanceToBaseOld = 0;
						}
						thisTimesPeriodCorrectBiasYaw = 0;
					}

					/* Compute AltitudeError, GroundSpeedActual, GroundSpeedError, EnergyError. */
					AltitudeError = (positionDesiredAlt - positionActual.Altitude + positionActual.GeoidSeparation);
					GroundSpeedActual = (SpeedToRTB > ccguidanceSettings.GroundSpeedMinForCorrectBiasYaw) ? positionActual.Groundspeed : 0;
					GroundSpeedError = ccguidanceSettings.GroundSpeedCruise - GroundSpeedActual;
					EnergyError = (ccguidanceSettings.GroundSpeedCruise * ccguidanceSettings.GroundSpeedCruise +
								positionDesiredAlt * ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_GRAV2KP]) -
								(GroundSpeedActual * GroundSpeedActual +
								(positionActual.Altitude + positionActual.GeoidSeparation) * ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_GRAV2KP]);

					/* 2. Pitch */
					if (ccguidanceSettings.SpeedControlThePitch == TRUE) {
						if (ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KI] > 0.0f){
							//Integrate with saturation
							integral->GroundSpeedError=bound(integral->GroundSpeedError + GroundSpeedError * dT_gps,
														-ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_ILIMIT] /
														ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KI],
														ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_ILIMIT] /
														ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KI]);
						}
						stabDesired.Pitch = bound(
							-(GroundSpeedError * ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KP] +
							integral->GroundSpeedError * ccguidanceSettings.PitchPI[CCGUIDANCESETTINGS_PITCHPI_KI]) +
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
							// if the velocity of the target is less than ccguidanceSettings.GroundSpeedMinForCorrectBiasYaw
							bound((SpeedToRTB - ccguidanceSettings.GroundSpeedMinForCorrectBiasYaw) *
									SPEEDTOBASE_PITH_TO_NEUTRAL_KP,
									ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_NEUTRAL],
									ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_CLIMB])
							);
					}
					/* 3. Throttle */
					//Integrate with bound. Make integral leaky for better performance. Approximately 30s time constant.
					if (ccguidanceSettings.ThrottleControl == TRUE) {
						if (ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MAX] > 1) ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MAX] = 1;
						if (ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MIN] < 0) ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MIN] = 0;

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

						Throttle_manualControl = FALSE;
					} else {
						/* Throttle (manual) */
						Throttle_manualControl = TRUE;
					}

					stabDesired.Roll = ccguidanceSettings.Roll[CCGUIDANCESETTINGS_ROLL_NEUTRAL];
					AlarmsClear(SYSTEMALARMS_ALARM_GUIDANCE);

				} else {
					/* Fallback, no position data! */
					stabDesired.Yaw = ccguidanceSettings.Yaw;
					stabDesired.Pitch = ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_CLIMB];
					stabDesired.Roll = ccguidanceSettings.Roll[CCGUIDANCESETTINGS_ROLL_MAX];
					stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_YAW] = STABILIZATIONDESIRED_STABILIZATIONMODE_RATE;
					/* Throttle (manual) */
					Throttle_manualControl = TRUE;
					AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_WARNING);
				}
				//StabilizationDesiredSet(&stabDesired);
			} else {
				// reset globals...
				positionHoldLast = 0;
				AlarmsClear(SYSTEMALARMS_ALARM_GUIDANCE);
			}

			gpsNew_flag = false;
			dT_gps = 0;
		}

		if (Guidance_Run == TRUE) {
			if(Throttle_manualControl == TRUE) {
				ManualControlCommandGet(&manualControl);
				stabDesired.Throttle = manualControl.Throttle;
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
 * Calculate the arithmetic mean of the DiffHeadingYaw
 */

static float meanDiffHeadingYaw(float diffHeadingYaw, bool firstMeanDiffHeadingYaw)
{
	static float diffHeadingYawOld;

	if (firstMeanDiffHeadingYaw == FALSE) {
		diffHeadingYaw = diffHeadingYawOld - diffHeadingYaw;

		if (diffHeadingYaw > 180.0f) diffHeadingYaw -=360.0f;
		else if (diffHeadingYaw < -180.0f) diffHeadingYaw +=360.0f;

		diffHeadingYaw = diffHeadingYawOld - diffHeadingYaw * 0.5f;

		if (diffHeadingYaw > 180.0f) diffHeadingYaw -=360.0f;
		else if (diffHeadingYaw < -180.0f) diffHeadingYaw +=360.0f;
	}
	diffHeadingYawOld = diffHeadingYaw;
	return diffHeadingYaw;
}



#if !defined(SIMPLE_COURSE_CALCULATION)
/**
 * calculate spherical distance and course between two coordinate pairs
 * see http://de.wikipedia.org/wiki/Orthodrome for details
 */
 static float sphereDistance(float lat1, float long1, float lat2, float long2)
{
	float zeta=(RAD2DEG * acos (
		sin(DEG2RAD*lat1) * sin(DEG2RAD*lat2)
		+ cos(DEG2RAD*lat1) * cos(DEG2RAD*lat2) * cos(DEG2RAD*(long2-long1))
	));
	if (isnan(zeta)) {
		zeta=0.0f;
	}
	return zeta;

}
static float sphereCourse(float lat1, float long1, float lat2, float long2, float zeta)
{
	float angle = RAD2DEG * acos(
			( sin(DEG2RAD*lat2) - sin(DEG2RAD*lat1) * cos(DEG2RAD*zeta) )
			/ ( cos(DEG2RAD*lat1) * sin(DEG2RAD*zeta) )
		);
	if (isnan(angle)) angle=0.0f;
	float diff=long2-long1;
	if (diff>180.0f) diff-=360.0f;
	if (diff<-180.0f) diff+=360.0f;
	if (diff>=0.0f) {
		return angle;
	} else {
		return -angle;
	}
}
#endif

static void GPSPositionUpdatedCb(UAVObjEvent * objEv)
{
	gpsNew_flag=true;
}
