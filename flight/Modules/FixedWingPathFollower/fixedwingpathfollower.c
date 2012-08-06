/**
 ******************************************************************************
 *
 * @file       fixedwingpathfollower.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      This module compared @ref PositionActuatl to @ref ActiveWaypoint 
 * and sets @ref AttitudeDesired.  It only does this when the FlightMode field
 * of @ref ManualControlCommand is Auto.
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
 * Input object: ActiveWaypoint
 * Input object: PositionActual
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
#include "paths.h"

#include "accels.h"
#include "hwsettings.h"
#include "attitudeactual.h"
#include "pathdesired.h"	// object that will be updated by the module
#include "positionactual.h"
#include "manualcontrol.h"
#include "flightstatus.h"
#include "pathstatus.h"
#include "baroairspeed.h"
#include "gpsvelocity.h"
#include "gpsposition.h"
#include "fixedwingpathfollowersettings.h"
#include "fixedwingpathfollowerstatus.h"
#include "homelocation.h"
#include "nedposition.h"
#include "stabilizationdesired.h"
#include "stabilizationsettings.h"
#include "systemsettings.h"
#include "velocitydesired.h"
#include "velocityactual.h"
#include "CoordinateConversions.h"

// Private constants
#define MAX_QUEUE_SIZE 4
#define STACK_SIZE_BYTES 1548
#define TASK_PRIORITY (tskIDLE_PRIORITY+2)
#define F_PI 3.14159265358979323846f
#define RAD2DEG (180.0f/F_PI)
#define DEG2RAD (F_PI/180.0f)
#define GEE 9.805f
#define CRITICAL_ERROR_THRESHOLD_MS 5000 //Time in [ms] before an error becomes a critical error
// Private types

// Private variables
static bool followerEnabled = false;
static xTaskHandle pathfollowerTaskHandle;
static PathDesiredData pathDesired;
static PathStatusData pathStatus;
static FixedWingPathFollowerSettingsData fixedwingpathfollowerSettings;
static uint8_t flightMode=0;
static float pathLength;

// Private functions
static void pathfollowerTask(void *parameters);
static void FixedWingPathFollowerParamsUpdatedCb(UAVObjEvent * ev);
static void FlightStatusUpdatedCb(UAVObjEvent * ev);
static void updatePathVelocity();
static uint8_t updateFixedDesiredAttitude(uint8_t pathDesiredMode);
static void updateFixedAttitude();
//static void baroAirspeedUpdatedCb(UAVObjEvent * ev);
static float bound(float val, float min, float max);
static float followStraightLine(float r[3], float q[3], float p[3], float heading, float chi_inf, float k_path, float k_psi_int, float delT);
static float followOrbit(float c[3], float rho, bool direction, float p[3], float phi, float k_orbit, float k_psi_int, float delT);
static float calculateThrottle(float descentspeedError, float airspeedError, float dT, FixedWingPathFollowerStatusData *fixedwingpathfollowerStatus);

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t FixedWingPathFollowerStart()
{
	if (followerEnabled) {
		// Start main task
		xTaskCreate(pathfollowerTask, (signed char *)"PathFollower", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &pathfollowerTaskHandle);
		TaskMonitorAdd(TASKINFO_RUNNING_PATHFOLLOWER, pathfollowerTaskHandle);
	}

	return 0;
}

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t FixedWingPathFollowerInitialize()
{
	HwSettingsInitialize();
	uint8_t optionalModules[HWSETTINGS_OPTIONALMODULES_NUMELEM];
	HwSettingsOptionalModulesGet(optionalModules);
	if (optionalModules[HWSETTINGS_OPTIONALMODULES_FIXEDWINGPATHFOLLOWER] == HWSETTINGS_OPTIONALMODULES_ENABLED) {
		followerEnabled = true;
		FixedWingPathFollowerSettingsInitialize();
		FixedWingPathFollowerStatusInitialize();
		PathDesiredInitialize();
		PathStatusInitialize();
		VelocityDesiredInitialize();
		BaroAirspeedInitialize();
	} else {
		followerEnabled = false;
	}
	return 0;
}
MODULE_INITCALL(FixedWingPathFollowerInitialize, FixedWingPathFollowerStart)

static float northVelIntegral = 0;
static float eastVelIntegral = 0;
static float downVelIntegral = 0;

static float headingIntegral = 0;
static float speedIntegral = 0;
static float bearingIntegral = 0;
static float accelIntegral = 0;
static float powerIntegral = 0;
static float airspeedErrorInt=0;

static float lineErrorIntegral=0;
static float circleErrorIntegral=0;

/**
 * Module thread, should not return.
 */
