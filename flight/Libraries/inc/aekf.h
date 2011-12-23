/**
 ******************************************************************************
 * @addtogroup AHRS 
 * @{
 * @addtogroup AEKF
 * @{
 * @brief AEKF is a 14-state Adaptive High-gain EKF
 *
 * @file       aekf.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Include file of the INSGPS exposed functionality.
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

#ifndef AEKF_H_
#define AEKF_H_

#include "stdint.h"

/**
 * @addtogroup Constants
 * @{
 */
#define POS_SENSORS ((1 << 0)| (1<<1))
#define BARO_SENSOR ((1 << 2)
#define ACC_SENSORS ((1 << 3) | (1<<4) | (1<<5))
#define MAG_SENSORS ((1 << 6) | (1<<7))
#define MAG_SENSORS ((1 << 8) | (1<<9) | (1<<10))


#define FULL_SENSORS (0b11111111111);

/**
  * @}
  */

//  Exposed Function Prototypes
void AEKFInit();
void INSStatePrediction(float gyro_data[3], float accel_data[3], float dT);
void INSCovariancePrediction(float dT);
void INSCorrection(float mag_data[3], float Pos[3], float Vel[3], float BaroAlt, uint16_t SensorsUsed);

void INSResetP(float PDiag[13]);
void INSSetState(float pos[3], float vel[3], float q[4], float gyro_bias[3], float accel_bias[3]);
void INSSetPosVelVar(float PosVar, float VelVar);
void INSSetGyroBias(float gyro_bias[3]);
void INSSetAccelVar(float accel_var[3]);
void INSSetGyroVar(float gyro_var[3]);
void INSSetMagNorth(float B[3]);
void INSSetMagVar(float scaled_mag_var[3]);
void INSPosVelReset(float pos[3], float vel[3]);

void MagCorrection(float mag_data[3]);
void MagVelBaroCorrection(float mag_data[3], float Vel[3], float BaroAlt);
void FullCorrection(float mag_data[3], float Pos[3], float Vel[3],
		    float BaroAlt);
void GpsBaroCorrection(float Pos[3], float Vel[3], float BaroAlt);
void GpsMagCorrection(float mag_data[3], float Pos[3], float Vel[2]);
void VelBaroCorrection(float Vel[3], float BaroAlt);

uint16_t ins_get_num_states();

//  Nav structure containing current solution
struct NavStruct {
	float Pos[3];		// Position in meters and relative to a local NED frame
	float Vel[3];		// Velocity in meters and in NED
	float q[4];		// unit quaternion rotation relative to NED
	float gyro_bias[3];
	float accel_bias[3];
} Nav;

/**
 * @}
 * @}
 */

#endif /* EKF_H_ */
