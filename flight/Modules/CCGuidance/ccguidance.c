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
 * Output object: AttitudeDesired
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

// Private constants
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

// Private variables
static uint8_t positionHoldLast;

// Private functions
static void ccguidanceTask(UAVObjEvent * ev);
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
	return 0;
}

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t CCGuidanceInitialize()
{
	bool CCGuidanceEnabled;
	uint8_t optionalModules[HWSETTINGS_OPTIONALMODULES_NUMELEM];

	HwSettingsInitialize();
	HwSettingsOptionalModulesGet(optionalModules);

	if (optionalModules[HWSETTINGS_OPTIONALMODULES_CCGUIDANCE] == HWSETTINGS_OPTIONALMODULES_ENABLED)
		CCGuidanceEnabled = true;
	else
		CCGuidanceEnabled = false;

	if (CCGuidanceEnabled) {

		CCGuidanceSettingsInitialize();
		GPSPositionInitialize();

		GPSPositionConnectCallback(&ccguidanceTask);

		// indicate error - can only run correctly if GPSPosition is ever set, which will change the alarm state.
		AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_CRITICAL);
		return 0;
	}
	return -1;
}
// no macro - optional module
MODULE_INITCALL(CCGuidanceInitialize, CCGuidanceStart)

/**
 * Main module callback.
 */
