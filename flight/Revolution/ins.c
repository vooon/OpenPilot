/**
 ******************************************************************************
 * @addtogroup INS INS

 * @brief The INS Modules perform
 *
 * @{
 * @addtogroup INS_Main
 * @brief Main function which does the hardware dependent stuff
 * @{
 *
 *
 * @file       ins.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @brief      INSGPS Test Program
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

/* 
 TODO:
  BMP085 - Pressure
  IMU3000 interrupt
  BMA180 interrupt
 */

#define TYPICAL_PERIOD 3300
#define timer_rate() 100000
#define timer_count() 1
/* OpenPilot Includes */
#include "ins.h"
#include "pios.h"
#include "ahrs_spi_comm.h"
#include "insgps.h"
#include "CoordinateConversions.h"
#include "NMEA.h"
#include <stdbool.h>
#include "fifo_buffer.h"
#include "insgps_helper.h"

#define DEG_TO_RAD         (M_PI / 180.0)
#define RAD_TO_DEG         (180.0 / M_PI)

#define INSGPS_MAGLEN      1000
#define INSGPS_MAGTOL      0.5 /* error in magnetic vector length to use  */

#define GYRO_OOB(x) ((x > (1000 * DEG_TO_RAD)) || (x < (-1000 * DEG_TO_RAD)))
#define ACCEL_OOB(x) (((x > 12*9.81) || (x < -12*9.81)))
#define ISNAN(x) (x != x)
// down-sampled data index

volatile int8_t ahrs_algorithm;

/* Data accessors */
void get_gps_data();
void get_mag_data();
void get_baro_data();
void get_accel_gyro_data();

void reset_values();
void measure_noise(void);
void zero_gyros(bool update_settings);

/* Communication functions */
//void send_calibration(void);
void send_attitude(void);
void send_velocity(void);
void homelocation_callback(AhrsObjHandle obj);
//void calibration_callback(AhrsObjHandle obj);
void settings_callback(AhrsObjHandle obj);
void affine_rotate(float scale[3][4], float rotation[3]);
void calibration(float result[3], float scale[3][4], float arg[3]);

extern void PIOS_Board_Init(void);
void panic(uint32_t blinks);
static void print_ekf_binary(bool ekf);
void simple_update();

/* Bootloader related functions and var*/
void firmwareiapobj_callback(AhrsObjHandle obj);
volatile uint8_t reset_count=0;

/**
 * @addtogroup INS_Global_Data INS Global Data
 * @{
 * Public data.  Used by both EKF and the sender
 */

//! Contains the data from the mag sensor chip
struct mag_sensor mag_data;

//! Contains the data from the accelerometer
struct accel_sensor  accel_data;

//! Contains the data from the gyro
struct gyro_sensor gyro_data;

//! Conains the current estimate of the attitude
struct attitude_solution attitude_data;

//! Contains data from the altitude sensor
struct altitude_sensor altitude_data;

//! Contains data from the GPS (via the SPI link)
struct gps_sensor gps_data;

static float mag_len = 0;

typedef enum { INS_IDLE, INS_DATA_READY, INS_PROCESSING } states;

/**
 * @}
 */

/**
 * @brief INS Main function
 */

uint32_t total_conversion_blocks;
static bool bias_corrected_raw;

float pressure, altitude;

int32_t dr;

