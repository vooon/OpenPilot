/**
  ******************************************************************************
  * @addtogroup PIOS PIOS Core hardware abstraction layer
  * @{
  * @addtogroup PIOS_BMP085 BMP085 Functions
  * @brief Hardware functions to deal with the altitude pressure sensor
  * @{
  *
  * @file       pios_ms5611.c  
  * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
  * @brief      MS5611 Pressure Sensor Routines
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
// TODO: Clean this up.  Getting around old constant.
#define PIOS_MS5611_OVERSAMPLING oversampling

#include "pios.h"

#if defined(PIOS_INCLUDE_MS5611)

/* Glocal Variables */
ConversionTypeTypeDef CurrentRead;

/* Local Variables */
MS5611CalibDataTypeDef CalibData;

/* Straight from the datasheet */
static int32_t X1, X2, X3, B3, B5, B6, P;
static uint32_t B4, B7;
static volatile int32_t RawTemperature;
static volatile int32_t RawPressure;
static volatile float Pressure;
static volatile float Temperature;

static int32_t PIOS_MS5611_Read(uint8_t address, uint8_t * buffer, uint8_t len);
static int32_t PIOS_MS5611_WriteCommand(uint8_t command);

// Move into proper driver structure with cfg stored
static uint32_t oversampling;
static const struct pios_ms5611_cfg * dev_cfg;
static int32_t i2c_id;

/**
* Initialise the BMP085 sensor
*/
int32_t ms5611_read_flag;
void PIOS_MS5611_Init(const struct pios_ms5611_cfg * cfg, int32_t i2c_device)
{
	i2c_id = i2c_device;

	oversampling = cfg->oversampling;
	dev_cfg = cfg;	// Store cfg before enabling interrupt

	PIOS_MS5611_WriteCommand(MS5611_RESET);
	PIOS_DELAY_WaitmS(20);			

	uint8_t data[2];

	/* Calibration parameters */
	for (int i = 0; i < 6; i++) {
		PIOS_MS5611_Read(MS5611_CALIB_ADDR + i * 2, data, 2);
		CalibData.C[i] = (data[0] << 8) | data[1];
	}
}

/**
* Start the ADC conversion
* \param[in] PresOrTemp BMP085_PRES_ADDR or BMP085_TEMP_ADDR
* \return 0 for success, -1 for failure (conversion completed and not read)
*/
int32_t PIOS_MS5611_StartADC(ConversionTypeTypeDef Type)
{
	/* Start the conversion */
	if (Type == TemperatureConv) {
		while (PIOS_MS5611_WriteCommand(MS5611_TEMP_ADDR) != 0)
			continue;
	} else if (Type == PressureConv) {
		while (PIOS_MS5611_WriteCommand(MS5611_PRES_ADDR) != 0)
			continue;
	}

	CurrentRead = Type;
	
	return 0;
}

/**
* Read the ADC conversion value (once ADC conversion has completed)
* \param[in] PresOrTemp BMP085_PRES_ADDR or BMP085_TEMP_ADDR
* \return 0 if successfully read the ADC, -1 if failed
*/
int32_t PIOS_MS5611_ReadADC(void)
{
	uint8_t Data[3];
	Data[0] = 0;
	Data[1] = 0;
	Data[2] = 0;

	static float dT;
	/* Read and store the 16bit result */
	if (CurrentRead == TemperatureConv) {
		/* Read the temperature conversion */
		if (PIOS_MS5611_Read(MS5611_ADC_READ, Data, 3) != 0)
			return -1;

		RawTemperature = (Data[0] << 16) | (Data[1] << 8) | Data[2];
		
		dT = RawTemperature - CalibData.C[4] * (1 << 8);
		Temperature = 2000 + dT * CalibData.C[5] / (1<<23);
		Temperature /= 100.0f;

	} else {	
		float Offset;
		float  Sens;

		/* Read the pressure conversion */
		if (PIOS_MS5611_Read(MS5611_ADC_READ, Data, 3) != 0)
			return -1;
		RawPressure = ((Data[0] << 16) | (Data[1] << 8) | Data[2]);
		
		Offset = (float) CalibData.C[1] * (1<<16) +(float) CalibData.C[3] * (float) dT / (1<<7);
		Sens = (float) CalibData.C[0] * (1<<15) + ((float) CalibData.C[2] * (float) dT) / (1<<8);
		
		Pressure = (RawPressure * Sens / (1<<21) - Offset) / (1<<15);
		Pressure /= 1000.0f;
	}
	return 0;
}

/**
 * Return the most recently computed temperature in kPa
 */
float PIOS_MS5611_GetTemperature(void)
{
	return Temperature;
}

/**
 * Return the most recently computed pressure in kPa
 */
float PIOS_MS5611_GetPressure(void)
{
	return Pressure;
}

/**
* Reads one or more bytes into a buffer
* \param[in] the command indicating the address to read
* \param[out] buffer destination buffer
* \param[in] len number of bytes which should be read
* \return 0 if operation was successful
* \return -1 if error during I2C transfer
*/
int32_t PIOS_MS5611_Read(uint8_t address, uint8_t * buffer, uint8_t len)
{

	const struct pios_i2c_txn txn_list[] = {
		{
			.info = __func__,
			.addr = MS5611_I2C_ADDR,
			.rw = PIOS_I2C_TXN_WRITE,
			.len = 1,
			.buf = &address,
		}
		,
		{
		 .info = __func__,
		 .addr = MS5611_I2C_ADDR,
		 .rw = PIOS_I2C_TXN_READ,
		 .len = len,
		 .buf = buffer,
		 }
	};

	return PIOS_I2C_Transfer(i2c_id, txn_list, NELEMENTS(txn_list));
}

/**
* Writes one or more bytes to the MS5611
* \param[in] address Register address
* \param[in] buffer source buffer
* \return 0 if operation was successful
* \return -1 if error during I2C transfer
*/
int32_t PIOS_MS5611_WriteCommand(uint8_t command)
{
	const struct pios_i2c_txn txn_list[] = {
		{
		 .info = __func__,
		 .addr = MS5611_I2C_ADDR,
		 .rw = PIOS_I2C_TXN_WRITE,
		 .len = 1,
		 .buf = &command,
		 }
		,
	};

	return PIOS_I2C_Transfer(i2c_id, txn_list, NELEMENTS(txn_list));
}

/**
* @brief Run self-test operation.
* \return 0 if self-test succeed, -1 if failed
*/
int32_t PIOS_MS5611_Test()
{
	// TODO: Is there a better way to test this than just checking that pressure/temperature has changed?
	int32_t cur_value = 0;

	cur_value = Temperature;
	PIOS_MS5611_StartADC(TemperatureConv);
	PIOS_DELAY_WaitmS(5);
	PIOS_MS5611_ReadADC();
	if (cur_value == Temperature)
		return -1;

	cur_value=Pressure;
	PIOS_MS5611_StartADC(PressureConv);
	PIOS_DELAY_WaitmS(26);
	PIOS_MS5611_ReadADC();
	if (cur_value == Pressure)
		return -1;
	
	return 0;
}

#endif

/**
 * @}
 * @}
 */