static void pathfollowerTask(void *parameters)
{
	SystemSettingsData systemSettings;
	FlightStatusData flightStatus;
	
	portTickType lastUpdateTime;
	
//	BaroAirspeedConnectCallback(baroAirspeedUpdatedCb);
	
	FlightStatusConnectCallback(FlightStatusUpdatedCb);
	FixedWingPathFollowerSettingsConnectCallback(FixedWingPathFollowerParamsUpdatedCb);
	PathDesiredConnectCallback(FixedWingPathFollowerParamsUpdatedCb);
	
	FixedWingPathFollowerSettingsGet(&fixedwingpathfollowerSettings);
	PathDesiredGet(&pathDesired);
	
	static uint16_t errorCnt=0;
	
	// Main task loop
	lastUpdateTime = xTaskGetTickCount();
	while (1) {

		// Conditions when this runs:
		// 1. Must have FixedWing type airframe
		// 2. Flight mode is PositionHold and PathDesired.Mode is Endpoint  OR
		//    FlightMode is PathPlanner and PathDesired.Mode is Endpoint or Path

		SystemSettingsGet(&systemSettings);
		if ( (systemSettings.AirframeType != SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWING) &&
			(systemSettings.AirframeType != SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGELEVON) &&
			(systemSettings.AirframeType != SYSTEMSETTINGS_AIRFRAMETYPE_FIXEDWINGVTAIL) )
		{
			AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_WARNING);
			vTaskDelay(1000);
			continue;
		}

		// Continue collecting data if not enough time
		vTaskDelayUntil(&lastUpdateTime, fixedwingpathfollowerSettings.UpdatePeriod / portTICK_RATE_MS);

		
		FlightStatusGet(&flightStatus);
		PathStatusGet(&pathStatus);
	
		uint8_t result;
		// Check the combinations of flightmode and pathdesired mode
		switch(flightStatus.FlightMode) {
			case FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD:
			case FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE:
				if (pathDesired.Mode == PATHDESIRED_MODE_FLYENDPOINT) {
					updatePathVelocity();
					result = updateFixedDesiredAttitude(pathDesired.Mode);
					if (!result) {
						AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_OK);
						errorCnt=0;
					} else {
						if (errorCnt++ > CRITICAL_ERROR_THRESHOLD_MS/(fixedwingpathfollowerSettings.UpdatePeriod / portTICK_RATE_MS)) {
							AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_WARNING);
						}
					}
				} else {
					AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_ERROR);
				}
				break;
			case FLIGHTSTATUS_FLIGHTMODE_PATHPLANNER:
				pathStatus.UID = pathDesired.UID;
				pathStatus.Status = PATHSTATUS_STATUS_INPROGRESS;
				switch(pathDesired.Mode) {
					case PATHDESIRED_MODE_FLYENDPOINT:
					case PATHDESIRED_MODE_FLYVECTOR:
					case PATHDESIRED_MODE_FLYCIRCLERIGHT:
					case PATHDESIRED_MODE_FLYCIRCLELEFT:
						updatePathVelocity();
						result = updateFixedDesiredAttitude(pathDesired.Mode);
						if (!result) {
							AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_OK);
							errorCnt=0;
						} else {
							if (errorCnt++ > CRITICAL_ERROR_THRESHOLD_MS/(fixedwingpathfollowerSettings.UpdatePeriod / portTICK_RATE_MS)) {
								pathStatus.Status = PATHSTATUS_STATUS_CRITICAL;
								AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_WARNING);
							}
						}
						break;
					case PATHDESIRED_MODE_FIXEDATTITUDE:
						updateFixedAttitude(pathDesired.ModeParameters);
						AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_OK);
						break;
					case PATHDESIRED_MODE_DISARMALARM:
						AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_CRITICAL);
						break;
					default:
						pathStatus.Status = PATHSTATUS_STATUS_CRITICAL;
						AlarmsSet(SYSTEMALARMS_ALARM_GUIDANCE,SYSTEMALARMS_ALARM_ERROR);
						break;
				}
				break;
			default:
				// Be cleaner and get rid of global variables
				northVelIntegral = 0;
				eastVelIntegral = 0;
				downVelIntegral = 0;
				bearingIntegral = 0;
				headingIntegral = 0;
				speedIntegral = 0;
				accelIntegral = 0;
				powerIntegral = 0;

				break;
		}
		PathStatusSet(&pathStatus);
	}
}

/**
 * Compute desired velocity from the current position and path
 *
 * Takes in @ref PositionActual and compares it to @ref PathDesired 
 * and computes @ref VelocityDesired
 */