static volatile bool init_algorithm = false;
static bool zeroed_gyros = false;
int32_t sclk, sclk_prev;
int32_t sclk_count;
uint32_t loop_time;
int main()
{	
//	*(volatile unsigned long *)0xE000ED88 |= (0xf << 20);

	PIOS_Board_Init();
	PIOS_LED_Off(LED1);
	PIOS_LED_On(LED2);
		
	// Sensors need a second to start
	PIOS_DELAY_WaitmS(100);

	ahrs_algorithm = INSSETTINGS_ALGORITHM_SIMPLE;
	reset_values();	
	gps_data.quality = -1;
#if 0
	// Sensor test
	if(PIOS_IMU3000_Test() != 0)
		panic(1);
	
	if(PIOS_BMA180_Test() != 0)
		panic(2);
	
	if(PIOS_HMC5883_Test() != 0)
		panic(3);	
	
	if(PIOS_BMP085_Test() != 0)
		panic(4); 	
#endif

	PIOS_LED_On(LED1);
	PIOS_LED_Off(LED2);
	
	// Kickstart BMP085 measurements until driver improved
	PIOS_BMP085_StartADC(TemperatureConv);

	// Flash warning light while trying to connect
	uint32_t time_val1 = PIOS_DELAY_GetRaw();
	uint32_t time_val2;
	uint32_t ms_count = 0;
	while(!AhrsLinkReady()) {
		AhrsPoll();
		if(PIOS_DELAY_DiffuS(time_val1) > 1000) {
			ms_count += 1;
			time_val1 = PIOS_DELAY_GetRaw();
		}
		if(ms_count > 100) {
			PIOS_LED_Toggle(LED2);
			ms_count = 0;
		}
	}
	PIOS_LED_Off(LED2);
	
	/* we didn't connect the callbacks before because we have to wait
	 for all data to be up to date before doing anything*/
	InsSettingsConnectCallback(settings_callback);
	HomeLocationConnectCallback(homelocation_callback);
	//FirmwareIAPObjConnectCallback(firmwareiapobj_callback);
	
	for(uint32_t i = 0; i < 200; i++) {
		get_accel_gyro_data();   // This function blocks till data avilable
		get_mag_data();
		get_baro_data();
		PIOS_DELAY_WaituS(TYPICAL_PERIOD);
	}

	settings_callback(InsSettingsHandle());
	ins_init_algorithm(); 
	
	/******************* Main EKF loop ****************************/
	while(1) {
		AhrsPoll();
		InsStatusData status;
		InsStatusGet(&status);
		
		// Alive signal
		if ((total_conversion_blocks++ % 100) == 0)
			PIOS_LED_Toggle(LED1);
		
		loop_time = PIOS_DELAY_DiffuS(time_val1);
		time_val1 = PIOS_DELAY_GetRaw();
		
		get_accel_gyro_data();   // This function blocks till data avilable
		get_mag_data();
		get_baro_data();
		get_gps_data();
		
		status.IdleTimePerCycle = PIOS_DELAY_DiffuS(time_val1);
		
		if(ISNAN(accel_data.filtered.x + accel_data.filtered.y + accel_data.filtered.z) ||
		   ISNAN(gyro_data.filtered.x + gyro_data.filtered.y + gyro_data.filtered.z) ||
		   ACCEL_OOB(accel_data.filtered.x) || ACCEL_OOB(accel_data.filtered.y) || ACCEL_OOB(accel_data.filtered.z) ||
		   GYRO_OOB(gyro_data.filtered.x) || GYRO_OOB(gyro_data.filtered.y) || GYRO_OOB(gyro_data.filtered.z)) {
			// If any values are NaN or huge don't update
			//TODO: add field to ahrs status to track number of these events
			continue;
		}
		
		if(total_conversion_blocks <= 100 && !zeroed_gyros) {
			// TODO: Replace this with real init
			zero_gyros(total_conversion_blocks == 100);
			if(total_conversion_blocks == 100)
				zeroed_gyros = true;
			PIOS_DELAY_WaituS(TYPICAL_PERIOD);

			float zeros[3] = {0,0,0};
			INSSetGyroBias(zeros);

			continue;
		}
		/* If algorithm changed reinit.  This could go in callback but wouldn't be synchronous */
		if (init_algorithm) {
			ins_init_algorithm(); 
			init_algorithm = false;
		}
		
		time_val2 = PIOS_DELAY_GetRaw();
		
		print_ekf_binary(true);
		
		switch(ahrs_algorithm) {
			case INSSETTINGS_ALGORITHM_SIMPLE:
				simple_update();
				break;
			case INSSETTINGS_ALGORITHM_INSGPS_OUTDOOR:
				ins_outdoor_update();
				break;
			case INSSETTINGS_ALGORITHM_INSGPS_INDOOR:
			case INSSETTINGS_ALGORITHM_INSGPS_INDOOR_NOMAG:
				ins_indoor_update();
				break;
			case INSSETTINGS_ALGORITHM_CALIBRATION:
				measure_noise();
				break;
			case INSSETTINGS_ALGORITHM_SENSORRAW:
				print_ekf_binary(false);
				// Run at standard rate
				while(PIOS_DELAY_DiffuS(time_val1) < TYPICAL_PERIOD);
				break;
			case INSSETTINGS_ALGORITHM_ZEROGYROS:
				zero_gyros(false);
				// Run at standard rate
				while(PIOS_DELAY_DiffuS(time_val1) < TYPICAL_PERIOD);
				break;
		}
		
		status.RunningTimePerCycle = PIOS_DELAY_DiffuS(time_val2);
		InsStatusSet(&status);
	}
	
	return 0;
}



