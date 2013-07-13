/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_MPU6000 MPU6000 Functions
 * @brief Deals with the hardware interface to the 3-axis gyro
 * @{
 *
 * @file       pios_mpu000.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012-2013.
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

#ifdef PIOS_INCLUDE_MPU6000

#include "fifo_buffer.h"

enum pios_mpu6000_dev_magic {
    PIOS_MPU6000_DEV_MAGIC = 0x9da9b3ed,
};

#define PIOS_MPU6000_MAX_DOWNSAMPLE 2
struct mpu6000_dev {
    uint32_t interface_id;
    uint32_t slave_num;
    xSemaphoreHandle data_ready_sema;
    const struct pios_mpu6000_cfg *cfg;
    enum pios_mpu6000_range gyro_range;
    enum pios_mpu6000_accel_range accel_range;
    enum pios_mpu6000_filter filter;
    enum pios_mpu6000_dev_magic   magic;
};

/* Global Variables */
static struct mpu6000_dev *dev;
volatile bool mpu6000_configured = false;
static uint32_t mpu6000_irq = 0;
static uint8_t mpu6000_rec_buf[sizeof(struct pios_mpu6000_data) + 1];

/* Private functions */
static struct mpu6000_dev *PIOS_MPU6000_alloc(void);
static int32_t PIOS_MPU6000_Validate(struct mpu6000_dev *dev);
static void PIOS_MPU6000_Config(struct pios_mpu6000_cfg const *cfg);
static int32_t PIOS_MPU6000_SetReg(uint8_t address, uint8_t buffer);
static int32_t PIOS_MPU6000_GetReg(uint8_t address);

#define GRAV 9.81f

/**
 * @brief Allocate a new device
 */
static struct mpu6000_dev *PIOS_MPU6000_alloc(void)
{
    struct mpu6000_dev *mpu6000_dev;

    mpu6000_dev = (struct mpu6000_dev *)pvPortMalloc(sizeof(*mpu6000_dev));
    if (!mpu6000_dev) {
        return NULL;
    }

    mpu6000_dev->magic = PIOS_MPU6000_DEV_MAGIC;

    mpu6000_dev->data_ready_sema = xSemaphoreCreateMutex();
    if (mpu6000_dev->data_ready_sema == NULL) {
        vPortFree(mpu6000_dev);
        return NULL;
    }

    mpu6000_dev->interface_id = 0;
    mpu6000_dev->slave_num    = 0;

    return mpu6000_dev;
}

/**
 * @brief Validate the handle to the spi device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_MPU6000_Validate(struct mpu6000_dev *vdev)
{
    if (vdev == NULL) {
        return -1;
    }
    if (vdev->magic != PIOS_MPU6000_DEV_MAGIC) {
        return -2;
    }
    if (vdev->interface_id == 0) {
        return -3;
    }
    return 0;
}

/**
 * @brief Change the interface clock speed
 */
#ifdef PIOS_MPU6000_I2C_INTERFACE
void PIOS_MPU6000_SetClockSpeed(__attribute__((unused)) uint32_t interface_id, __attribute__((unused)) SPIPrescalerTypeDef speed) {}
#else
void PIOS_MPU6000_SetClockSpeed(uint32_t interface_id, SPIPrescalerTypeDef speed)
{
    PIOS_SPI_SetClockSpeed(interface_id, speed);
}
#endif

/**
 * @brief Initialize the MPU6000 3-axis gyro sensor.
 * @return 0 for success, -1 for failure
 */
int32_t PIOS_MPU6000_Init(uint32_t interface_id, uint32_t slave_num, const struct pios_mpu6000_cfg *cfg)
{
    dev = PIOS_MPU6000_alloc();
    if (dev == NULL) {
        return -1;
    }

    dev->interface_id = interface_id;
    dev->slave_num    = slave_num;
    dev->cfg = cfg;

    /* Configure the MPU6000 Sensor */
    PIOS_MPU6000_SetClockSpeed(dev->interface_id, PIOS_SPI_PRESCALER_256);
    PIOS_MPU6000_Config(cfg);
    PIOS_MPU6000_SetClockSpeed(dev->interface_id, PIOS_SPI_PRESCALER_16);

    /* Set up EXTI line */
    PIOS_EXTI_Init(cfg->exti_cfg);
    return 0;
}

/**
 * @brief Initialize the MPU6000 3-axis gyro sensor
 * \return none
 * \param[in] PIOS_MPU6000_ConfigTypeDef struct to be used to configure sensor.
 *
 */
static void PIOS_MPU6000_Config(struct pios_mpu6000_cfg const *cfg)
{
    PIOS_MPU6000_Test();

    // Reset chip
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_PWR_MGMT_REG, PIOS_MPU6000_PWRMGMT_IMU_RST) != 0) {
        ;
    }
    PIOS_DELAY_WaitmS(50);

    // Reset chip and fifo
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_USER_CTRL_REG,
                               PIOS_MPU6000_USERCTL_GYRO_RST |
                               PIOS_MPU6000_USERCTL_SIG_COND |
                               PIOS_MPU6000_USERCTL_FIFO_RST) != 0) {
        ;
    }

    // Wait for reset to finish
    while (PIOS_MPU6000_GetReg(PIOS_MPU6000_USER_CTRL_REG) &
           (PIOS_MPU6000_USERCTL_GYRO_RST |
            PIOS_MPU6000_USERCTL_SIG_COND |
            PIOS_MPU6000_USERCTL_FIFO_RST)) {
        ;
    }
    PIOS_DELAY_WaitmS(10);
    // Power management configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_PWR_MGMT_REG, cfg->Pwr_mgmt_clk) != 0) {
        ;
    }

    // Interrupt configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_INT_CFG_REG, cfg->interrupt_cfg) != 0) {
        ;
    }

    // Interrupt configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_INT_EN_REG, cfg->interrupt_en) != 0) {
        ;
    }

    // FIFO storage