static void updatePathVelocity()
{
	PositionActualData positionActual;
	PositionActualGet(&positionActual);
	VelocityActualData velocityActual;
	VelocityActualGet(&velocityActual);

	// look ahead fixedwingpathfollowerSettings.HeadingFeedForward seconds
	float cur[3] = {positionActual.North + (velocityActual.North * fixedwingpathfollowerSettings.HeadingFeedForward),
					positionActual.East + (velocityActual.East * fixedwingpathfollowerSettings.HeadingFeedForward),
					positionActual.Down + (velocityActual.Down * fixedwingpathfollowerSettings.HeadingFeedForward)
					};
	struct path_status progress;

	path_progress(pathDesired.Start, pathDesired.End, cur, &progress, pathDesired.Mode);
	
	float groundspeed;
	float altitudeSetpoint;
	switch (pathDesired.Mode) {
		case PATHDESIRED_MODE_FLYCIRCLERIGHT:
		case PATHDESIRED_MODE_FLYCIRCLELEFT:
//			groundspeed = pathDesired.EndingVelocity;
			groundspeed = fixedwingpathfollowerSettings.BestClimbRateSpeed;
			altitudeSetpoint = pathDesired.End[2];
			break;
		case PATHDESIRED_MODE_FLYENDPOINT:
		case PATHDESIRED_MODE_FLYVECTOR:
		default:
//			groundspeed = pathDesired.StartingVelocity + (pathDesired.EndingVelocity - pathDesired.StartingVelocity) *
//			              bound(progress.fractional_progress,0,1);
			if (0) { //NOT GOOD. I DON'T THINK WE WANT OUR ALTITUDE SETPOINT TO BE DRIVEN BY OUR POSITION ESTIMATE. IT MEANS THAT WHEN THE POSITION ESTIMATE GETS NOISY, SO DOES THE ALTITUDE SETPOINT
				altitudeSetpoint = pathDesired.Start[2] + (pathDesired.End[2] - pathDesired.Start[2]) *
			                   bound(progress.fractional_progress,0,1);
			}
			else{
				//ANOTHER APPROACH IS TO GET TO THE ALTITUDE AS QUICKLY AS POSSIBLE.
				//PERHAPS A MORE CLASSIC APPROACH IS TO CALCULATE HOW QUICKLY WE SHOULD BE DESCENDING, AND TRY TO MAINTAIN THAT. BUT THAT'S NOT WHAT WE DO HERE
				altitudeSetpoint = pathDesired.End[2];
			}
			groundspeed = fixedwingpathfollowerSettings.BestClimbRateSpeed;
			break;
	}
	
	// calculate velocity
	VelocityDesiredData velocityDesired;
	velocityDesired.North = progress.path_direction[0] * fmaxf(groundspeed,1e-6);
	velocityDesired.East = progress.path_direction[1] * fmaxf(groundspeed,1e-6);
	

//	// calculate correction - can also be zero if correction vector is 0 or no error present
//	float error_speed = progress.error * fixedwingpathfollowerSettings.HorizontalPosP;
//	velocityDesired.North += progress.correction_direction[0] * error_speed;
//	velocityDesired.East += progress.correction_direction[1] * error_speed;
	
	float downError = altitudeSetpoint - positionActual.Down;
	velocityDesired.Down = downError * fixedwingpathfollowerSettings.VerticalPosP;

	// update pathstatus
	pathStatus.error = progress.error;
	pathStatus.fractional_progress = progress.fractional_progress;

	VelocityDesiredSet(&velocityDesired);
}


/**
 * Compute desired attitude from a fixed preset
 *
 */
static void updateFixedAttitude(float* attitude)
{
	StabilizationDesiredData stabDesired;
	StabilizationDesiredGet(&stabDesired);
	stabDesired.Roll     = attitude[0];
	stabDesired.Pitch    = attitude[1];
	stabDesired.Yaw      = attitude[2];
	stabDesired.Throttle = attitude[3];
	stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_ROLL] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
	stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_PITCH] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
	stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_YAW] = STABILIZATIONDESIRED_STABILIZATIONMODE_RATE;
	StabilizationDesiredSet(&stabDesired);
}

/**
 * Compute desired attitude from the desired velocity
 *
 * Takes in @ref NedActual which has the acceleration in the 
 * NED frame as the feedback term and then compares the 
 * @ref VelocityActual against the @ref VelocityDesired
 */
static uint8_t updateFixedDesiredAttitude(uint8_t pathDesiredMode)
{

	uint8_t result = 0;

	float dT = fixedwingpathfollowerSettings.UpdatePeriod / 1000.0f; //Convert from [ms] to [s]

	VelocityDesiredData velocityDesired;
	VelocityActualData velocityActual;
	StabilizationDesiredData stabDesired;
	AttitudeActualData attitudeActual;
	AccelsData accels;
	FixedWingPathFollowerSettingsData fixedwingpathfollowerSettings;
	StabilizationSettingsData stabSettings;
	FixedWingPathFollowerStatusData fixedwingpathfollowerStatus;
	BaroAirspeedData baroAirspeed;
	
	float calibratedAirspeedActual;
	float airspeedDesired;
	float airspeedError;
	float accelActual;
	float accelDesired;
	float accelError;
	float accelCommand;
	
	float pitchCommand;

	float descentspeedDesired;
	float descentspeedError;
//	float powerError;
	float powerCommand;

	float headingError_R;
	float rollCommand;

	FixedWingPathFollowerSettingsGet(&fixedwingpathfollowerSettings);

	FixedWingPathFollowerStatusGet(&fixedwingpathfollowerStatus);
	
	VelocityActualGet(&velocityActual);
//	VelocityDesiredGet(&velocityDesired);
	StabilizationDesiredGet(&stabDesired);
	VelocityDesiredGet(&velocityDesired);
	AttitudeActualGet(&attitudeActual);
	AccelsGet(&accels);
	StabilizationSettingsGet(&stabSettings);
	BaroAirspeedGet(&baroAirspeed);
	



	/**
	 * Compute speed error (required for throttle and pitch)
	 */

	// Current groundspeed
//	float groundspeedActual = sqrtf( velocityActual.East*velocityActual.East + velocityActual.North*velocityActual.North );

	// Current airspeed
	calibratedAirspeedActual = baroAirspeed.CalibratedAirspeed;

	// Current heading and bearing
/*	float bearingActual_R;
	{
		float q[4];
		float Rbe[3][3];
		// Get the current attitude estimate
		quat_copy(&attitudeActual.q1, q);

		Quaternion2R(q, Rbe);
		bearingActual_R=atan2f(Rbe[1][0],Rbe[0][0]);
	}
*/
	float headingActual_R = atan2f(velocityActual.East, velocityActual.North);

	// Desired airspeed
	airspeedDesired=fixedwingpathfollowerSettings.BestClimbRateSpeed;
	

	// Airspeed error
	airspeedError = airspeedDesired - calibratedAirspeedActual;

	// Vertical speed error
	descentspeedDesired = bound (
						velocityDesired.Down,
						-fixedwingpathfollowerSettings.VerticalVelMax,
						fixedwingpathfollowerSettings.VerticalVelMax);
	descentspeedError = descentspeedDesired - velocityActual.Down;

	// Error condition: wind speed is higher than maximum allowed speed. We are forced backwards!
	//COMMENT THIS OUT UNTIL BETTER WAY IS FOUND TO DETECT TRUE WINDSPEED. DEPEND ON USER TO KNOW WHEN WIND IS TOO HIGH.
	fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_WIND] = 0;