/**
 * @brief Simple update using just mag and accel.  Yaw biased and big attitude changes.
 */
void simple_update() {
	float q[4];
	float rpy[3];
	/***************** SIMPLE ATTITUDE FROM NORTH AND ACCEL ************/
	/* Very simple computation of the heading and attitude from accel. */
	rpy[2] = atan2((mag_data.raw.axis[1]), (-1 * mag_data.raw.axis[0])) * RAD_TO_DEG;
	rpy[1] = atan2(accel_data.filtered.x, accel_data.filtered.z) * RAD_TO_DEG;
	rpy[0] = atan2(accel_data.filtered.y, accel_data.filtered.z) * RAD_TO_DEG;

	RPY2Quaternion(rpy, q);
	attitude_data.quaternion.q1 = q[0];
	attitude_data.quaternion.q2 = q[1];
	attitude_data.quaternion.q3 = q[2];
	attitude_data.quaternion.q4 = q[3];
	send_attitude();
}

/**
 * @brief Output all the important inputs and states of the ekf through serial port
 */
static void print_ekf_binary(bool ekf)
{
	static uint32_t timeval;
	uint16_t delay;
	delay = PIOS_DELAY_DiffuS(timeval);
	timeval = PIOS_DELAY_GetRaw();
	
	PIOS_DELAY_WaituS(500);
	
	uint8_t framing[] = { 0xff, 0x00, 0xc3, 0x7d };
	// Dump raw buffer
	
	PIOS_COM_SendBuffer(PIOS_COM_AUX, &framing[0], sizeof(framing));
	PIOS_COM_SendBuffer(PIOS_COM_AUX, (uint8_t *) & total_conversion_blocks, sizeof(total_conversion_blocks));
	PIOS_COM_SendBuffer(PIOS_COM_AUX, (uint8_t *) & accel_data.filtered.x, 4*3);
	PIOS_COM_SendBuffer(PIOS_COM_AUX, (uint8_t *) & gyro_data.filtered.x, 4*3);
	PIOS_COM_SendBuffer(PIOS_COM_AUX, (uint8_t *) & mag_data.updated, 1);
	PIOS_COM_SendBuffer(PIOS_COM_AUX, (uint8_t *) & mag_data.scaled.axis, 3*4);
	PIOS_COM_SendBuffer(PIOS_COM_AUX, (uint8_t *) & altitude_data.updated, 1);
	PIOS_COM_SendBuffer(PIOS_COM_AUX, (uint8_t *) & altitude_data.altitude, 4);
	PIOS_COM_SendBuffer(PIOS_COM_AUX, (uint8_t *) &gyro_data.temperature, 4);
	PIOS_COM_SendBuffer(PIOS_COM_AUX, (uint8_t *) &accel_data.temperature, 4);
	PIOS_COM_SendBuffer(PIOS_COM_AUX, (uint8_t *) &delay, 2);
	PIOS_COM_SendBuffer(PIOS_COM_AUX, (uint8_t *) & gps_data, sizeof(gps_data));

	if(ekf)
		PIOS_COM_SendBuffer(PIOS_COM_AUX, (uint8_t *) & Nav, sizeof(Nav));                                             // X (86:149)
	else {
		mag_data.updated = 0;
		altitude_data.updated = 0;
		gps_data.updated = 0;
	}
}

void panic(uint32_t blinks) 
{
	int blinked = 0;
	while(1) {
		PIOS_LED_On(LED2);
		PIOS_DELAY_WaitmS(200);
		PIOS_LED_Off(LED2);
		PIOS_DELAY_WaitmS(200);

		blinked++;
		if(blinked >= blinks) {
			blinked = 0;
			PIOS_DELAY_WaitmS(1000);
		}			
	}
}

/**
 * @brief Get the accel and gyro data from whichever source when available
 *
 * This function will act as the HAL for the new INS sensors
 */
 
uint32_t accel_samples;
uint32_t gyro_samples;
struct pios_bma180_data accel;
struct pios_imu3000_data gyro;
AttitudeRawData raw;
int32_t accel_accum[3] = {0, 0, 0};
int32_t gyro_accum[3] = {0,0,0};
float scaling;