#if defined(PIOS_MPU6000_ACCEL)
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_FIFO_EN_REG, cfg->Fifo_store | PIOS_MPU6000_ACCEL_OUT) != 0) {
        ;
    }
#else
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_FIFO_EN_REG, cfg->Fifo_store) != 0) {
        ;
    }
#endif
    PIOS_MPU6000_ConfigureRanges(cfg->gyro_range, cfg->accel_range, cfg->filter);
    // Interrupt configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_USER_CTRL_REG, cfg->User_ctl) != 0) {
        ;
    }

    // Interrupt configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_PWR_MGMT_REG, cfg->Pwr_mgmt_clk) != 0) {
        ;
    }

    // Interrupt configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_INT_CFG_REG, cfg->interrupt_cfg) != 0) {
        ;
    }

    // Interrupt configuration
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_INT_EN_REG, cfg->interrupt_en) != 0) {
        ;
    }
    if ((PIOS_MPU6000_GetReg(PIOS_MPU6000_INT_EN_REG)) != cfg->interrupt_en) {
        return;
    }

    mpu6000_configured = true;
}
/**
 * @brief Configures Gyro, accel and Filter ranges/setings
 * @return 0 if successful, -1 if device has not been initialized
 */
int32_t PIOS_MPU6000_ConfigureRanges(
    enum pios_mpu6000_range gyroRange,
    enum pios_mpu6000_accel_range accelRange,
    enum pios_mpu6000_filter filterSetting)
{
    if (dev == NULL) {
        return -1;
    }

    // TODO: check that changing the SPI clock speed is safe
    // to do here given that interrupts are enabled and the bus has
    // not been claimed/is not claimed during this call
    PIOS_MPU6000_SetClockSpeed(dev->interface_id, PIOS_SPI_PRESCALER_256);

    // update filter settings
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_DLPF_CFG_REG, filterSetting) != 0) {
        ;
    }

    // Sample rate divider, chosen upon digital filtering settings
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_SMPLRT_DIV_REG,
                               filterSetting == PIOS_MPU6000_LOWPASS_256_HZ ?
                               dev->cfg->Smpl_rate_div_no_dlp : dev->cfg->Smpl_rate_div_dlp) != 0) {
        ;
    }

    dev->filter = filterSetting;

    // Gyro range
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_GYRO_CFG_REG, gyroRange) != 0) {
        ;
    }

    dev->gyro_range = gyroRange;
#if defined(PIOS_MPU6000_ACCEL)
    // Set the accel range
    while (PIOS_MPU6000_SetReg(PIOS_MPU6000_ACCEL_CFG_REG, accelRange) != 0) {
        ;
    }

    dev->accel_range = accelRange;