static void ccguidanceTask(UAVObjEvent * ev)
{
	SystemSettingsData systemSettings;
	CCGuidanceSettingsData ccguidanceSettings;
	ManualControlCommandData manualControl;
	FlightStatusData flightStatus;
	AttitudeActualData attitudeActual;
	portTickType thisTime;

	static portTickType lastUpdateTime = 0;
	static float positionDesiredNorth, positionDesiredEast, positionDesiredDown, diffHeadingYaw, DistanceToBaseOld, SpeedToRTB;
	float positionActualNorth = 0, positionActualEast = 0, course = 0, DistanceToBase = 0;
	static uint32_t thisTimesPeriodCorrectBiasYaw, TimeEnableFailSafe;
	static bool	firstRunSetCourse = TRUE, StateSaveCurrentPositionToRTB = FALSE, fixedHeading = TRUE, TacksAngleRight = FALSE, firstMeanDiffHeadingYaw = TRUE;
	float ThrottleStep = 0;
	static uint8_t TacksNumsRemain;

// length of one degree of latitude parallel to the current, meters
	const float degreeLatLength = 40000000.0f/360.0f;
#if defined(SIMPLE_COURSE_CALCULATION)
	// length of one degree of longitude, meters
	static float degreeLonLenght;
	float DistanceToBaseNorth, DistanceToBaseEast;
#endif

	CCGuidanceSettingsGet(&ccguidanceSettings);

	if (!lastUpdateTime) lastUpdateTime = xTaskGetTickCount();

	// Continue collecting data if not enough time
	thisTime = xTaskGetTickCount();
	if( (thisTime - lastUpdateTime) < (ccguidanceSettings.UpdatePeriod / portTICK_RATE_MS) ) return;

	thisTimesPeriodCorrectBiasYaw += (thisTime - lastUpdateTime) / portTICK_RATE_MS;

	FlightStatusGet(&flightStatus);
	SystemSettingsGet(&systemSettings);

	//Activate failsafe mode, activate return to base
	if ((AlarmsGet(SYSTEMALARMS_ALARM_MANUALCONTROL) != SYSTEMALARMS_ALARM_OK) &&
		(ccguidanceSettings.FailSafeRTB == TRUE))	{
		TimeEnableFailSafe += (thisTime - lastUpdateTime) / portTICK_RATE_MS;
		if (TimeEnableFailSafe >= DELAY_ENABLE_FAILSAFE) {
			TimeEnableFailSafe = 0;
			flightStatus.FlightMode = FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE;
			FlightStatusSet(&flightStatus);
		}
	} else TimeEnableFailSafe = 0;

	lastUpdateTime = thisTime;

	//Check that Return to Base is enabled 
	if ((PARSE_FLIGHT_MODE(flightStatus.FlightMode) == FLIGHTMODE_GUIDANCE) &&
		((systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWING) ||
		 (systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGELEVON) ||
		 (systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGVTAIL) ||
		 (systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_VTOL) ||
		 (systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_QUADP) ||
		 (systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_QUADX) ||
		 (systemSettings.AirframeType == SYSTEMSETTINGS_AIRFRAMETYPE_HEXA) ))
	{
		GPSPositionData positionActual;
		GPSPositionGet(&positionActual);

		if(positionHoldLast != 1 && (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD ||
		  (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE && ccguidanceSettings.HomeLocationSet == FALSE) ||
		  (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_FLIGHTTOTARGET && ccguidanceSettings.TargetLocationSet == FALSE)))
		  {
			/* When entering position hold mode, use current position as set point*/
			positionDesiredNorth = positionActual.Latitude * 1e-7;
			positionDesiredEast = positionActual.Longitude * 1e-7;
			positionDesiredDown = positionActual.Altitude + positionActual.GeoidSeparation + 1;
			positionHoldLast = 1;
			firstRunSetCourse = TRUE;
#if defined(SIMPLE_COURSE_CALCULATION)
			degreeLonLenght = degreeLatLength * cos(positionDesiredNorth * DEG2RAD);
#endif
		} else if (positionHoldLast != 2 && (flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE && ccguidanceSettings.HomeLocationSet == TRUE)) {
			/* When we RTB, use home position as setpoint */
			positionDesiredNorth = ccguidanceSettings.HomeLocationLatitude * 1e-7;
			positionDesiredEast = ccguidanceSettings.HomeLocationLongitude * 1e-7;
			positionDesiredDown = ccguidanceSettings.HomeLocationAltitude + ccguidanceSettings.ReturnTobaseAltitudeOffset ;
			positionHoldLast = 2;
			firstRunSetCourse = TRUE;
#if defined(SIMPLE_COURSE_CALCULATION)
			degreeLonLenght = degreeLatLength * cos(positionDesiredNorth * DEG2RAD);
#endif
		} else 	if (positionHoldLast != 3 &&
		  ((flightStatus.FlightMode == FLIGHTSTATUS_FLIGHTMODE_FLIGHTTOTARGET) &&
		  ( ccguidanceSettings.HomeLocationSet == TRUE))) 
		  {
			/* Use target position as setpoint */
			positionDesiredNorth = ccguidanceSettings.TargetLocationLatitude * 1e-7;
			positionDesiredEast = ccguidanceSettings.TargetLocationLongitude * 1e-7;
			positionDesiredDown = ccguidanceSettings.TargetLocationAltitude + ccguidanceSettings.ReturnTobaseAltitudeOffset ;
			positionHoldLast = 3;
			firstRunSetCourse = TRUE;
#if defined(SIMPLE_COURSE_CALCULATION)
			degreeLonLenght = degreeLatLength * cos(positionDesiredNorth * DEG2RAD);
#endif
		}

		StabilizationDesiredData stabDesired;
		StabilizationDesiredGet(&stabDesired);

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
						ccguidanceSettings.HomeLocationLatitude	= positionActual.Latitude;
						ccguidanceSettings.HomeLocationLongitude = positionActual.Longitude;
						ccguidanceSettings.HomeLocationAltitude = positionActual.Altitude;
						ccguidanceSettings.HomeLocationSet = TRUE;
						CCGuidanceSettingsSet(&ccguidanceSettings);
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

				// Calculate error between the gyroscope yaw and GPS heading. Only do this when speed is above the minimum threshold
				if ((thisTimesPeriodCorrectBiasYaw >= ccguidanceSettings.PeriodCorrectBiasYaw) &&
					positionActual.Groundspeed >= ccguidanceSettings.GroundSpeedMinForCorrectBiasYaw) {
					diffHeadingYaw = attitudeActual.Yaw - positionActual.Heading;
					while (diffHeadingYaw<-180.) diffHeadingYaw+=360.;
					while (diffHeadingYaw>180.)  diffHeadingYaw-=360.;
					// Ends hold the current direction of flight
					fixedHeading = FALSE;

					diffHeadingYaw = meanDiffHeadingYaw(diffHeadingYaw, firstMeanDiffHeadingYaw);
					firstMeanDiffHeadingYaw = FALSE;
				}

			} else {

				/* 1. Calculate course */
				positionActualNorth = positionActual.Latitude * 1e-7;
				positionActualEast  = positionActual.Longitude * 1e-7;
				// calculate course to target
#if defined(SIMPLE_COURSE_CALCULATION)
				// calculation Simple method of the course
				// calculation of rectangular coordinates
				DistanceToBaseNorth = (positionDesiredNorth - positionActualNorth) * degreeLatLength;
				DistanceToBaseEast = (positionDesiredEast - positionActualEast) * degreeLonLenght;
				// square of the distance to the base
				DistanceToBase = sqrt((DistanceToBaseNorth * DistanceToBaseNorth) + (DistanceToBaseEast * DistanceToBaseEast));
				// true direction of the base
				course = atan2(DistanceToBaseEast, DistanceToBaseNorth) * RAD2DEG;
#else
				// calculation method of the shortest course
				DistanceToBase = sphereDistance(
					positionActualNorth,
					positionActualEast,
					positionDesiredNorth,
					positionDesiredEast
				);
				course = sphereCourse(
					positionActualNorth,
					positionActualEast,
					positionDesiredNorth,
					positionDesiredEast,
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
						course += ccguidanceSettings.CircleAroundBaseAngle;
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

			/* 2. Altitude */
			stabDesired.Pitch = bound(
				((positionDesiredDown - positionActual.Altitude + positionActual.GeoidSeparation)
					* ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_KP]) + ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_NEUTRAL],
				ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_SINK],
				// decrease in the maximum deflection of the elevator to climb,
				// if the velocity of the target is less than ccguidanceSettings.GroundSpeedMinForCorrectBiasYaw
				bound((SpeedToRTB - ccguidanceSettings.GroundSpeedMinForCorrectBiasYaw) * SPEEDTOBASE_PITH_TO_NEUTRAL_KP,
						ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_NEUTRAL],
						ccguidanceSettings.Pitch[CCGUIDANCESETTINGS_PITCH_CLIMB])
				);

			/* 3. GroundSpeed */
			if (ccguidanceSettings.ThrottleControl == TRUE) {
				if (ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MAX] > 1) ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MAX] = 1;
				if (ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MIN] < 0) ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MIN] = 0;
				
				ThrottleStep = bound(
					(ccguidanceSettings.GroundSpeedMax - (positionActual.Groundspeed + SpeedToRTB)*0.5) * ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_KP],
					-ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_STEPMAX],
					ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_STEPMAX]
					);
				
				stabDesired.Throttle += ThrottleStep;
				stabDesired.Throttle = bound(
					stabDesired.Throttle,
					ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MIN],
					ccguidanceSettings.Throttle[CCGUIDANCESETTINGS_THROTTLE_MAX]
					);

			} else {
				/* Throttle (manual) */
				ManualControlCommandGet(&manualControl);
				stabDesired.Throttle = manualControl.Throttle;
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
			ManualControlCommandGet(&manualControl);
			stabDesired.Throttle = manualControl.Throttle;
			AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_WARNING);
		}
		StabilizationDesiredSet(&stabDesired);
	} else {
		// reset globals...
		positionHoldLast = 0;
		AlarmsClear(SYSTEMALARMS_ALARM_GUIDANCE);
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

		if (diffHeadingYaw > 180) diffHeadingYaw -=360;
		else if (diffHeadingYaw < -180) diffHeadingYaw +=360;

		diffHeadingYaw = diffHeadingYawOld - diffHeadingYaw * 0.5;

		if (diffHeadingYaw > 180) diffHeadingYaw -=360;
		else if (diffHeadingYaw < -180) diffHeadingYaw +=360;
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
		zeta=0;
	}
	return zeta;

}
static float sphereCourse(float lat1, float long1, float lat2, float long2, float zeta)
{
	float angle = RAD2DEG * acos(
			( sin(DEG2RAD*lat2) - sin(DEG2RAD*lat1) * cos(DEG2RAD*zeta) )
			/ ( cos(DEG2RAD*lat1) * sin(DEG2RAD*zeta) )
		);
	if (isnan(angle)) angle=0;
	float diff=long2-long1;
	if (diff>180) diff-=360;
	if (diff<-180) diff+=360;
	if (diff>=0) {
		return angle;
	} else {
		return -angle;
	}
}
#endif