void get_accel_gyro_data()
{
	int32_t read_good;
	int32_t count;
	
	for (int i = 0; i < 3; i++) {
		accel_accum[i] = 0;
		gyro_accum[i] = 0;
	}
	accel_samples = 0;
	gyro_samples = 0;
	
	// Make sure we get one sample
	count = 0;
	while((read_good = PIOS_BMA180_ReadFifo(&accel)) != 0);
	while(read_good == 0) {	
		count++;

		accel_accum[0] += accel.x;
		accel_accum[1] += accel.y;
		accel_accum[2] += accel.z;
		
		read_good = PIOS_BMA180_ReadFifo(&accel);
	}
	accel_samples = count;

	// Make sure we get one sample
	count = 0;
	while((read_good = PIOS_IMU3000_ReadFifo(&gyro)) != 0);
	while(read_good == 0) {
		count++;

		gyro_accum[0] += gyro.x;
		gyro_accum[1] += gyro.y;
		gyro_accum[2] += gyro.z;
		
		read_good = PIOS_IMU3000_ReadFifo(&gyro);
	}
	gyro_samples = count;

	// Not the swaping of channel orders
	scaling = PIOS_BMA180_GetScale() / accel_samples;
	accel_data.filtered.x = accel_accum[0] * scaling * accel_data.calibration.scale[0] + accel_data.calibration.bias[0];
	accel_data.filtered.y = -accel_accum[1] * scaling * accel_data.calibration.scale[1] + accel_data.calibration.bias[1];
	accel_data.filtered.z = -accel_accum[2] * scaling * accel_data.calibration.scale[2] + accel_data.calibration.bias[2];

	scaling = PIOS_IMU3000_GetScale() / gyro_samples;
	gyro_data.filtered.x = -((float) gyro_accum[1]) * scaling * gyro_data.calibration.scale[0] + gyro_data.calibration.bias[0];
	gyro_data.filtered.y = -((float) gyro_accum[0]) * scaling * gyro_data.calibration.scale[1] + gyro_data.calibration.bias[1];
	gyro_data.filtered.z = -((float) gyro_accum[2]) * scaling * gyro_data.calibration.scale[2] + gyro_data.calibration.bias[2];
	
	raw.accels[0] = accel_data.filtered.x;
	raw.accels[1] = accel_data.filtered.y;
	raw.accels[2] = accel_data.filtered.z;
	raw.gyros[0] = gyro_data.filtered.x * RAD_TO_DEG;
	raw.gyros[1] = gyro_data.filtered.y * RAD_TO_DEG;
	raw.gyros[2] = gyro_data.filtered.z * RAD_TO_DEG;

	// From data sheet 35 deg C corresponds to -13200, and 280 LSB per C
	gyro_data.temperature = 35.0f + ((float) gyro.temperature + 13200) / 280;
	// From the data sheet 25 deg C corresponds to 2 and 2 LSB per C
	accel_data.temperature = 25.0f + ((float) accel.temperature - 2) / 2;
	
	if (bias_corrected_raw)
	{
		raw.gyros[0] -= Nav.gyro_bias[0] * RAD_TO_DEG;
		raw.gyros[1] -= Nav.gyro_bias[1] * RAD_TO_DEG;
		raw.gyros[2] -= Nav.gyro_bias[2] * RAD_TO_DEG;
		
		raw.accels[0] -= Nav.accel_bias[0];
		raw.accels[1] -= Nav.accel_bias[1];
		raw.accels[2] -= Nav.accel_bias[2];
	}
	
	raw.temperature[ATTITUDERAW_TEMPERATURE_GYRO] = gyro_data.temperature;
	raw.temperature[ATTITUDERAW_TEMPERATURE_ACCEL] = accel_data.temperature;
	
	raw.magnetometers[0] = mag_data.scaled.axis[0];
	raw.magnetometers[1] = mag_data.scaled.axis[1];
	raw.magnetometers[2] = mag_data.scaled.axis[2];
	AttitudeRawSet(&raw);
}

/**
 * @brief Get the mag data from the I2C sensor and load into structure
 * @return none
 *
 * This function also considers if the home location is set and has a valid
 * magnetic field before updating the mag data to prevent data being used that
 * cannot be interpreted.  In addition the mag data is not used for the first
 * five seconds to allow the filter to start to converge
 */