/*	if ( groundspeedDesired + groundspeedActual - baroAirspeed.Airspeed  <= 0 ) {
		fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_WIND] = 1;
		result = 1;
	}
*/ 
	// Error condition: plane too slow or too fast
	fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_HIGHSPEED] = 0;
	fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_LOWSPEED] = 0;
	if ( calibratedAirspeedActual >  fixedwingpathfollowerSettings.AirSpeedMax) {
		fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_OVERSPEED] = 1;
		result = 1;
	}
	if ( calibratedAirspeedActual >  fixedwingpathfollowerSettings.CruiseSpeed * 1.2f) {
		fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_HIGHSPEED] = 1;
		result = 1;
	}
	if (calibratedAirspeedActual < fixedwingpathfollowerSettings.BestClimbRateSpeed * 0.8f && 1) { //The next three && 1 are placeholders for UAVOs representing LANDING and TAKEOFF
		fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_LOWSPEED] = 1;
		result = 1;
	}
	if (calibratedAirspeedActual < fixedwingpathfollowerSettings.StallSpeedClean && 1 && 1) { //Where the && 1 represents the UAVO that will control whether the airplane is prepped for landing or not
		fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_STALLSPEED] = 1;
		result = 1;
	}
	if (calibratedAirspeedActual < fixedwingpathfollowerSettings.StallSpeedDirty && 1) {
		fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_STALLSPEED] = 1;
		result = 1;
	}
	
	if (calibratedAirspeedActual<1e-6) {
		// prevent division by zero, abort without controlling anything. This guidance mode is not suited for takeoff or touchdown, or handling stationary planes
		// also we cannot handle planes flying backwards, lets just wait until the nose drops
		fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_LOWSPEED] = 1;
		return 1;
	}

	/**
	 * Compute desired throttle command
	 */
	// compute proportional throttle response
	powerCommand=calculateThrottle(descentspeedError, airspeedError, dT, &fixedwingpathfollowerStatus);
