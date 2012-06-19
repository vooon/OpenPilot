/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MPXV7002 MPXV7002 Functions
 * @brief Hardware functions to deal with the DIYDrones airspeed kit, using MPXV7002. 
 *    This is a differential sensor, so the value returned is first converted into 
 *    calibrated airspeed, using http://en.wikipedia.org/wiki/Calibrated_airspeed
 * @{
 *
 * @file       pios_mpxv7002.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      ETASV3 Airspeed Sensor Driver
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

/* Project Includes */
#include "pios.h"

#if defined(PIOS_INCLUDE_MPXV7002)

#include "pios_mpxv7002.h"

uint32_t calibrationSum = 0;
uint16_t calibrationOffset;

uint16_t PIOS_MPXV7002_Measure()
{
	return PIOS_ADC_PinGet(1);
}


//Returns zeroPoint so that the user can inspect the calibration vs. the sensor value
uint16_t PIOS_MPXV7002_Calibrate(uint16_t calibrationCount){
	calibrationSum +=  PIOS_MPXV7002_Measure();
	uint16_t zeroPoint = (uint16_t)(((float)calibrationSum) / calibrationCount + 0.5f);	

	calibrationOffset = zeroPoint - 2.5f/3.3f*4096.0f; //The offset should set the zero point to 2.5V
	
	return zeroPoint;
}

void PIOS_MPXV7002_UpdateCalibration(uint16_t zeroPoint){
	calibrationOffset = zeroPoint - 2.5f/3.3f*4096.0f; //The offset should set the zero point to 2.5V
}

float PIOS_MPXV7002_ReadAirspeed (void)
{

	float sensorVal = PIOS_MPXV7002_Measure();
	
	//Calculate dynamic pressure, as per docs
	float Qc = 5.0f*(((sensorVal - calibrationOffset)/4096.0f*3.3f)/Vcc - 0.5f);

	//Saturate Qc on the lower bound, in order to make sure we don't have negative airspeeds. No need
	// to saturate on the upper bound, we'll handle that later with calibratedAirspeed.
	if (Qc < 0) {
		Qc=0;
	}
	
	//Compute calibraterd airspeed, as per http://en.wikipedia.org/wiki/Calibrated_airspeed
	float calibratedAirspeed = a0*sqrt(5.0f*(pow(Qc/P0+1.0f,power)-1.0f));
	
	//Upper bound airspeed. No need to lower bound it, that comes from Qc
	if (calibratedAirspeed > 59) { //in [m/s]
		calibratedAirspeed=59;
	}
	
	
	return calibratedAirspeed;
}

#endif /* PIOS_INCLUDE_MPXV7002 */