#endif
    PIOS_MPU6000_SetClockSpeed(dev->interface_id, PIOS_SPI_PRESCALER_16);
    return 0;
}

#ifdef PIOS_MPU6000_I2C_INTERFACE
/**
 * @brief Reads one or more bytes into a buffer
 * \param[in] address HMC5883 register address (depends on size)
 * \param[out] buffer destination buffer
 * \param[in] len number of bytes which should be read
 * \return 0 if operation was successful
 * \return -1 if error during I2C transfer
 * \return -2 if unable to claim i2c device
 */
static int32_t PIOS_MPU6000_I2CRead(uint8_t address, uint8_t *buffer, uint8_t len)
{
    uint8_t addr_buffer[] = {
        address,
    };

    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = dev->slave_num,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = sizeof(addr_buffer),
            .buf  = addr_buffer,
        },
        {
            .info = __func__,
            .addr = dev->slave_num,
            .rw   = PIOS_I2C_TXN_READ,
            .len  = len,
            .buf  = buffer,
        }
    };

    return PIOS_I2C_Transfer(dev->interface_id, txn_list, NELEMENTS(txn_list));
}

#else /* PIOS_MPU6000_I2C_INTERFACE */

/**
 * @brief Claim the SPI bus for the accel communications and select this chip
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 */
static int32_t PIOS_MPU6000_ClaimBus()
{
    if (PIOS_MPU6000_Validate(dev) != 0) {
        return -1;
    }
    if (PIOS_SPI_ClaimBus(dev->interface_id) != 0) {
        return -2;
    }
    PIOS_SPI_RC_PinSet(dev->interface_id, dev->slave_num, 0);
    return 0;
}

/**
 * @brief Release the SPI bus for the accel communications and end the transaction
 * @return 0 if successful
 */
static int32_t PIOS_MPU6000_ReleaseBus()
{
    if (PIOS_MPU6000_Validate(dev) != 0) {
        return -1;
    }
    PIOS_SPI_RC_PinSet(dev->interface_id, dev->slave_num, 1);
    return PIOS_SPI_ReleaseBus(dev->interface_id);
}
#endif /* PIOS_MPU6000_I2C_INTERFACE */

/**
 * @brief Read a register from MPU6000
 * @returns The register value or -1 if failure to get bus
 * @param reg[in] Register address to be read
 */
static int32_t PIOS_MPU6000_GetReg(uint8_t reg)
{
    uint8_t data;

#ifdef PIOS_MPU6000_I2C_INTERFACE
    int32_t retval = PIOS_MPU6000_I2CRead(reg, &data, sizeof(data));

    if (retval != 0) {
        return retval;
    } else {
        return data;
    }
#else
    if (PIOS_MPU6000_ClaimBus() != 0) {
        return -1;
    }

    PIOS_SPI_TransferByte(dev->interface_id, (0x80 | reg)); // request byte
    data = PIOS_SPI_TransferByte(dev->interface_id, 0); // receive response

    PIOS_MPU6000_ReleaseBus();
    return data;

#endif /* PIOS_MPU6000_I2C_INTERFACE */
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
#ifdef PIOS_MPU6000_I2C_INTERFACE
    uint8_t i2c_data[] = {
        reg,
        data,
    };

    const struct pios_i2c_txn txn_list[] = {
        {
            .info = __func__,
            .addr = dev->slave_num,
            .rw   = PIOS_I2C_TXN_WRITE,
            .len  = sizeof(i2c_data),
            .buf  = i2c_data,
        },
    };

    return PIOS_I2C_Transfer(dev->interface_id, txn_list, NELEMENTS(txn_list));

#else
    if (PIOS_MPU6000_ClaimBus() != 0) {
        return -1;
    }

    if (PIOS_SPI_TransferByte(dev->interface_id, 0x7f & reg) != 0) {
        PIOS_MPU6000_ReleaseBus();
        return -2;
    }

    if (PIOS_SPI_TransferByte(dev->interface_id, data) != 0) {
        PIOS_MPU6000_ReleaseBus();
        return -3;
    }

    PIOS_MPU6000_ReleaseBus();

    return 0;

#endif /* ifdef PIOS_MPU6000_I2C_INTERFACE */
}