/*	powerError = -descentspeedError +
		bound (
			 (airspeedError/fixedwingpathfollowerSettings.BestClimbRateSpeed)* fixedwingpathfollowerSettings.AirspeedToVerticalCrossFeed[FIXEDWINGPATHFOLLOWERSETTINGS_AIRSPEEDTOVERTICALCROSSFEED_KP] ,
			 -fixedwingpathfollowerSettings.AirspeedToVerticalCrossFeed[FIXEDWINGPATHFOLLOWERSETTINGS_AIRSPEEDTOVERTICALCROSSFEED_MAX],
			 fixedwingpathfollowerSettings.AirspeedToVerticalCrossFeed[FIXEDWINGPATHFOLLOWERSETTINGS_AIRSPEEDTOVERTICALCROSSFEED_MAX]
			 );
	
	// compute saturated integral error throttle response. Make integral leaky for better performance. Approximately 30s time constant.
	if (fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI] >0) {
		powerIntegral =	bound(powerIntegral + -descentspeedError * dT, 
										-fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_ILIMIT]/fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI],
										fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_ILIMIT]/fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI]
										)*(1.0f-1.0f/(1.0f+30.0f/dT));
	}
	
	// Compute final throttle response
	powerCommand = (powerError * fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KP] +
		powerIntegral*	fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI]) + fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_NEUTRAL];

	if (0) {
		//Saturate command, and reduce integral as a way of further avoiding integral windup
		if ( powerCommand > fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MAX]) {
			if (fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI] >0) {
				powerIntegral = bound(
					powerIntegral -
					( powerCommand
					- fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MAX]
					)/fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI],
					-fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_ILIMIT],
					fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_ILIMIT]);			
			}
			
			powerCommand = fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MAX];
		}
		if ( powerCommand < fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MIN]) {
			if (fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI] >0) {
				powerIntegral = bound(
					powerIntegral +
					( powerCommand 
					- fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MIN]
					)/fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI],
					-fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_ILIMIT],
					fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_ILIMIT]);
			}
		}
	}
	
	//Saturate throttle command.
	if ( powerCommand > fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MAX]) {
		powerCommand = fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MAX];
	}
	if ( powerCommand < fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MIN]) {
		powerCommand = fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MIN];
	}

	//Output internal state to telemetry
	fixedwingpathfollowerStatus.Error[FIXEDWINGPATHFOLLOWERSTATUS_ERROR_POWER] = powerError;
	fixedwingpathfollowerStatus.ErrorInt[FIXEDWINGPATHFOLLOWERSTATUS_ERRORINT_POWER] = powerIntegral;
	fixedwingpathfollowerStatus.Command[FIXEDWINGPATHFOLLOWERSTATUS_COMMAND_POWER] = powerCommand;
*/
	// set throttle
	stabDesired.Throttle = powerCommand;

	// Error condition: plane cannot hold altitude at current speed.
	fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_LOWPOWER] = 0;
	if (
		powerCommand == fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MAX] // throttle at maximum
		&& velocityActual.Down > 0 // we ARE going down
		&& descentspeedDesired < 0 // we WANT to go up
		&& airspeedError > 0 // we are too slow already
		) 
	{
		fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_LOWPOWER] = 1;
		result = 1;
	}
	// Error condition: plane keeps climbing despite minimum throttle (opposite of above)
	fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_HIGHPOWER] = 0;
	if (
		powerCommand == fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MIN] // throttle at minimum
		&& velocityActual.Down < 0 // we ARE going up
		&& descentspeedDesired > 0 // we WANT to go down
		&& airspeedError < 0 // we are too fast already
		) 
	{
		fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_HIGHPOWER] = 1;
		result = 1;
	}


	/**
	 * Compute desired pitch command
	 */
	// compute desired acceleration
	if(0){
		accelDesired = bound( (airspeedError/calibratedAirspeedActual) * fixedwingpathfollowerSettings.SpeedP[FIXEDWINGPATHFOLLOWERSETTINGS_SPEEDP_KP],
			-fixedwingpathfollowerSettings.SpeedP[FIXEDWINGPATHFOLLOWERSETTINGS_SPEEDP_MAX],
			fixedwingpathfollowerSettings.SpeedP[FIXEDWINGPATHFOLLOWERSETTINGS_SPEEDP_MAX]);
		fixedwingpathfollowerStatus.Error[FIXEDWINGPATHFOLLOWERSTATUS_ERROR_SPEED] = airspeedError;
		fixedwingpathfollowerStatus.ErrorInt[FIXEDWINGPATHFOLLOWERSTATUS_ERRORINT_SPEED] = 0.0f;
		fixedwingpathfollowerStatus.Command[FIXEDWINGPATHFOLLOWERSTATUS_COMMAND_SPEED] = accelDesired;

		// exclude gravity from acceleration. If the aicraft is flying at high pitch, it fights gravity without getting faster
		accelActual = accels.x - (sinf( DEG2RAD * attitudeActual.Pitch) * GEE);

		accelError = accelDesired - accelActual;
		if (fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_KI] > 0){
			accelIntegral = bound(accelIntegral + accelError * dT, 
				-fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_ILIMIT]/fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_KI],
				fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_ILIMIT]/fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_KI]);
		}
		
		accelCommand = (accelError * fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_KP] + 
			accelIntegral*fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_KI]);
	
		fixedwingpathfollowerStatus.Error[FIXEDWINGPATHFOLLOWERSTATUS_ERROR_ACCEL] = accelError;
		fixedwingpathfollowerStatus.ErrorInt[FIXEDWINGPATHFOLLOWERSTATUS_ERRORINT_ACCEL] = accelIntegral;
		fixedwingpathfollowerStatus.Command[FIXEDWINGPATHFOLLOWERSTATUS_COMMAND_ACCEL] = accelCommand;
		
		pitchCommand= -accelCommand + bound ( (-descentspeedError/calibratedAirspeedActual) * fixedwingpathfollowerSettings.VerticalToPitchCrossFeed[FIXEDWINGPATHFOLLOWERSETTINGS_VERTICALTOPITCHCROSSFEED_KP],
			-fixedwingpathfollowerSettings.VerticalToPitchCrossFeed[FIXEDWINGPATHFOLLOWERSETTINGS_VERTICALTOPITCHCROSSFEED_MAX],
			fixedwingpathfollowerSettings.VerticalToPitchCrossFeed[FIXEDWINGPATHFOLLOWERSETTINGS_VERTICALTOPITCHCROSSFEED_MAX]
			);
	}
	else {
		
		if (fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_KI] > 0){
			//Integrate with saturation
			airspeedErrorInt=bound(airspeedErrorInt + airspeedError * dT, 
					-fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_ILIMIT]/fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_KI],
					fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_ILIMIT]/fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_KI]);
		}				
		
		//Compute the cross feed from vertical speed to pitch, with saturation
		float verticalSpeedToPitchCommandComponent=bound (-descentspeedError * fixedwingpathfollowerSettings.VerticalToPitchCrossFeed[FIXEDWINGPATHFOLLOWERSETTINGS_VERTICALTOPITCHCROSSFEED_KP],
				 -fixedwingpathfollowerSettings.VerticalToPitchCrossFeed[FIXEDWINGPATHFOLLOWERSETTINGS_VERTICALTOPITCHCROSSFEED_MAX],
				 fixedwingpathfollowerSettings.VerticalToPitchCrossFeed[FIXEDWINGPATHFOLLOWERSETTINGS_VERTICALTOPITCHCROSSFEED_MAX]
				 );
		
		//Compute the pitch command as err*Kp + errInt*Ki + X_feed.
		pitchCommand= -(airspeedError*fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_KP] 
							 + airspeedErrorInt*fixedwingpathfollowerSettings.AccelPI[FIXEDWINGPATHFOLLOWERSETTINGS_ACCELPI_KI]
							 )	+ verticalSpeedToPitchCommandComponent;
		
		fixedwingpathfollowerStatus.Error[FIXEDWINGPATHFOLLOWERSTATUS_ERROR_SPEED] = airspeedError;
		fixedwingpathfollowerStatus.ErrorInt[FIXEDWINGPATHFOLLOWERSTATUS_ERRORINT_SPEED] = airspeedErrorInt;
		fixedwingpathfollowerStatus.Command[FIXEDWINGPATHFOLLOWERSTATUS_COMMAND_SPEED] = airspeedDesired;
		
		fixedwingpathfollowerStatus.Error[FIXEDWINGPATHFOLLOWERSTATUS_ERROR_ACCEL] = -123;
		fixedwingpathfollowerStatus.ErrorInt[FIXEDWINGPATHFOLLOWERSTATUS_ERRORINT_ACCEL] = -123;
		fixedwingpathfollowerStatus.Command[FIXEDWINGPATHFOLLOWERSTATUS_COMMAND_ACCEL] = pitchCommand;
		
	}

	stabDesired.Pitch = bound(fixedwingpathfollowerSettings.PitchLimit[FIXEDWINGPATHFOLLOWERSETTINGS_PITCHLIMIT_NEUTRAL] +
		pitchCommand,
		fixedwingpathfollowerSettings.PitchLimit[FIXEDWINGPATHFOLLOWERSETTINGS_PITCHLIMIT_MIN],
		fixedwingpathfollowerSettings.PitchLimit[FIXEDWINGPATHFOLLOWERSETTINGS_PITCHLIMIT_MAX]);

	// Error condition: high speed dive
	fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_PITCHCONTROL] = 0;
	if (
		pitchCommand == fixedwingpathfollowerSettings.PitchLimit[FIXEDWINGPATHFOLLOWERSETTINGS_PITCHLIMIT_MAX] // pitch demand is full up
		&& velocityActual.Down > 0 // we ARE going down
		&& descentspeedDesired < 0 // we WANT to go up
		&& airspeedError < 0 // we are too fast already
		) {
		fixedwingpathfollowerStatus.Errors[FIXEDWINGPATHFOLLOWERSTATUS_ERRORS_PITCHCONTROL] = 1;
		result = 1;
	}


	/**
	 * Compute desired roll command
	 */
	
	PositionActualData positionActual;
	PositionActualGet(&positionActual);
	
	float p[3]={positionActual.North, positionActual.East, positionActual.Down};
	float *c = pathDesired.End;
	float *r = pathDesired.Start;
	float q[3] = {pathDesired.End[0]-pathDesired.Start[0], pathDesired.End[1]-pathDesired.Start[1], pathDesired.End[2]-pathDesired.Start[2]};
	
