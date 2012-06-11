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

#include "openpilot.h"
#include "hwsettings.h"
#include "airspeed.h"
#include "baroairspeed.h"	// object that will be updated by the module

// Private constants
#define STACK_SIZE_BYTES 500
#define TASK_PRIORITY (tskIDLE_PRIORITY+1)
#define SAMPLING_DELAY_MS_ETASV3 50 
#define SAMPLING_DELAY_MS_MPXV7002 1 
#define ETS_AIRSPEED_SCALE 1.0f
#define CALIBRATION_IDLE_MS  2000  //Time to wait before calibrating, in [s]
#define CALIBRATION_COUNT_MS 2000 //Time to spend calibrating, in [s]

// Private types

// Private variables
static xTaskHandle taskHandle;

// Private functions
static void airspeedTask(void *parameters);


static bool airspeedEnabled = false;

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t AirspeedStart()
{	
	
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
	BaroAirspeedData data;
	BaroAirspeedGet(&data);
	
	uint16_t calibrationCount = 0;
	uint32_t calibrationSum = 0;
	uint16_t sensorCalibration;
	
	
	// Main task loop
	while (1)
	{
		//Find out which sensor we're using.
		if (data.AirspeedSensorType==BAROAIRSPEED_AIRSPEEDSENSORTYPE_DIYDRONESMPXV7002){	//DiyDrones MPXV7002
#if defined(PIOS_INCLUDE_MPXV7002)
			//Wait until our turn
			vTaskDelay(SAMPLING_DELAY_MS_MPXV7002);
			

			// Update the airspeed
			BaroAirspeedGet(&data);
			
						
			//Calibrate sensor by averaging zero point value //THIS SHOULD NOT BE DONE IF THERE IS AN IN-AIR RESET. HOW TO DETECT THIS?
			if (calibrationCount < CALIBRATION_IDLE_MS/SAMPLING_DELAY_MS_MPXV7002) { //First let sensor warm up and stabilize.
				calibrationCount++;
				continue;
			} else if (calibrationCount < (CALIBRATION_IDLE_MS + CALIBRATION_COUNT_MS)/SAMPLING_DELAY_MS_MPXV7002) { //Then compute the average.
				calibrationCount++; /*DO NOT MOVE FROM BEFORE sensorCalibration=... LINE, OR ELSE WILL HAVE DIVIDE BY ZERO */
				sensorCalibration=PIOS_MPXV7002_Calibrate(calibrationCount-CALIBRATION_IDLE_MS/SAMPLING_DELAY_MS_MPXV7002);
				
				data.ZeroPoint=sensorCalibration;
				data.Airspeed = 0;
				BaroAirspeedSet(&data);
				continue;
			}
			else if (sensorCalibration!= data.ZeroPoint){       //Finally, monitor the UAVO in case the user has manually changed the sensor calibration value.
				PIOS_MPXV7002_UpdateCalibration(data.ZeroPoint); //This makes sense for the user if the initial calibration was not good and the user does not wish to reboot.
			}
			
			//Get CAS
			float calibratedAirspeed = PIOS_MPXV7002_ReadAirspeed();

			//Get sensor value, just for telemetry purposes. 
			//This is a silly waste of resources, and should probably be removed at some point in the future.
			//At this time, PIOS_MPXV7002_Measure() should become a static function and be removed from the header file.
			data.SensorValue=PIOS_MPXV7002_Measure();
			
			//Filter CAS
			float alpha=.01f*((float) SAMPLING_DELAY_MS_MPXV7002); //Low pass filter. The time constant is equal to about 100ms
			float filteredAirspeed = calibratedAirspeed*(alpha)+data.Airspeed*(1.0f-alpha);
			
			data.Connected = BAROAIRSPEED_CONNECTED_TRUE;
			data.Airspeed = filteredAirspeed;
#endif			
		}
		else if (data.AirspeedSensorType==BAROAIRSPEED_AIRSPEEDSENSORTYPE_EAGLETREEAIRSPEEDV3){ //Eagletree Airspeed v3
#if defined(PIOS_INCLUDE_ETASV3)
			//Wait until our turn
			vTaskDelay(SAMPLING_DELAY_MS_ETASV3);
			
			// Update the airspeed
			BaroAirspeedGet(&data);
			
			//Check to see if airspeed sensor is returning data
			data.SensorValue = PIOS_ETASV3_ReadAirspeed();
			if (data.SensorValue==-1) {
				data.Connected = BAROAIRSPEED_CONNECTED_FALSE;
				data.Airspeed = 0;
				BaroAirspeedSet(&data);
				continue;
			}
			
			//Calibrate sensor by averaging zero point value //THIS SHOULD NOT BE DONE IF THERE IS AN IN-AIR RESET. HOW TO DETECT THIS?
			if (calibrationCount < CALIBRATION_IDLE_MS/SAMPLING_DELAY_MS_ETASV3) {
				calibrationCount++;
				continue;
			} else if (calibrationCount < (CALIBRATION_IDLE_MS + CALIBRATION_COUNT_MS)/SAMPLING_DELAY_MS_ETASV3) {
				calibrationCount++;
				calibrationSum +=  data.SensorValue;
				if (calibrationCount == (CALIBRATION_IDLE_MS + CALIBRATION_COUNT_MS)/SAMPLING_DELAY_MS_ETASV3) {
					data.ZeroPoint = (uint16_t) (((float)calibrationSum) / CALIBRATION_COUNT_MS +0.5f);
				} else {
					continue;
				}
			}
			
			//Compute airspeed
			data.Airspeed = ETS_AIRSPEED_SCALE * sqrtf((float)abs(data.SensorValue - data.ZeroPoint)); //Is this calibrated or indicated airspeed?

			data.Connected = BAROAIRSPEED_CONNECTED_TRUE;
		}		
#endif
		// Update the AirspeedActual UAVObject
		BaroAirspeedSet(&data);
	}	
}

/**
 * @}
 * @}
 */
