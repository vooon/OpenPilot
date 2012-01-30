/**
  ******************************************************************************
  * @addtogroup PIOS PIOS Core hardware abstraction layer
  * @{
  * @addtogroup PIOS_BMP085 BMP085 Functions
  * @brief Hardware functions to deal with the altitude pressure sensor
  * @{
  *
  * @file       pios_bmp085.c  
  * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
  * @brief      BMP085 Pressure Sensor Routines
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
#define PIOS_BMP085_OVERSAMPLING oversampling

#include "pios.h"

#if defined(PIOS_INCLUDE_BMP085)

/* Glocal Variables */
ConversionTypeTypeDef CurrentRead;
int32_t pios_bmp085_eoc;

/* Local Variables */
static BMP085CalibDataTypeDef CalibData;

/* Straight from the datasheet */
static int32_t X1, X2, X3, B3, B5, B6, P;
static uint32_t B4, B7;
static volatile uint16_t RawTemperature;
static volatile uint32_t RawPressure;
static volatile uint32_t Pressure;
static volatile uint16_t Temperature;

static int32_t PIOS_BMP085_Read(uint8_t address, uint8_t * buffer, uint8_t len);
static int32_t PIOS_BMP085_Write(uint8_t address, uint8_t buffer);

// Move into proper driver structure with cfg stored
static uint32_t oversampling;
static const struct pios_bmp085_cfg * dev_cfg;

/**
* Initialise the BMP085 sensor
*/
void PIOS_BMP085_Init(const struct pios_bmp085_cfg * cfg)
{
	pios_bmp085_eoc = 0;

	oversampling = cfg->oversampling;
	dev_cfg = cfg;	// Store cfg before enabling interrupt
	
	/* Configure EOC pin as input floating */
	GPIO_Init(cfg->drdy.gpio, &cfg->drdy.init);
	
	/* Configure the End Of Conversion (EOC) interrupt */
	SYSCFG_EXTILineConfig(cfg->eoc_exti.port_source, cfg->eoc_exti.pin_source);
	EXTI_Init(&cfg->eoc_exti.init);
	
	/* Enable and set EOC EXTI Interrupt to the lowest priority */
	NVIC_Init(&cfg->eoc_irq.init);

	/* Configure anothing GPIO pin pin as output to set address */
	GPIO_Init(cfg->xclr.gpio, &cfg->xclr.init);
	GPIO_SetBits(cfg->xclr.gpio,cfg->xclr.init.GPIO_Pin);
	
	/* Read all 22 bytes of calibration data in one transfer, this is a very optimized way of doing things */
	uint8_t Data[BMP085_CALIB_LEN];
	bool good_cal = false;
	while(!good_cal) {
		good_cal = (PIOS_BMP085_Read(BMP085_CALIB_ADDR, Data, BMP085_CALIB_LEN) == 0);
		// Check none of the calibration is zero to ensure calibration is good
		for(uint8_t i = 0; i < BMP085_CALIB_LEN; i += 2)
			good_cal &= (Data[i] != 0) || (Data[i+1] != 0);
	}
			

	/* Parameters AC1-AC6 */
	CalibData.AC1 = (Data[0] << 8) | Data[1];
	CalibData.AC2 = (Data[2] << 8) | Data[3];
	CalibData.AC3 = (Data[4] << 8) | Data[5];
	CalibData.AC4 = (Data[6] << 8) | Data[7];
	CalibData.AC5 = (Data[8] << 8) | Data[9];
	CalibData.AC6 = (Data[10] << 8) | Data[11];

	/* Parameters B1, B2 */
	CalibData.B1 = (Data[12] << 8) | Data[13];
	CalibData.B2 = (Data[14] << 8) | Data[15];

	/* Parameters MB, MC, MD */
	CalibData.MB = (Data[16] << 8) | Data[17];
	CalibData.MC = (Data[18] << 8) | Data[19];
	CalibData.MD = (Data[20] << 8) | Data[21];
}