//========================================	
	float k_path  = 0.05f;
	float k_orbit = 0.05f;
	
	float k_psi_int = 0.0002f;
	bool direction;
	
	float chi_inf=F_PI/4.0; //THIS NEEDS TO BE A FUNCTION OF HOW LONG OUR PATH IS.
//========================================	
	
	float rho;
	
	//Saturate chi_inf. I.e., never approach the path at a steeper angle than 45 degrees
	chi_inf= chi_inf < F_PI/4.0f? F_PI/4.0f: chi_inf; 
	
	float rollFF;
	float headingCommand_R;
	
	float pncn=p[0]-c[0];
	float pece=p[1]-c[1];
	float d=sqrtf(pncn*pncn + pece*pece);

#define ROLL_FOR_HOLDING_CIRCLE 15.0f	 //Assume that we want a 15 degree bank angle. This should yield a nice, non-agressive turn	
		
	//Calculate radius, rho, using r*omega=v and omega = g/V_g * tan(phi)
	//THIS SHOULD ONLY BE CALCULATED ONCE, INSTEAD OF EVERY TIME
	rho=powf(fixedwingpathfollowerSettings.BestClimbRateSpeed,2)/(9.805f*tanf(fabs(ROLL_FOR_HOLDING_CIRCLE*DEG2RAD)));
		
	typedef enum {
		LINE, 
		ORBIT
	} pathTypes_t;
		
	pathTypes_t pathType;

	//Determine if we should fly on a line or orbit path.
	switch (flightMode) {
		case FLIGHTSTATUS_FLIGHTMODE_RETURNTOBASE:
			//BAD. This should really be a one way function, where we never go back to line mode until the waypoint changes.
			if (d < rho+5.0f*fixedwingpathfollowerSettings.BestClimbRateSpeed) //When approx five seconds from the circle, start integrating into it
				pathType=ORBIT;
			else 
				pathType=LINE;
			break;
		case FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD:
			pathType=ORBIT;
			break;
		default:
			pathType=LINE;
			break;
	}
	
	//Check to see if we've gone past our destination. Since the path follower is simply following a vector, it has no concept of
	//where the vector ends. It will simply keep following it to infinity if we don't stop it.
	//So while we don't know why the commutation to the next point failed, we don't know we don't want the plane flying off.
	if (pathType==LINE) {
		//Perform a quick vector math operation, |a| < a.b/|a| = |b|cos(theta), to test if we've gone past the point. Add in a distance equal to 5s of flight time, for good measure to make sure we don't add jitter.
		if (((p[0]-r[0])*q[0]+(p[1]-r[1])*q[1]) > pow(pathLength,2)+5.0f*fixedwingpathfollowerSettings.BestClimbRateSpeed){
			//Whoops, we've really overflown our destination point, and haven't received any instructions. Start circling
			flightMode=FLIGHTSTATUS_FLIGHTMODE_POSITIONHOLD; //flightMode will reset the next time a waypoint changes, so there's no danger of it getting stuck in orbit mode.
			pathType=ORBIT;
		}
	}
	
	switch (pathType){
		case ORBIT:
			if(pathDesiredMode==PATHDESIRED_MODE_FLYCIRCLELEFT) {
				rollFF=-ROLL_FOR_HOLDING_CIRCLE*DEG2RAD;
				direction=false;
			}
			else {
				//In the case where the direction is undefined, always fly in a clockwise fashion
				direction=true;
				rollFF=ROLL_FOR_HOLDING_CIRCLE*DEG2RAD; 
			}
			
			headingCommand_R=followOrbit(c, rho, direction, p, headingActual_R, k_orbit, k_psi_int, dT);
			break;
		case LINE:
			rollFF=0;
			headingCommand_R=followStraightLine(r, q, p, headingActual_R, chi_inf, k_path, k_psi_int, dT);
			break;
	}
		
	if (airspeedDesired> 1e-6) {
		headingError_R = headingCommand_R-headingActual_R;
	} else {
		// if we are not supposed to move, keep going wherever we are now. Don't make things worse by changing direction.
		headingError_R = 0;
	}
		
	if (headingError_R<-F_PI) headingError_R+=2.0f*F_PI;
	if (headingError_R> F_PI) headingError_R-=2.0f*F_PI;
		
		
	//GET RID OF THE RAD2DEG. IT CAN BE FACTORED INTO BearingPI
	rollCommand = (/*rollFF*/ + headingError_R * fixedwingpathfollowerSettings.BearingPI[FIXEDWINGPATHFOLLOWERSETTINGS_BEARINGPI_KP])* RAD2DEG;