void get_mag_data()
{
	// Get magnetic readings
	// For now don't use mags until the magnetic field is set AND until 5 seconds
	// after initialization otherwise it seems to have problems
	// TODO: Follow up this initialization issue
	HomeLocationData home;
	HomeLocationGet(&home);
	if (PIOS_HMC5883_NewDataAvailable()) {
		PIOS_HMC5883_ReadMag(mag_data.raw.axis);

		mag_data.scaled.axis[0] = -(mag_data.raw.axis[0] * mag_data.calibration.scale[0]) + mag_data.calibration.bias[0];
		mag_data.scaled.axis[1] = -(mag_data.raw.axis[1] * mag_data.calibration.scale[1]) + mag_data.calibration.bias[1];
		mag_data.scaled.axis[2] = -(mag_data.raw.axis[2] * mag_data.calibration.scale[2]) + mag_data.calibration.bias[2];

		mag_data.updated =  true;
	}
}

/**
 * @brief Get the barometer data
 * @return none
 */
uint32_t baro_conversions = 0;
void get_baro_data() 
{
	int32_t retval = PIOS_BMP085_ReadADC();
	if (retval == 0) { // Conversion completed 
		pressure = PIOS_BMP085_GetPressure();
		altitude = 44330.0 * (1.0 - powf(pressure / BMP085_P0, (1.0 / 5.255)));
		
		BaroAltitudeData data;
		BaroAltitudeGet(&data);
		data.Altitude = altitude;
		data.Pressure = pressure  / 1000.0f;
		data.Temperature = PIOS_BMP085_GetTemperature() / 10.0f;  // Convert to deg C
		BaroAltitudeSet(&data);
		
		if((baro_conversions++) % 2)
			PIOS_BMP085_StartADC(PressureConv);
		else
			PIOS_BMP085_StartADC(TemperatureConv);
			
		altitude_data.altitude = data.Altitude;
		altitude_data.updated = true;
	}
}

/**
 * @brief Process any data coming in the gps port
 */
void get_gps_data() 
{
	uint8_t c;
	static bool start_flag = false;
	static bool found_cr = false;
	static char gps_rx_buffer[NMEA_MAX_PACKET_LENGTH];
	static uint32_t rx_count = 0;
	static uint32_t numChecksumErrors = 0;
	static uint32_t numParsingErrors = 0;
	static uint32_t numOverflowErrors = 0;
	static uint32_t numUpdates = 0;
	while(PIOS_COM_ReceiveBuffer(pios_com_gps_id, &c, 1, 0) == 1) 
	{
		// Echo data back out aux port
		//PIOS_COM_SendBufferNonBlocking(pios_com_aux_id, &c, 1);
		
		// detect start while acquiring stream
		if (!start_flag && (c == '$'))
		{
			start_flag = true;
			found_cr = false;
			rx_count = 0;
		}
		else if (!start_flag)
			continue;
		
		if (rx_count >= NMEA_MAX_PACKET_LENGTH)
		{
			// The buffer is already full and we haven't found a valid NMEA sentence.
			// Flush the buffer and note the overflow event.
			start_flag = false;
			found_cr = false;
			rx_count = 0;
			numOverflowErrors++;
		}
		else
		{
			gps_rx_buffer[rx_count] = c;
			rx_count++;
		}
		
		// look for ending '\r\n' sequence
		if (!found_cr && (c == '\r') )
			found_cr = true;
		else if (found_cr && (c != '\n') )
				found_cr = false;  // false end flag
		else if (found_cr && (c == '\n') )
		{
			// The NMEA functions require a zero-terminated string
			// As we detected \r\n, the string as for sure 2 bytes long, we will also strip the \r\n
			gps_rx_buffer[rx_count-2] = 0;
			
			// prepare to parse next sentence
			start_flag = false;
			found_cr = false;
			rx_count = 0;
			// Our rxBuffer must look like this now:
			//   [0]           = '$'
			//   ...           = zero or more bytes of sentence payload
			//   [end_pos - 1] = '\r'
			//   [end_pos]     = '\n'
			//
			// Prepare to consume the sentence from the buffer
			
			// Validate the checksum over the sentence
			if (!NMEA_checksum(&gps_rx_buffer[1]))
			{	// Invalid checksum.  May indicate dropped characters on Rx.
				//PIOS_DEBUG_PinHigh(2);
				++numChecksumErrors;
				//PIOS_DEBUG_PinLow(2);
			}
			else
			{	// Valid checksum, use this packet to update the GPS position
				if (!NMEA_update_position(&gps_rx_buffer[1])) {
					//PIOS_DEBUG_PinHigh(2);
					++numParsingErrors;
					//PIOS_DEBUG_PinLow(2);
				}
				else {
					++numUpdates;
					
					GPSPositionData pos;
					GPSPositionGet(&pos);
					HomeLocationData home;
					HomeLocationGet(&home);
					
					// convert from cm back to meters
					double LLA[3] = {(double) pos.Latitude / 1e7, (double) pos.Longitude / 1e7, (double) (pos.GeoidSeparation + pos.Altitude)};
					// put in local NED frame
					double ECEF[3] = {(double) (home.ECEF[0] / 100), (double) (home.ECEF[1] / 100), (double) (home.ECEF[2] / 100)};
					LLA2Base(LLA, ECEF, (float (*)[3]) home.RNE, gps_data.NED);
					
					gps_data.heading = pos.Heading;
					gps_data.groundspeed = pos.Groundspeed;
					gps_data.quality = pos.Satellites;
					gps_data.updated = true;
					
					const uint32_t INSGPS_GPS_MINSAT = 6;
					const float INSGPS_GPS_MINPDOP = 4;
					
					// if poor don't use this update
					if((ahrs_algorithm != INSSETTINGS_ALGORITHM_INSGPS_OUTDOOR) ||
					   (pos.Satellites < INSGPS_GPS_MINSAT) || 
					   (pos.PDOP >= INSGPS_GPS_MINPDOP) || 
					   (home.Set == HOMELOCATION_SET_FALSE) ||
					   ((home.ECEF[0] == 0) && (home.ECEF[1] == 0) && (home.ECEF[2] == 0)))
					{
						gps_data.updated = false;
					}	
				}
			}
		}
	}
	
}

