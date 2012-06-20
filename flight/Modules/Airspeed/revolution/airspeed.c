/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{ 
 * @addtogroup AirspeedModule Airspeed Module
 * @brief Communicate with BMP085 and update @ref BaroAirspeed "BaroAirspeed UAV Object"
 * @{ 
 *
 * @file       airspeed.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Airspeed module, handles temperature and pressure readings from BMP085
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
 * Output object: BaroAirspeed
 *
 * This module will periodically update the value of the BaroAirspeed object.
 *
 */

#define AIRSPEED_STORED_IN_A_PROPER_UAVO FALSE //AIRSPEED SHOULD NOT GO INTO BaroAirspeed. IT IS A STATE VARIABLE, AND NOT A MEASUREMENT

#include "openpilot.h"
#include "hwsettings.h"
#include "airspeed.h"
#include "gpsvelocity.h"
#include "baroairspeed.h"	// object that will be updated by the module
#include "gps_airspeed.h"
#if (AIRSPEED_STORED_IN_A_PROPER_UAVO)
#include "velocityactual.h"
#endif
#include "attitudeactual.h"
#include "CoordinateConversions.h"

// Private constants
#if defined (PIOS_INCLUDE_GPS)
// #define STACK_SIZE_GPS 150
 #define GPS_AIRSPEED_PRESENT
#else
// #define STACK_SIZE_GPS 0
#endif

#if defined (PIOS_INCLUDE_MPXV7002) || defined (PIOS_INCLUDE_ETASV3)
// #define STACK_SIZE_BARO 550
 #define BARO_AIRSPEED_PRESENT
#else
// #define STACK_SIZE_BARO 0
#endif

#if defined (GPS_AIRSPEED_PRESENT) && defined (BARO_AIRSPEED_PRESENT)
 #define STACK_SIZE_BYTES 700 
#elif defined (GPS_AIRSPEED_PRESENT)
 #define STACK_SIZE_BYTES 600
#elif defined (BARO_AIRSPEED_PRESENT)
 #define STACK_SIZE_BYTES 550
#endif


#define TASK_PRIORITY (tskIDLE_PRIORITY+1)

#define SAMPLING_DELAY_MS_ETASV3                50     //Update at 20Hz
#define SAMPLING_DELAY_MS_MPXV7002              10	    //Update at 100Hz
#define SAMPLING_DELAY_MS_FALLTHROUGH           50     //Update at 20Hz. The fallthrough runs faster than the GPS to ensure that we don't miss GPS updates because we're slightly out of sync
#define ETS_AIRSPEED_SCALE                      1.0f   //???
#define CALIBRATION_IDLE_MS                     2000   //Time to wait before calibrating, in [ms]
#define CALIBRATION_COUNT_MS                    2000   //Time to spend calibrating, in [ms]
#define ANALOG_BARO_AIRSPEED_TIME_CONSTANT_MS   100.0f //Needs to be settable in a UAVO

#define GPS_AIRSPEED_BIAS_KP           0.01f   //Needs to be settable in a UAVO
#define GPS_AIRSPEED_BIAS_KI           0.01f   //Needs to be settable in a UAVO
#define SAMPLING_DELAY_MS_GPS          100    //Needs to be settable in a UAVO
#define GPS_AIRSPEED_TIME_CONSTANT_MS  500.0f //Needs to be settable in a UAVO

#define F_PI 3.141526535897932f
#define DEG2RAD (F_PI/180.0f)

// Private types

// Private variables
static xTaskHandle taskHandle;
static bool airspeedEnabled = false;
volatile bool gpsNew = false;

// Private functions
static void airspeedTask(void *parameters);

#ifdef GPS_AIRSPEED_PRESENT
static void GPSVelocityUpdatedCb(UAVObjEvent * ev);
#endif

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AirspeedStart()
{	
	//Check if module is enabled or not
	if (airspeedEnabled == false) {
		return -1;
	}
		
	// Start main task
	xTaskCreate(airspeedTask, (signed char *)"Airspeed", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &taskHandle);
	TaskMonitorAdd(TASKINFO_RUNNING_AIRSPEED, taskHandle);
	
	return 0;
}

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AirspeedInitialize()
{
#ifdef MODULE_AIRSPEED_BUILTIN
	airspeedEnabled = true;
#else
	
	HwSettingsInitialize();
	uint8_t optionalModules[HWSETTINGS_OPTIONALMODULES_NUMELEM];
	HwSettingsOptionalModulesGet(optionalModules);
	
	if (optionalModules[HWSETTINGS_OPTIONALMODULES_AIRSPEED] == HWSETTINGS_OPTIONALMODULES_ENABLED) {
		airspeedEnabled = true;
	} else {
		airspeedEnabled = false;
		return -1;
	}
#endif
	
	BaroAirspeedInitialize();
	
	return 0;
}
MODULE_INITCALL(AirspeedInitialize, AirspeedStart)


/**
 * Module thread, should not return.
 */
static void airspeedTask(void *parameters)
{
	
#ifdef BARO_AIRSPEED_PRESENT		
	BaroAirspeedData baroAirspeedData;
	BaroAirspeedGet(&baroAirspeedData);
	
	baroAirspeedData.Connected = BAROAIRSPEED_CONNECTED_FALSE;
	
	portTickType lastGPSTime= xTaskGetTickCount();

	float airspeedErrInt=0;
	
	//Calibration variables
	uint16_t calibrationCount = 0;
 #if defined(PIOS_INCLUDE_ETASV3)	
	uint32_t calibrationSum = 0;
 #endif	
 #if defined(PIOS_INCLUDE_MPXV7002)	
	uint16_t sensorCalibration;
 #endif	
#endif
	
	//GPS airspeed calculation variables
#ifdef GPS_AIRSPEED_PRESENT
	GPSVelocityConnectCallback(GPSVelocityUpdatedCb);	
	
	gps_airspeedInitialize();
#endif
	
	// Main task loop
	portTickType lastSysTime = xTaskGetTickCount();
	while (1)
	{
		float airspeed_cas;
		uint8_t airspeedSensorType;

#if (AIRSPEED_STORED_IN_A_PROPER_UAVO)		
		VelocityActualAirspeedGet(&airspeed_cas);		
#else
		BaroAirspeedAirspeedGet(&airspeed_cas);		
#endif		
		//Instead of getting the whole UAVO, just get the sensortype value
		BaroAirspeedAirspeedSensorTypeGet(&(airspeedSensorType)); //MAYBE THIS SHOULDN'T BE DONE EVERY LOOP, BUT ONCE A SECOND?
		
#ifdef BARO_AIRSPEED_PRESENT
		//Find out which sensor we're using.
		if (airspeedSensorType==BAROAIRSPEED_AIRSPEEDSENSORTYPE_DIYDRONESMPXV7002){	//DiyDrones MPXV7002
 #if defined(PIOS_INCLUDE_MPXV7002)
			//Wait until our turn
			vTaskDelayUntil(&lastSysTime, SAMPLING_DELAY_MS_MPXV7002 / portTICK_RATE_MS);
			
			// Update the airspeed
			BaroAirspeedGet(&baroAirspeedData);
			
			
			//Calibrate sensor by averaging zero point value //THIS SHOULD NOT BE DONE IF THERE IS AN IN-AIR RESET. HOW TO DETECT THIS?
			if (calibrationCount < CALIBRATION_IDLE_MS/SAMPLING_DELAY_MS_MPXV7002) { //First let sensor warm up and stabilize.
				calibrationCount++;
				continue;
			} else if (calibrationCount < (CALIBRATION_IDLE_MS + CALIBRATION_COUNT_MS)/SAMPLING_DELAY_MS_MPXV7002) { //Then compute the average.
				calibrationCount++; /*DO NOT MOVE FROM BEFORE sensorCalibration=... LINE, OR ELSE WILL HAVE DIVIDE BY ZERO */
				sensorCalibration=PIOS_MPXV7002_Calibrate(calibrationCount-CALIBRATION_IDLE_MS/SAMPLING_DELAY_MS_MPXV7002);
				
				baroAirspeedData.ZeroPoint=sensorCalibration;
				baroAirspeedData.Airspeed = 0;
				baroAirspeedData.Connected = BAROAIRSPEED_CONNECTED_TRUE;
				
				BaroAirspeedSet(&baroAirspeedData);
				continue;
			}
			else if (sensorCalibration!= baroAirspeedData.ZeroPoint){       //Finally, monitor the UAVO in case the user has manually changed the sensor calibration value.
				sensorCalibration=baroAirspeedData.ZeroPoint;
				PIOS_MPXV7002_UpdateCalibration(baroAirspeedData.ZeroPoint); //This makes sense for the user if the initial calibration was not good and the user does not wish to reboot.
			}
			
			//Get CAS
			float calibratedAirspeed = PIOS_MPXV7002_ReadAirspeed();
			if (calibratedAirspeed < 0) //This only occurs when there's a bad ADC reading. 
				continue;
			
			//Get sensor value, just for telemetry purposes. 
			//This is a silly waste of resources, and should probably be removed at some point in the future.
			//At this time, PIOS_MPXV7002_Measure() should become a static function and be removed from the header file.
			//
			//Moreover, due to the way the ADC driver is currently written, this code will return 0 more often than
			//not. This is something that will have to change on the ADC side of things.
			baroAirspeedData.SensorValue=PIOS_MPXV7002_Measure();
			
			//Filter CAS
			float alpha=SAMPLING_DELAY_MS_MPXV7002/(SAMPLING_DELAY_MS_MPXV7002 + ANALOG_BARO_AIRSPEED_TIME_CONSTANT_MS); //Low pass filter.
			float filteredAirspeed = calibratedAirspeed*(alpha) + baroAirspeedData.Airspeed*(1.0f-alpha);
			
			//Set two values, one for the UAVO airspeed sensor reading, and the other for the GPS corrected one
			airspeed_cas = filteredAirspeed;
			baroAirspeedData.Airspeed = filteredAirspeed;

 #endif
		}
		else if (airspeedSensorType==BAROAIRSPEED_AIRSPEEDSENSORTYPE_EAGLETREEAIRSPEEDV3){ //Eagletree Airspeed v3
 #if defined(PIOS_INCLUDE_ETASV3)
			//Wait until our turn
			vTaskDelayUntil(&lastSysTime, SAMPLING_DELAY_MS_ETASV3 / portTICK_RATE_MS);
			
			// Refresh the airspeed object
			BaroAirspeedGet(&baroAirspeedData);
			
			//Check to see if airspeed sensor is returning baroAirspeedData
			baroAirspeedData.SensorValue = PIOS_ETASV3_ReadAirspeed();
			if (baroAirspeedData.SensorValue==-1) {
				baroAirspeedData.Connected = BAROAIRSPEED_CONNECTED_FALSE;
				baroAirspeedData.Airspeed = 0;
				BaroAirspeedSet(&baroAirspeedData);
				continue;
			}
			
			//Calibrate sensor by averaging zero point value //THIS SHOULD NOT BE DONE IF THERE IS AN IN-AIR RESET. HOW TO DETECT THIS?
			if (calibrationCount < CALIBRATION_IDLE_MS/SAMPLING_DELAY_MS_ETASV3) {
				calibrationCount++;
				continue;
			} else if (calibrationCount < (CALIBRATION_IDLE_MS + CALIBRATION_COUNT_MS)/SAMPLING_DELAY_MS_ETASV3) {
				calibrationCount++;
				calibrationSum +=  baroAirspeedData.SensorValue;
				if (calibrationCount == (CALIBRATION_IDLE_MS + CALIBRATION_COUNT_MS)/SAMPLING_DELAY_MS_ETASV3) {
					baroAirspeedData.ZeroPoint = (uint16_t) (((float)calibrationSum) / CALIBRATION_COUNT_MS +0.5f);
				} else {
					continue;
				}
			}
			
			//Compute airspeed
			airspeed_cas = ETS_AIRSPEED_SCALE * sqrtf((float)abs(baroAirspeedData.SensorValue - baroAirspeedData.ZeroPoint)); //Is this calibrated or indicated airspeed?
			
			baroAirspeedData.Connected = BAROAIRSPEED_CONNECTED_TRUE;
			baroAirspeedData.Airspeed = airspeed_cas;
 #endif
		}
		else
#endif
		{ //Have to catch the fallthrough, or else this loop will monopolize the processor!
			//Likely, we have a GPS, so let's configure the fallthrough at close to GPS refresh rates
			vTaskDelayUntil(&lastSysTime, SAMPLING_DELAY_MS_FALLTHROUGH / portTICK_RATE_MS);
		}
		
		
		
#ifdef GPS_AIRSPEED_PRESENT
		float v_air_GPS=-1.0f;
		
		//Check if sufficient time has passed. This will depend on whether we have a pitot tube
		//sensor or not. In the case we do, shoot for about once per second. Otherwise, consume GPS
		//as quickly as possible.
 #ifdef BARO_AIRSPEED_PRESENT
		float delT = (lastSysTime - lastGPSTime)/1000.0f;
		if ( (delT > portTICK_RATE_MS	 || airspeedSensorType==BAROAIRSPEED_AIRSPEEDSENSORTYPE_GPSONLY)
				&& gpsNew) {
			lastGPSTime=lastSysTime;
 #else
		if (gpsNew)	{				
 #endif
			gpsNew=false; //Do this first

			//Calculate airspeed as a function of GPS groundspeed and vehicle attitude. From "IMU Wind Estimation (Theory)", by William Premerlani
			gps_airspeedGet(&v_air_GPS);			
		}
		
			
		//Use the GPS error to correct the airspeed estimate.
		// Most of the time, we won't have computed an airspeed via GPS, so skip this step.
		if (v_air_GPS > 0)
		{
			baroAirspeedData.GPSAirspeed=v_air_GPS;

 #ifdef BARO_AIRSPEED_PRESENT
			if(baroAirspeedData.Connected){ //Check if there is an airspeed sensors present...
				//Calculate error and error integral
				float airspeedErr=v_air_GPS - baroAirspeedData.TrueAirspeed;
				airspeedErrInt+=airspeedErr;
				
				float airspeedErrIntComponent = airspeedErrInt * GPS_AIRSPEED_BIAS_KI;

				//Saturate integral component at 5 m/s
				airspeedErrIntComponent = airspeedErrIntComponent >  5?  5: airspeedErrIntComponent;
				airspeedErrIntComponent = airspeedErrIntComponent < -5? -5: airspeedErrIntComponent;
							
				//There's already an airspeed sensor, so instead correct it for bias with PI correction
				airspeed_cas+=(airspeedErr * GPS_AIRSPEED_BIAS_KP + airspeedErrIntComponent)*delT;
			}
			else
 #endif
			{
				//...there's no airspeed sensor, so everything comes from GPS. In this
				//case, filter the airspeed for smoother output
				float alpha=SAMPLING_DELAY_MS_GPS/(SAMPLING_DELAY_MS_GPS + GPS_AIRSPEED_TIME_CONSTANT_MS); //Low pass filter.
				airspeed_cas=v_air_GPS*(alpha) + airspeed_cas*(1.0f-alpha);
			}			
		}
 #ifdef BARO_AIRSPEED_PRESENT
		else if(baroAirspeedData.Connected){
			//In the case of no current GPS data-- but with valid barometric airspeed sensor-- don't forget to still add in integral error
			airspeed_cas = baroAirspeedData.Airspeed + airspeedErrInt * GPS_AIRSPEED_BIAS_KI;
		}
 #endif
#else //No GPS support compiled. Just pass airspeed data straight through
		airspeed_cas = baroAirspeedData.Airspeed;
#endif
		
		// Update the airspeed related UAVObjects
#if (AIRSPEED_STORED_IN_A_PROPER_UAVO)
		VelocityActualAirspeedSet(&airspeed_cas);
 #ifdef BARO_AIRSPEED_PRESENT
		if (baroAirspeedData.Connected) 
				BaroAirspeedAirspeedSet(&airspeed_cas);
 #endif
#endif
		
		//Save GPS+Baro airspeed here
		baroAirspeedData.TrueAirspeed=airspeed_cas;
		
		//If we only have a GPS, use this data instead
		if (!baroAirspeedData.Connected)
			baroAirspeedData.Airspeed=airspeed_cas;
			
		//Set the UAVO
		BaroAirspeedSet(&baroAirspeedData);
			
	}
}

#ifdef GPS_AIRSPEED_PRESENT
static void GPSVelocityUpdatedCb(UAVObjEvent * ev)
{
	gpsNew=true;
}
#endif

/**
 * @}
 * @}
 */