#ifdef SIM_OSX
	fprintf(stderr, " headingError_R: %f, rollCommand: %f\n", headingError_R, rollCommand);
#endif		
	//	fixedwingpathfollowerStatus.Error[FIXEDWINGPATHFOLLOWERSTATUS_ERROR_BEARING] = headingError_R;
	//	fixedwingpathfollowerStatus.ErrorInt[FIXEDWINGPATHFOLLOWERSTATUS_ERRORINT_BEARING] = -12345;
	//	fixedwingpathfollowerStatus.Command[FIXEDWINGPATHFOLLOWERSTATUS_COMMAND_BEARING] = rollCommand;
	
	fixedwingpathfollowerStatus.Error[FIXEDWINGPATHFOLLOWERSTATUS_ERROR_BEARING] = headingActual_R;
	fixedwingpathfollowerStatus.ErrorInt[FIXEDWINGPATHFOLLOWERSTATUS_ERRORINT_BEARING] = headingCommand_R;
	fixedwingpathfollowerStatus.Command[FIXEDWINGPATHFOLLOWERSTATUS_COMMAND_BEARING] = rollCommand;
	//Turn heading 
		
	stabDesired.Roll = bound( fixedwingpathfollowerSettings.RollLimit[FIXEDWINGPATHFOLLOWERSETTINGS_ROLLLIMIT_NEUTRAL] +
							 rollCommand,
							 fixedwingpathfollowerSettings.RollLimit[FIXEDWINGPATHFOLLOWERSETTINGS_ROLLLIMIT_MIN],
							 fixedwingpathfollowerSettings.RollLimit[FIXEDWINGPATHFOLLOWERSETTINGS_ROLLLIMIT_MAX] );

	// TODO: find a check to determine loss of directional control. Likely needs some check of derivative


	/**
	 * Compute desired yaw command
	 */
	// TODO implement raw control mode for yaw and base on Accels.X
	stabDesired.Yaw = 0;


	stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_ROLL] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
	stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_PITCH] = STABILIZATIONDESIRED_STABILIZATIONMODE_ATTITUDE;
	stabDesired.StabilizationMode[STABILIZATIONDESIRED_STABILIZATIONMODE_YAW] = STABILIZATIONDESIRED_STABILIZATIONMODE_NONE;
	
	StabilizationDesiredSet(&stabDesired);

	FixedWingPathFollowerStatusSet(&fixedwingpathfollowerStatus);

	return result;
}

float followStraightLine(float r[3], float q[3], float p[3], float psi, float chi_inf, float k_path, float k_psi_int, float delT){
	float chi_q=atan2f(q[1], q[0]);
	while (chi_q - psi < -F_PI) {
		chi_q+=2.0f*F_PI;
	}
	while (chi_q - psi > F_PI) {
		chi_q-=2.0f*F_PI;
	}
	
	float err_p=-sinf(chi_q)*(p[0]-r[0])+cosf(chi_q)*(p[1]-r[1]);
	lineErrorIntegral+=delT*err_p;
	float psi_command = chi_q-chi_inf*2.0f/F_PI*atanf(k_path*err_p)-k_psi_int*lineErrorIntegral*0;
	
	return psi_command;
}