/**
 * @brief Assumes board is not moving computes biases and variances of sensors
 * @returns None
 *
 * All data is stored in global structures.  This function should be called from OP when
 * aircraft is in stable state and then the data stored to SD card.
 *
 * After this function the bias for each sensor will be the mean value.  This doesn't make
 * sense for the z accel so make sure 6 point calibration is also run and those values set
 * after these read.
 */
#define NMEAN  500
#define NVAR   1000
#define CHANNELS 6
static uint32_t calibrate_count = 0;
float f_means[CHANNELS];
float f_var[CHANNELS] = {0, 0, 0, 0, 0, 0};
void measure_noise()
{
	uint32_t i;

	float data[CHANNELS] = {accel_data.filtered.x, 
		accel_data.filtered.y,
		accel_data.filtered.z, 
		gyro_data.filtered.x,
		gyro_data.filtered.y,
		gyro_data.filtered.z
	};
	
	// First step, zero all sufficient statistics
	if(calibrate_count == 0) {
		for (i = 0; i < CHANNELS; i++) {
			f_means[i] = 0;
			f_var[i] = 0;
		}
	}
	
	// Accumulate for an estimate of mean
	if(calibrate_count < NMEAN)
		for (i = 0; i < CHANNELS; i++)
			f_means[i] += data[i];
	
	if(calibrate_count == NMEAN)
		for (i = 0; i < CHANNELS; i++)
			f_means[i] /= (float) NMEAN;
	
	// Accumulate for estimate of variance.  This needs to be done
	// sequentially because storing second moment would go out of 
	// float precision
	if(calibrate_count >= NMEAN && calibrate_count < (NMEAN + NVAR))
		for (i = 0; i < CHANNELS; i++)
			f_var[i] += pow(f_means[i] - data[i],2);
	
	if(calibrate_count == (NMEAN + NVAR)) {
		for (i = 0; i < CHANNELS; i++)
			f_var[i] /= (float) (NVAR - 1);

		calibrate_count = 0;

		InsSettingsData settings;
		InsSettingsGet(&settings);
		
		settings.Algorithm = INSSETTINGS_ALGORITHM_NONE;
		settings.accel_var[0] = f_var[0];
		settings.accel_var[1] = f_var[1];
		settings.accel_var[2] = f_var[2];
		settings.gyro_var[0] = f_var[3];
		settings.gyro_var[1] = f_var[4];
		settings.gyro_var[2] = f_var[5];
		
		InsSettingsSet(&settings);
		settings_callback(InsSettingsHandle());
	} else {
		PIOS_DELAY_WaituS(TYPICAL_PERIOD);
		calibrate_count++;
	}
}

