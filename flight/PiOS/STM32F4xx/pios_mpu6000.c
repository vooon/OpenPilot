/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MPU6000 MPU6000 Functions
 * @brief Deals with the hardware interface to the 3-axis gyro
 * @{
 *
 * @file       pios_mpu000.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @brief      MPU6000 6-axis gyro and accel chip
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************
 */
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

#if defined(PIOS_INCLUDE_MPU6000)

/* Global Variables */
uint32_t pios_spi_gyro;

/* Local Variables */
#define DEG_TO_RAD (M_PI / 180.0)
static void PIOS_MPU6000_Config(struct pios_mpu6000_cfg const * cfg);
static int32_t PIOS_MPU6000_SetReg(uint8_t address, uint8_t buffer);
static int32_t PIOS_MPU6000_GetReg(uint8_t address);

#define PIOS_MPU6000_MAX_DOWNSAMPLE 100
static int16_t pios_mpu6000_buffer[PIOS_MPU6000_MAX_DOWNSAMPLE * sizeof(struct pios_mpu6000_data)];
static t_fifo_buffer pios_mpu6000_fifo;

volatile bool mpu6000_configured = false;

static struct pios_mpu6000_cfg const * cfg;

/**
 * @brief Initialize the MPU6050 3-axis gyro sensor.
 * @return none
 */
void PIOS_MPU6000_Init(const struct pios_mpu6000_cfg * new_cfg)
{
	cfg = new_cfg;
	
	fifoBuf_init(&pios_mpu6000_fifo, (uint8_t *) pios_mpu6000_buffer, sizeof(pios_mpu6000_buffer));

	/* Configure the MPU6050 Sensor */
	PIOS_MPU6000_Config(cfg);
	PIOS_MPU6000_Config(cfg);

	/* Configure EOC pin as input floating */
	GPIO_Init(cfg->drdy.gpio, &cfg->drdy.init);
	
	/* Configure the End Of Conversion (EOC) interrupt */
	SYSCFG_EXTILineConfig(cfg->eoc_exti.port_source, cfg->eoc_exti.pin_source);
	EXTI_Init(&cfg->eoc_exti.init);
	
	/* Enable and set EOC EXTI Interrupt to the lowest priority */
	NVIC_Init(&cfg->eoc_irq.init);

}

/**
 * @brief Initialize the MPU6050 3-axis gyro sensor
 * \return none
 * \param[in] PIOS_MPU6000_ConfigTypeDef struct to be used to configure sensor.
*
*/
static void PIOS_MPU6000_Config(struct pios_mpu6000_cfg const * cfg)
{
	// Reset chip and fifo
	while (PIOS_MPU6000_SetReg(PIOS_MPU6000_USER_CTRL_REG, 0x01 | 0x02 | 0x04) != 0);
	// Wait for reset to finish
	while (PIOS_MPU6000_GetReg(PIOS_MPU6000_USER_CTRL_REG) & 0x07);
	
	//Power management configuration
	while (PIOS_MPU6000_SetReg(PIOS_MPU6000_PWR_MGMT_REG, cfg->Pwr_mgmt_clk) != 0) ;

	// Interrupt configuration
	while (PIOS_MPU6000_SetReg(PIOS_MPU6000_INT_CFG_REG, cfg->interrupt_cfg) != 0) ;

	// Interrupt configuration
	while (PIOS_MPU6000_SetReg(PIOS_MPU6000_INT_EN_REG, cfg->interrupt_en) != 0) ;

	// FIFO storage
	while (PIOS_MPU6000_SetReg(PIOS_MPU6000_FIFO_EN_REG, cfg->Fifo_store) != 0);
	
	// Sample rate divider
	while (PIOS_MPU6000_SetReg(PIOS_MPU6000_SMPLRT_DIV_REG, cfg->Smpl_rate_div) != 0) ;
	
	// Digital low-pass filter and scale
	while (PIOS_MPU6000_SetReg(PIOS_MPU6000_DLPF_CFG_REG, cfg->filter) != 0) ;
	
	// Digital low-pass filter and scale
	while (PIOS_MPU6000_SetReg(PIOS_MPU6000_GYRO_CFG_REG, cfg->gyro_range) != 0) ;
	
	// Interrupt configuration
	while (PIOS_MPU6000_SetReg(PIOS_MPU6000_USER_CTRL_REG, cfg->User_ctl) != 0) ;
	
	// Interrupt configuration
	while (PIOS_MPU6000_SetReg(PIOS_MPU6000_PWR_MGMT_REG, cfg->Pwr_mgmt_clk) != 0) ;
	
	// Interrupt configuration
	while (PIOS_MPU6000_SetReg(PIOS_MPU6000_INT_CFG_REG, cfg->interrupt_cfg) != 0) ;
	
	// Interrupt configuration
	while (PIOS_MPU6000_SetReg(PIOS_MPU6000_INT_EN_REG, cfg->interrupt_en) != 0) ;
	if((PIOS_MPU6000_GetReg(PIOS_MPU6000_INT_EN_REG)) != cfg->interrupt_en)
		return;
	
	mpu6000_configured = true;
}

/**
 * @brief Claim the SPI bus for the accel communications and select this chip
 * @return 0 if successful, -1 if unable to claim bus
 */
int32_t PIOS_MPU6000_ClaimBus()
{
	if(PIOS_SPI_ClaimBus(pios_spi_gyro) != 0)
		return -1;
	PIOS_SPI_RC_PinSet(pios_spi_gyro,0,0);
	return 0;
}

/**
 * @brief Release the SPI bus for the accel communications and end the transaction
 * @return 0 if successful
 */
int32_t PIOS_MPU6000_ReleaseBus()
{
	PIOS_SPI_RC_PinSet(pios_spi_gyro,0,1);
	return PIOS_SPI_ReleaseBus(pios_spi_gyro);
}

/**
 * @brief Connect to the correct SPI bus
 */
void PIOS_MPU6000_Attach(uint32_t spi_id)
{
	pios_spi_gyro = spi_id;
}

/**
 * @brief Read a register from MPU6000
 * @returns The register value or -1 if failure to get bus
 * @param reg[in] Register address to be read
 */
static int32_t PIOS_MPU6000_GetReg(uint8_t reg)
{
	uint8_t data;
	
	if(PIOS_MPU6000_ClaimBus() != 0)
		return -1;	
	
	PIOS_SPI_TransferByte(pios_spi_gyro,(0x80 | reg) ); // request byte
	data = PIOS_SPI_TransferByte(pios_spi_gyro,0 );     // receive response
	
	PIOS_MPU6000_ReleaseBus();
	return data;
}

/**
 * @brief Writes one byte to the MPU6000
 * \param[in] reg Register address
 * \param[in] data Byte to write
 * \return 0 if operation was successful
 * \return -1 if unable to claim SPI bus
 * \return -2 if unable to claim i2c device
 */
static int32_t PIOS_MPU6000_SetReg(uint8_t reg, uint8_t data)
{
	if(PIOS_MPU6000_ClaimBus() != 0)
		return -1;
	
	if(PIOS_SPI_TransferByte(pios_spi_gyro, 0x7f & reg) != 0) {
		PIOS_MPU6000_ReleaseBus();
		return -2;
	}
	
	if(PIOS_SPI_TransferByte(pios_spi_gyro, data) != 0) {
		PIOS_MPU6000_ReleaseBus();
		return -3;
	}
	
	PIOS_MPU6000_ReleaseBus();
	
	return 0;
}

/**
 * @brief Read current X, Z, Y values (in that order)
 * \param[out] int16_t array of size 3 to store X, Z, and Y magnetometer readings
 * \returns The number of samples remaining in the fifo
 */
int32_t PIOS_MPU6000_ReadGyros(struct pios_mpu6000_data * data)
{
	uint8_t buf[7] = {PIOS_MPU6000_GYRO_X_OUT_MSB | 0x80, 0, 0, 0, 0, 0, 0};
	uint8_t rec[7];
	
	if(PIOS_MPU6000_ClaimBus() != 0)
		return -1;

	if(PIOS_SPI_TransferBlock(pios_spi_gyro, &buf[0], &rec[0], sizeof(buf), NULL) < 0)
		return -2;
		
	PIOS_MPU6000_ReleaseBus();
	
	data->gyro_x = rec[1] << 8 | rec[2];
	data->gyro_y = rec[3] << 8 | rec[4];
	data->gyro_z = rec[5] << 8 | rec[6];
	
	return 0;
}

/**
 * @brief Read the identification bytes from the MPU6050 sensor
 * \return ID read from MPU6050 or -1 if failure
*/
int32_t PIOS_MPU6000_ReadID()
{
	int32_t mpu6000_id = PIOS_MPU6000_GetReg(PIOS_MPU6000_WHOAMI);
	if(mpu6000_id < 0)
		return -1;
	return mpu6000_id;
}

/**
 * \brief Reads the data from the MPU6050 FIFO
 * \param[out] buffer destination buffer
 * \param[in] len maximum number of bytes which should be read
 * \note This returns the data as X, Y, Z the temperature
 * \return number of bytes transferred if operation was successful
 * \return -1 if error during I2C transfer
 */
int32_t PIOS_MPU6000_ReadFifo(struct pios_mpu6000_data * buffer)
{
	if(fifoBuf_getUsed(&pios_mpu6000_fifo) < sizeof(*buffer))
		return -1;
		
	fifoBuf_getData(&pios_mpu6000_fifo, (uint8_t *) buffer, sizeof(*buffer));
	
	return 0;
}



float PIOS_MPU6000_GetScale() 
{
	switch (cfg->gyro_range) {
		case PIOS_MPU6000_SCALE_250_DEG:
			return 1.0f / 131.0f;
		case PIOS_MPU6000_SCALE_500_DEG:
			return 1.0f / 65.5f;
		case PIOS_MPU6000_SCALE_1000_DEG:
			return 1.0f / 32.8f;
		case PIOS_MPU6000_SCALE_2000_DEG:
			return 1.0f / 16.4f;
	}
	return 0;
}
/**
 * @brief Run self-test operation.
 * \return 0 if test succeeded
 * \return non-zero value if test succeeded
 */
uint8_t PIOS_MPU6000_Test(void)
{
	/* Verify that ID matches (MPU6050 ID is 0x69) */
	int32_t mpu6000_id = PIOS_MPU6000_ReadID();
	if(mpu6000_id < 0)
		return -1;
	
	if(mpu6000_id != 0x68);
		return -2;
	
	return 0;
}

/**
 * @brief Run self-test operation.
 * \return 0 if test succeeded
 * \return non-zero value if test succeeded
 */
static int32_t PIOS_MPU6000_FifoDepth(void)
{
	uint8_t mpu6000_send_buf[3] = {PIOS_MPU6000_FIFO_CNT_MSB | 0x80, 0, 0};
	uint8_t mpu6000_rec_buf[3];

	if(PIOS_MPU6000_ClaimBus() != 0)
		return -1;

	if(PIOS_SPI_TransferBlock(pios_spi_gyro, &mpu6000_send_buf[0], &mpu6000_rec_buf[0], sizeof(mpu6000_send_buf), NULL) < 0) {
		PIOS_MPU6000_ReleaseBus();
		return -1;
	}

	PIOS_MPU6000_ReleaseBus();
	
	return (mpu6000_rec_buf[1] << 8) | mpu6000_rec_buf[2];
}

/**
* @brief IRQ Handler.  Read all the data from onboard buffer
*/
uint32_t mpu6000_irq = 0;
int32_t mpu6000_count;
uint32_t mpu6000_fifo_full = 0;

uint8_t mpu6000_last_read_count = 0;
uint32_t mpu6000_fails = 0;

uint32_t mpu6000_interval_us;
uint32_t mpu6000_time_us;
uint32_t mpu6000_transfer_size;

void PIOS_MPU6000_IRQHandler(void)
{
	static uint32_t timeval;
	mpu6000_interval_us = PIOS_DELAY_DiffuS(timeval);
	timeval = PIOS_DELAY_GetRaw();

	if(!mpu6000_configured)
		return;

	mpu6000_count = PIOS_MPU6000_FifoDepth();
	if(mpu6000_count < sizeof(struct pios_mpu6000_data))
		return;
		
	if(PIOS_MPU6000_ClaimBus() != 0)
		return;		
		
	uint8_t mpu6000_send_buf[1+sizeof(struct pios_mpu6000_data)] = {PIOS_MPU6000_FIFO_REG | 0x80, 0, 0, 0, 0, 0, 0, 0, 0};
	uint8_t mpu6000_rec_buf[1+sizeof(struct pios_mpu6000_data)];
	
	if(PIOS_SPI_TransferBlock(pios_spi_gyro, &mpu6000_send_buf[0], &mpu6000_rec_buf[0], sizeof(mpu6000_send_buf), NULL) < 0) {
		PIOS_MPU6000_ReleaseBus();
		mpu6000_fails++;
		return;
	}

	PIOS_MPU6000_ReleaseBus();
	
	struct pios_mpu6000_data data;
	
	if(fifoBuf_getFree(&pios_mpu6000_fifo) < sizeof(data)) {
		mpu6000_fifo_full++;
		return;			
	}
	
	// In the case where extras samples backed up grabbed an extra
	if (mpu6000_count >= (sizeof(data) * 2)) {
		if(PIOS_MPU6000_ClaimBus() != 0)
			return;		
		
		uint8_t mpu6000_send_buf[1+sizeof(struct pios_mpu6000_data)] = {PIOS_MPU6000_FIFO_REG | 0x80, 0, 0, 0, 0, 0, 0, 0, 0};
		uint8_t mpu6000_rec_buf[1+sizeof(struct pios_mpu6000_data)];
		
		if(PIOS_SPI_TransferBlock(pios_spi_gyro, &mpu6000_send_buf[0], &mpu6000_rec_buf[0], sizeof(mpu6000_send_buf), NULL) < 0) {
			PIOS_MPU6000_ReleaseBus();
			mpu6000_fails++;
			return;
		}
		
		PIOS_MPU6000_ReleaseBus();
		
		struct pios_mpu6000_data data;
		
		if(fifoBuf_getFree(&pios_mpu6000_fifo) < sizeof(data)) {
			mpu6000_fifo_full++;
			return;			
		}

	}
	
	data.temperature = mpu6000_rec_buf[1] << 8 | mpu6000_rec_buf[2];
	data.gyro_x = mpu6000_rec_buf[3] << 8 | mpu6000_rec_buf[4];
	data.gyro_y = mpu6000_rec_buf[5] << 8 | mpu6000_rec_buf[6];
	data.gyro_z = mpu6000_rec_buf[7] << 8 | mpu6000_rec_buf[8];
	
	fifoBuf_putData(&pios_mpu6000_fifo, (uint8_t *) &data, sizeof(data));	
	mpu6000_irq++;
	
	mpu6000_time_us = PIOS_DELAY_DiffuS(timeval);
}

#endif

/**
 * @}
 * @}
 */