float followOrbit(float c[3], float rho, bool direction, float p[3], float psi, float k_orbit, float k_psi_int, float delT){
	float pncn=p[0]-c[0];
	float pece=p[1]-c[1];
	float d=sqrtf(pncn*pncn + pece*pece);
	float phi=atan2f(pece, pncn);
	while(phi-psi < -F_PI){
		phi=phi+2.0f*F_PI;
	}
	while(phi-psi > F_PI){
		phi=phi-2.0f*F_PI;
	}
	
	float err_orbit=d-rho;
	circleErrorIntegral+=err_orbit*delT;
	
	float psi_command= direction==true? 
		phi+(F_PI/2.0f + atanf(k_orbit*err_orbit) + k_psi_int*circleErrorIntegral):
		phi-(F_PI/2.0f + atanf(k_orbit*err_orbit) + k_psi_int*circleErrorIntegral);

#ifdef SIM_OSX
	fprintf(stderr, "actual heading: %f, circle error: %f, circl integral: %f, heading command: %f", psi, err_orbit, circleErrorIntegral, psi_command);
#endif
	return psi_command;
}


float calculateThrottle(float descentspeedError, float airspeedError, float dT, FixedWingPathFollowerStatusData *fixedwingpathfollowerStatus){
	float powerCommand;
	float powerError = -descentspeedError +
	bound (
		   (airspeedError/fixedwingpathfollowerSettings.BestClimbRateSpeed)* fixedwingpathfollowerSettings.AirspeedToVerticalCrossFeed[FIXEDWINGPATHFOLLOWERSETTINGS_AIRSPEEDTOVERTICALCROSSFEED_KP] ,
		   -fixedwingpathfollowerSettings.AirspeedToVerticalCrossFeed[FIXEDWINGPATHFOLLOWERSETTINGS_AIRSPEEDTOVERTICALCROSSFEED_MAX],
		   fixedwingpathfollowerSettings.AirspeedToVerticalCrossFeed[FIXEDWINGPATHFOLLOWERSETTINGS_AIRSPEEDTOVERTICALCROSSFEED_MAX]
		   );
	
	// compute saturated integral error throttle response. Make integral leaky for better performance. Approximately 30s time constant.
	if (fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI] >0) {
		powerIntegral =	bound(powerIntegral + -descentspeedError * dT, 
							  -fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_ILIMIT]/fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI],
							  fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_ILIMIT]/fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI]
							  )*(1.0f-1.0f/(1.0f+30.0f/dT));
	}
	
	// Compute final throttle response
	powerCommand = (powerError * fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KP] +
					powerIntegral*	fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI]) + fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_NEUTRAL];
	
	if (0) {
		//Saturate command, and reduce integral as a way of further avoiding integral windup
		if ( powerCommand > fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MAX]) {
			if (fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI] >0) {
				powerIntegral = bound(
									  powerIntegral -
									  ( powerCommand
									   - fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MAX]
									   )/fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI],
									  -fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_ILIMIT],
									  fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_ILIMIT]);			
			}
			
			powerCommand = fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MAX];
		}
		if ( powerCommand < fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MIN]) {
			if (fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI] >0) {
				powerIntegral = bound(
									  powerIntegral +
									  ( powerCommand 
									   - fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MIN]
									   )/fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_KI],
									  -fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_ILIMIT],
									  fixedwingpathfollowerSettings.PowerPI[FIXEDWINGPATHFOLLOWERSETTINGS_POWERPI_ILIMIT]);
			}
		}
	}
	else {
		//Saturate throttle command.
		if ( powerCommand > fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MAX]) {
			powerCommand = fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MAX];
		}
		if ( powerCommand < fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MIN]) {
			powerCommand = fixedwingpathfollowerSettings.ThrottleLimit[FIXEDWINGPATHFOLLOWERSETTINGS_THROTTLELIMIT_MIN];
		}
	}
	
	fixedwingpathfollowerStatus->Error[FIXEDWINGPATHFOLLOWERSTATUS_ERROR_POWER] = powerError;
	fixedwingpathfollowerStatus->ErrorInt[FIXEDWINGPATHFOLLOWERSTATUS_ERRORINT_POWER] = powerIntegral;
	fixedwingpathfollowerStatus->Command[FIXEDWINGPATHFOLLOWERSTATUS_COMMAND_POWER] = powerCommand;
	
	
	return powerCommand;
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

//Triggered by changes in FixedWingPathFollowerSettings and PathDesired
static void FixedWingPathFollowerParamsUpdatedCb(UAVObjEvent * ev)
{
	FixedWingPathFollowerSettingsGet(&fixedwingpathfollowerSettings);
	FlightStatusFlightModeGet(&flightMode);
	PathDesiredGet(&pathDesired);
	
	float r[2] = {pathDesired.End[0]-pathDesired.Start[0], pathDesired.End[1]-pathDesired.Start[1]};
	pathLength=sqrtf(r[0]*r[0]+r[1]*r[1]);
	
}

static void FlightStatusUpdatedCb(UAVObjEvent * ev){
}