/*
 * @brief Read the identification bytes from the MPU6000 sensor
 * \return ID read from MPU6000 or -1 if failure
 */
int32_t PIOS_MPU6000_ReadID()
{
    int32_t mpu6000_id = PIOS_MPU6000_GetReg(PIOS_MPU6000_WHOAMI);

    if (mpu6000_id < 0) {
        return -1;
    }
    return mpu6000_id;
}

/**
 * \brief Returns the data ready semaphore
 * \return Handle to the semaphore or null if invalid device
 */
xSemaphoreHandle PIOS_MPU6000_GetSemaphore()
{
    if (PIOS_MPU6000_Validate(dev) != 0) {
        return (xSemaphoreHandle)NULL;
    }

    return dev->data_ready_sema;
}


float PIOS_MPU6000_GetScale()
{
    switch (dev->gyro_range) {
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

float PIOS_MPU6000_GetAccelScale()
{
    switch (dev->accel_range) {
    case PIOS_MPU6000_ACCEL_2G:
        return GRAV / 16384.0f;

    case PIOS_MPU6000_ACCEL_4G:
        return GRAV / 8192.0f;

    case PIOS_MPU6000_ACCEL_8G:
        return GRAV / 4096.0f;

    case PIOS_MPU6000_ACCEL_16G:
        return GRAV / 2048.0f;
    }
    return 0;
}

/**
 * @brief Run self-test operation.
 * \return 0 if test succeeded
 * \return non-zero value if test succeeded
 */
int32_t PIOS_MPU6000_Test(void)
{
    /* Verify that ID matches (MPU6000 ID is 0x69) */
    int32_t mpu6000_id = PIOS_MPU6000_ReadID();

    if (mpu6000_id < 0) {
        return -1;
    }

    if (mpu6000_id != 0x68) {
        return -2;
    }

    return 0;
}

/**
 * @brief Obtains the number of bytes in the FIFO.
 * @return the number of bytes in the FIFO
 */
static uint16_t PIOS_MPU6000_FifoDepth()
{
    uint8_t buf[3];

#ifdef PIOS_MPU6000_I2C_INTERFACE
    if (PIOS_MPU6000_I2CRead(PIOS_MPU6000_FIFO_CNT_MSB, buf + 1, 2) < 0) {
        return -1;
    }
#else

    if (PIOS_MPU6000_ClaimBus() != 0) {
        return -1;
    }

    uint8_t mpu6000_send_buf[3] = { PIOS_MPU6000_FIFO_CNT_MSB | 0x80, 0, 0 };
    if (PIOS_SPI_TransferBlock(dev->interface_id, &mpu6000_send_buf[0], &buf[0], sizeof(mpu6000_send_buf), NULL) < 0) {
        PIOS_MPU6000_ReleaseBus();
        return -1;
    }

    PIOS_MPU6000_ReleaseBus();
#endif

    return (buf[1] << 8) | buf[2];
}

/**
 * @brief Get the status code.
 * \return the status code if successful
 * \return negative value if failed
 */
static uint8_t PIOS_MPU6000_GetStatus(void)
{
    int32_t reg;

    while ((reg = PIOS_MPU6000_GetReg(PIOS_MPU6000_INT_STATUS_REG)) < 0) {
        ;
    }
    return reg;
}

/**
 * @brief Determines if the FIFO has overflowed
 * \return true if an overflow has occurred.
 */
static bool PIOS_MPU6000_Overflowed()
{
    uint8_t status = PIOS_MPU6000_GetStatus();

    return status & PIOS_MPU6000_INT_STATUS_OVERFLOW;
}

/**
 * @brief Resets the Fifo (usually used after an overflow).
 */
static void PIOS_MPU6000_FifoReset()
{
    uint8_t reg = PIOS_MPU6000_GetReg(PIOS_MPU6000_USER_CTRL_REG);

    PIOS_MPU6000_SetReg(PIOS_MPU6000_USER_CTRL_REG, reg | PIOS_MPU6000_USERCTL_FIFO_RST);
}

/**
 * @brief Read the sensor values out of the data buffer and rotate appropriately.
 * \param[in] buf The input buffer
 * \param[out] data The scaled and rotated values.
 */
static void PIOS_MPU6000_Rotate(struct pios_mpu6000_data *data, const uint8_t *buf)
{
    uint8_t accel_y_msb = buf[0];
    uint8_t accel_y_lsb = buf[1];
    uint8_t accel_x_msb = buf[2];
    uint8_t accel_x_lsb = buf[3];
    uint8_t accel_z_msb = buf[4];
    uint8_t accel_z_lsb = buf[5];
    uint8_t temp_msb    = buf[6];
    uint8_t temp_lsb    = buf[7];
    uint8_t gyro_y_msb  = buf[8];
    uint8_t gyro_y_lsb  = buf[9];
    uint8_t gyro_x_msb  = buf[10];
    uint8_t gyro_x_lsb  = buf[11];
    uint8_t gyro_z_msb  = buf[12];
    uint8_t gyro_z_lsb  = buf[13];

    // Rotate the sensor to OP convention.  The datasheet defines X as towards the right
    // and Y as forward.  OP convention transposes this.  Also the Z is defined negatively
    // to our convention
#if defined(PIOS_MPU6000_ACCEL)
    // Currently we only support rotations on top so switch X/Y accordingly
    switch (dev->cfg->orientation) {
    case PIOS_MPU6000_TOP_0DEG:
        data->accel_y = accel_y_msb << 8 | accel_y_lsb; // chip X
        data->accel_x = accel_x_msb << 8 | accel_x_lsb; // chip Y
        data->gyro_y  = gyro_y_msb << 8 | gyro_y_lsb; // chip X
        data->gyro_x  = gyro_x_msb << 8 | gyro_x_lsb; // chip Y
        break;
    case PIOS_MPU6000_TOP_90DEG:
        // -1 to bring it back to -32768 +32767 range
        data->accel_y = -1 - (accel_x_msb << 8 | accel_x_lsb); // chip Y
        data->accel_x = accel_y_msb << 8 | accel_y_lsb; // chip X
        data->gyro_y  = -1 - (gyro_x_msb << 8 | gyro_x_lsb); // chip Y
        data->gyro_x  = gyro_y_msb << 8 | gyro_y_lsb; // chip X
        break;
    case PIOS_MPU6000_TOP_180DEG:
        data->accel_y = -1 - (accel_y_msb << 8 | accel_y_lsb); // chip X
        data->accel_x = -1 - (accel_x_msb << 8 | accel_x_lsb); // chip Y
        data->gyro_y  = -1 - (gyro_y_msb << 8 | gyro_y_lsb); // chip X
        data->gyro_x  = -1 - (gyro_x_msb << 8 | gyro_x_lsb); // chip Y
        break;
    case PIOS_MPU6000_TOP_270DEG:
        data->accel_y = accel_x_msb << 8 | accel_x_lsb; // chip Y
        data->accel_x = -1 - (accel_y_msb << 8 | accel_y_lsb); // chip X
        data->gyro_y  = gyro_x_msb << 8 | gyro_x_lsb; // chip Y
        data->gyro_x  = -1 - (gyro_y_msb << 8 | gyro_y_lsb); // chip X
        break;
    }
    data->gyro_z      = -1 - (gyro_z_msb << 8 | gyro_z_lsb);
    data->accel_z     = -1 - (accel_z_msb << 8 | accel_z_lsb);
    data->temperature = temp_msb << 8 | temp_lsb;
#else /* if defined(PIOS_MPU6000_ACCEL) */
    data->gyro_x      = accel_x_msb << 8 | accel_x_lsb;
    data->gyro_y      = accel_z_msb << 8 | accel_z_lsb;
    switch (dev->cfg->orientation) {
    case PIOS_MPU6000_TOP_0DEG:
        data->gyro_y = accel_x_msb << 8 | accel_x_lsb;
        data->gyro_x = accel_z_msb << 8 | accel_z_lsb;
        break;
    case PIOS_MPU6000_TOP_90DEG:
        data->gyro_y = -1 - (accel_z_msb << 8 | accel_z_lsb); // chip Y
        data->gyro_x = accel_x_msb << 8 | accel_x_lsb; // chip X
        break;
    case PIOS_MPU6000_TOP_180DEG:
        data->gyro_y = -1 - (accel_x_msb << 8 | accel_x_lsb);
        data->gyro_x = -1 - (accel_z_msb << 8 | accel_z_lsb);
        break;
    case PIOS_MPU6000_TOP_270DEG:
        data->gyro_y = accel_z_msb << 8 | accel_z_lsb; // chip Y
        data->gyro_x = -1 - (accel_x_msb << 8 | accel_x_lsb); // chip X
        break;
    }
    data->gyro_z = -1 - (temp_msb << 8 | temp_lsb);
    data->temperature = accel_y_msb << 8 | accel_y_lsb;
#endif /* if defined(PIOS_MPU6000_ACCEL) */
}

/**
 * @brief Read current accel, gyro, and temperature values.
 * \param[out] data The scaled and rotated values.
 * \returns true if succesful
 */
bool PIOS_MPU6000_ReadSensors(struct pios_mpu6000_data *data)
{
#ifdef PIOS_MPU6000_I2C_INTERFACE
    if (PIOS_MPU6000_I2CRead(PIOS_MPU6000_ACCEL_X_OUT_MSB, mpu6000_rec_buf, sizeof(struct pios_mpu6000_data)) < 0) {
        return false;
    }

    PIOS_MPU6000_Rotate(data, mpu6000_rec_buf);
#else /* PIOS_MPU6000_I2C_INTERFACE */
    if (PIOS_MPU6000_ClaimBus() != 0) {
        return false;
    }

    static uint8_t mpu6000_send_buf[1 + sizeof(struct pios_mpu6000_data)] = { PIOS_MPU6000_ACCEL_X_OUT_MSB | 0x80, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (PIOS_SPI_TransferBlock(dev->interface_id, mpu6000_send_buf, mpu6000_rec_buf, sizeof(mpu6000_send_buf), NULL) < 0) {
        return false;
    }

    PIOS_MPU6000_ReleaseBus();

    PIOS_MPU6000_Rotate(data, mpu6000_rec_buf + 1);
#endif /* PIOS_MPU6000_I2C_INTERFACE */

    return true;
}

/**
 * @brief Read current accel, gyro, and temperature values.
 * \param[out] data The scaled and rotated values.
 * \returns true if succesful
 */
bool PIOS_MPU6000_ReadFifo(struct pios_mpu6000_data *data)
{
    // Did the FIFO overflow?
    if (PIOS_MPU6000_Overflowed()) {
        PIOS_MPU6000_FifoReset();
        return false;
    }

    // Ensure that there's enough data in the FIFO.
    uint16_t depth = PIOS_MPU6000_FifoDepth();
    if (depth < sizeof(struct pios_mpu6000_data)) {
        return false;
    }

    // Read the data out of the FIFO.
#ifdef PIOS_MPU6000_I2C_INTERFACE
    if (PIOS_MPU6000_I2CRead(PIOS_MPU6000_FIFO_REG, mpu6000_rec_buf, sizeof(struct pios_mpu6000_data)) < 0) {
        return false;
    }

    PIOS_MPU6000_Rotate(data, mpu6000_rec_buf);
#else /* PIOS_MPU6000_I2C_INTERFACE */
    if (PIOS_MPU6000_ClaimBus() != 0) {
        return false;
    }

    static uint8_t mpu6000_send_buf[1 + sizeof(struct pios_mpu6000_data)] = { PIOS_MPU6000_FIFO_REG | 0x80, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (PIOS_SPI_TransferBlock(dev->interface_id, mpu6000_send_buf, mpu6000_rec_buf, sizeof(mpu6000_send_buf), NULL) < 0) {
        return false;
    }

    PIOS_MPU6000_ReleaseBus();

    PIOS_MPU6000_Rotate(data, mpu6000_rec_buf + 1);
#endif /* PIOS_MPU6000_I2C_INTERFACE */

    return true;
}

/**
 * @brief EXTI IRQ Handler.  Read all the data from onboard buffer
 * @return a boolean to the EXTI IRQ Handler wrapper indicating if a
 *         higher priority task is now eligible to run
 */
bool PIOS_MPU6000_IRQHandler(void)
{
    mpu6000_irq++;

    portBASE_TYPE xHigherPriorityTaskWoken;
    xSemaphoreGiveFromISR(dev->data_ready_sema, &xHigherPriorityTaskWoken);
    return xHigherPriorityTaskWoken == pdTRUE;
}

#endif /* PIOS_INCLUDE_MPU6000 */

/**
 * @}
 * @}
 */