/**
* Start the ADC conversion
* \param[in] PresOrTemp BMP085_PRES_ADDR or BMP085_TEMP_ADDR
* \return 0 for success, -1 for failure (conversion completed and not read)
*/
int32_t PIOS_BMP085_StartADC(ConversionTypeTypeDef Type)
{
	if(pios_bmp085_eoc)
		return -1;

	/* Start the conversion */
	if (Type == TemperatureConv) {
		while (PIOS_BMP085_Write(BMP085_CTRL_ADDR, BMP085_TEMP_ADDR) != 0)
			continue;
	} else if (Type == PressureConv) {
		while (PIOS_BMP085_Write(BMP085_CTRL_ADDR, BMP085_PRES_ADDR) != 0)
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
int32_t PIOS_BMP085_ReadADC(void)
{
	uint8_t Data[3];
	Data[0] = 0;
	Data[1] = 0;
	Data[2] = 0;
	
	if(!pios_bmp085_eoc)
		return -1;
		
	pios_bmp085_eoc = 0;
		
	/* Read and store the 16bit result */
	if (CurrentRead == TemperatureConv) {
		/* Read the temperature conversion */
		if (PIOS_BMP085_Read(BMP085_ADC_MSB, Data, 2) != 0)
			return -1;

		RawTemperature = ((Data[0] << 8) | Data[1]);

		X1 = (RawTemperature - CalibData.AC6) * CalibData.AC5 >> 15;
		X2 = ((int32_t) CalibData.MC << 11) / (X1 + CalibData.MD);
		B5 = X1 + X2;
		Temperature = (B5 + 8) >> 4;
	} else {
		/* Read the pressure conversion */
		if (PIOS_BMP085_Read(BMP085_ADC_MSB, Data, 3) != 0)
			return -1;
		RawPressure = ((Data[0] << 16) | (Data[1] << 8) | Data[2]) >> (8 - oversampling);

		B6 = B5 - 4000;
		X1 = (CalibData.B2 * (B6 * B6 >> 12)) >> 11;
		X2 = CalibData.AC2 * B6 >> 11;
		X3 = X1 + X2;
		B3 = ((((int32_t) CalibData.AC1 * 4 + X3) << oversampling) + 2) >> 2;
		X1 = CalibData.AC3 * B6 >> 13;
		X2 = (CalibData.B1 * (B6 * B6 >> 12)) >> 16;
		X3 = ((X1 + X2) + 2) >> 2;
		B4 = (CalibData.AC4 * (uint32_t) (X3 + 32768)) >> 15;
		B7 = ((uint32_t) RawPressure - B3) * (50000 >> oversampling);
		P = B7 < 0x80000000 ? (B7 * 2) / B4 : (B7 / B4) * 2;

		X1 = (P >> 8) * (P >> 8);
		X1 = (X1 * 3038) >> 16;
		X2 = (-7357 * P) >> 16;
		Pressure = P + ((X1 + X2 + 3791) >> 4);
	}
	return 0;
}

int16_t PIOS_BMP085_GetTemperature(void)
{
	return Temperature;
}

int32_t PIOS_BMP085_GetPressure(void)
{
	return Pressure;
}

/**
* Reads one or more bytes into a buffer
* \param[in] address BMP085 register address (depends on size)
* \param[out] buffer destination buffer
* \param[in] len number of bytes which should be read
* \return 0 if operation was successful
* \return -1 if error during I2C transfer
*/
int32_t PIOS_BMP085_Read(uint8_t address, uint8_t * buffer, uint8_t len)
{
	uint8_t addr_buffer[] = {
		address,
	};

	const struct pios_i2c_txn txn_list[] = {
		{
		 .info = __func__,
		 .addr = BMP085_I2C_ADDR,
		 .rw = PIOS_I2C_TXN_WRITE,
		 .len = sizeof(addr_buffer),
		 .buf = addr_buffer,
		 }
		,
		{
		 .info = __func__,
		 .addr = BMP085_I2C_ADDR,
		 .rw = PIOS_I2C_TXN_READ,
		 .len = len,
		 .buf = buffer,
		 }
	};

	return PIOS_I2C_Transfer(PIOS_I2C_MAIN_ADAPTER, txn_list, NELEMENTS(txn_list));
}

/**
* Writes one or more bytes to the BMP085
* \param[in] address Register address
* \param[in] buffer source buffer
* \return 0 if operation was successful
* \return -1 if error during I2C transfer
*/
int32_t PIOS_BMP085_Write(uint8_t address, uint8_t buffer)
{
	uint8_t data[] = {
		address,
		buffer,
	};

	const struct pios_i2c_txn txn_list[] = {
		{
		 .info = __func__,
		 .addr = BMP085_I2C_ADDR,
		 .rw = PIOS_I2C_TXN_WRITE,
		 .len = sizeof(data),
		 .buf = data,
		 }
		,
	};

	return PIOS_I2C_Transfer(PIOS_I2C_MAIN_ADAPTER, txn_list, NELEMENTS(txn_list));
}

/**
* @brief Run self-test operation.
* \return 0 if self-test succeed, -1 if failed
*/
int32_t PIOS_BMP085_Test()
{
	// TODO: Is there a better way to test this than just checking that pressure/temperature has changed?
	int32_t cur_value = 0;

	if(PIOS_BMP085_Read(BMP085_ADC_MSB, (uint8_t *) &cur_value, 1) != 0)
		return -1;
	return 0;
			    
	cur_value = Temperature;
	PIOS_BMP085_StartADC(TemperatureConv);
	PIOS_DELAY_WaitmS(5);
	PIOS_BMP085_ReadADC();
	if (cur_value == Temperature)
		return -1;

	cur_value=Pressure;
	PIOS_BMP085_StartADC(PressureConv);
	PIOS_DELAY_WaitmS(26);
	PIOS_BMP085_ReadADC();
	if (cur_value == Pressure)
		return -1;
	
	return 0;
}

/**
 * Handle external line 2 interrupt requests
 */
uint32_t bmp085_irqs = 0;

void EXTI2_IRQHandler(void)
{
	if (EXTI_GetITStatus(dev_cfg->eoc_exti.init.EXTI_Line) != RESET) {
		bmp085_irqs++;
		/* Read the ADC Value */
		pios_bmp085_eoc=1;
		/* Clear the EXTI line pending bit */
		EXTI_ClearITPendingBit(dev_cfg->eoc_exti.init.EXTI_Line);
	}
}

#endif

/**
 * @}
 * @}
 */