void zero_gyros(bool update_settings)
{
	const float rate = 1e-2;
	gyro_data.calibration.bias[0] += -gyro_data.filtered.x * rate;
	gyro_data.calibration.bias[1] += -gyro_data.filtered.y * rate;
	gyro_data.calibration.bias[2] += -gyro_data.filtered.z * rate;
	
	if(update_settings) {
		InsSettingsData settings;
		InsSettingsGet(&settings);
		settings.gyro_bias[INSSETTINGS_GYRO_BIAS_X] = gyro_data.calibration.bias[0];
		settings.gyro_bias[INSSETTINGS_GYRO_BIAS_Y] = gyro_data.calibration.bias[1];
		settings.gyro_bias[INSSETTINGS_GYRO_BIAS_Z] = gyro_data.calibration.bias[2];
		InsSettingsSet(&settings);
	}
}

/**
 * @brief Populate fields with initial values
 */
void reset_values()
{
	accel_data.calibration.scale[0] = 1;
	accel_data.calibration.scale[1] = 1;
	accel_data.calibration.scale[2] = 1;
	accel_data.calibration.bias[0] = 0;
	accel_data.calibration.bias[1] = 0;
	accel_data.calibration.bias[2] = 0;
	accel_data.calibration.variance[0] = 1;
	accel_data.calibration.variance[1] = 1;
	accel_data.calibration.variance[2] = 1;

	gyro_data.calibration.scale[0] = 1;
	gyro_data.calibration.scale[1] = 1;
	gyro_data.calibration.scale[2] = 1;
	gyro_data.calibration.bias[0] = 0;
	gyro_data.calibration.bias[1] = 0;
	gyro_data.calibration.bias[2] = 0;
	gyro_data.calibration.variance[0] = 1;
	gyro_data.calibration.variance[1] = 1;
	gyro_data.calibration.variance[2] = 1;
	
	mag_data.calibration.scale[0] = 1;
	mag_data.calibration.scale[1] = 1;
	mag_data.calibration.scale[2] = 1;
	mag_data.calibration.bias[0] = 0;
	mag_data.calibration.bias[1] = 0;
	mag_data.calibration.bias[2] = 0;
	mag_data.calibration.variance[0] = 50;
	mag_data.calibration.variance[1] = 50;
	mag_data.calibration.variance[2] = 50;
	
	ahrs_algorithm = INSSETTINGS_ALGORITHM_NONE;
}

void send_attitude(void)
{
	AttitudeActualData attitude;
	AttitudeActualGet(&attitude);
	attitude.q1 = attitude_data.quaternion.q1;
	attitude.q2 = attitude_data.quaternion.q2;
	attitude.q3 = attitude_data.quaternion.q3;
	attitude.q4 = attitude_data.quaternion.q4;
	float rpy[3];
	Quaternion2RPY(&attitude_data.quaternion.q1, rpy);
	attitude.Roll = rpy[0];
	attitude.Pitch = rpy[1];
	attitude.Yaw = rpy[2];
	AttitudeActualSet(&attitude);
}

void send_velocity(void)
{
	VelocityActualData velocityActual;
	VelocityActualGet(&velocityActual);

	// convert into cm
	velocityActual.North = Nav.Vel[0] * 100;
	velocityActual.East = Nav.Vel[1] * 100;
	velocityActual.Down = Nav.Vel[2] * 100;

	VelocityActualSet(&velocityActual);
}

int callback_count = 0;
void settings_callback(AhrsObjHandle obj)
{
	callback_count ++;
	InsSettingsData settings;
	InsSettingsGet(&settings);

	init_algorithm = ahrs_algorithm != settings.Algorithm;
	ahrs_algorithm = settings.Algorithm;
	bias_corrected_raw = settings.BiasCorrectedRaw == INSSETTINGS_BIASCORRECTEDRAW_TRUE;

	accel_data.calibration.scale[0] = settings.accel_scale[INSSETTINGS_ACCEL_SCALE_X];
	accel_data.calibration.scale[1] = settings.accel_scale[INSSETTINGS_ACCEL_SCALE_Y];
	accel_data.calibration.scale[2] = settings.accel_scale[INSSETTINGS_ACCEL_SCALE_Z];
	accel_data.calibration.bias[0] = settings.accel_bias[INSSETTINGS_ACCEL_BIAS_X];
	accel_data.calibration.bias[1] = settings.accel_bias[INSSETTINGS_ACCEL_BIAS_Y];
	accel_data.calibration.bias[2] = settings.accel_bias[INSSETTINGS_ACCEL_BIAS_Z];
	accel_data.calibration.variance[0] = settings.accel_var[INSSETTINGS_ACCEL_VAR_X];
	accel_data.calibration.variance[1] = settings.accel_var[INSSETTINGS_ACCEL_VAR_Y];
	accel_data.calibration.variance[2] = settings.accel_var[INSSETTINGS_ACCEL_VAR_Z];
	
	gyro_data.calibration.scale[0] = settings.gyro_scale[INSSETTINGS_GYRO_SCALE_X];
	gyro_data.calibration.scale[1] = settings.gyro_scale[INSSETTINGS_GYRO_SCALE_Y];
	gyro_data.calibration.scale[2] = settings.gyro_scale[INSSETTINGS_GYRO_SCALE_Z];
	gyro_data.calibration.bias[0] = settings.gyro_bias[INSSETTINGS_GYRO_BIAS_X];
	gyro_data.calibration.bias[1] = settings.gyro_bias[INSSETTINGS_GYRO_BIAS_Y];
	gyro_data.calibration.bias[2] = settings.gyro_bias[INSSETTINGS_GYRO_BIAS_Z];
	gyro_data.calibration.variance[0] = settings.gyro_var[INSSETTINGS_GYRO_VAR_X];
	gyro_data.calibration.variance[1] = settings.gyro_var[INSSETTINGS_GYRO_VAR_Y];
	gyro_data.calibration.variance[2] = settings.gyro_var[INSSETTINGS_GYRO_VAR_Z];
	
	mag_data.calibration.scale[0] = settings.mag_scale[INSSETTINGS_MAG_SCALE_X];
	mag_data.calibration.scale[1] = settings.mag_scale[INSSETTINGS_MAG_SCALE_Y];
	mag_data.calibration.scale[2] = settings.mag_scale[INSSETTINGS_MAG_SCALE_Z];
	mag_data.calibration.bias[0] = settings.mag_bias[INSSETTINGS_MAG_BIAS_X];
	mag_data.calibration.bias[1] = settings.mag_bias[INSSETTINGS_MAG_BIAS_Y];
	mag_data.calibration.bias[2] = settings.mag_bias[INSSETTINGS_MAG_BIAS_Z];
	mag_data.calibration.variance[0] = settings.mag_var[INSSETTINGS_MAG_VAR_X];
	mag_data.calibration.variance[1] = settings.mag_var[INSSETTINGS_MAG_VAR_Y];
	mag_data.calibration.variance[2] = settings.mag_var[INSSETTINGS_MAG_VAR_Z];
}

void homelocation_callback(AhrsObjHandle obj)
{
	HomeLocationData data;
	HomeLocationGet(&data);

	mag_len = sqrt(pow(data.Be[0],2) + pow(data.Be[1],2) + pow(data.Be[2],2));
	float Be[3] = {data.Be[0] / mag_len, data.Be[1] / mag_len, data.Be[2] / mag_len};

	INSSetMagNorth(Be);
	
	init_algorithm = true;
}

void firmwareiapobj_callback(AhrsObjHandle obj) 
{
#if 0
	const struct pios_board_info * bdinfo = &pios_board_info_blob;
	
	FirmwareIAPObjData firmwareIAPObj;
	FirmwareIAPObjGet(&firmwareIAPObj);
	if(firmwareIAPObj.ArmReset==0)
		reset_count=0;
	if(firmwareIAPObj.ArmReset==1)
	{
		
		if((firmwareIAPObj.BoardType==bdinfo->board_type) || (firmwareIAPObj.BoardType==0xFF))
		{
			
			++reset_count;
			if(reset_count>2)
			{
				PIOS_IAP_SetRequest1();
				PIOS_IAP_SetRequest2();
				PIOS_SYS_Reset();
			}
		}
	}
	else if(firmwareIAPObj.BoardType==bdinfo->board_type && firmwareIAPObj.crc!=PIOS_BL_HELPER_CRC_Memory_Calc())
	{
		PIOS_BL_HELPER_FLASH_Read_Description(firmwareIAPObj.Description,bdinfo->desc_size);
		firmwareIAPObj.crc=PIOS_BL_HELPER_CRC_Memory_Calc();
		firmwareIAPObj.BoardRevision=bdinfo->board_rev;
		FirmwareIAPObjSet(&firmwareIAPObj);
	}
#endif
}


/**
 * @}
 */

