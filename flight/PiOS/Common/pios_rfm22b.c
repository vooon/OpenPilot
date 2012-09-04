/**
******************************************************************************
* @addtogroup PIOS PIOS Core hardware abstraction layer
* @{
* @addtogroup   PIOS_RFM22B Radio Functions
* @brief PIOS interface for for the RFM22B radio
* @{
*
* @file       pios_rfm22b.c
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
* @brief      Implements a driver the the RFM22B driver
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

// *****************************************************************
// RFM22B hardware layer
//
// This module uses the RFM22B's internal packet handling hardware to
// encapsulate our own packet data.
//
// The RFM22B internal hardware packet handler configuration is as follows ..
//
// 4-byte (32-bit) preamble .. alternating 0's & 1's
// 4-byte (32-bit) sync
// 1-byte packet length (number of data bytes to follow)
// 0 to 255 user data bytes
//
// Our own packet data will also contain it's own header and 32-bit CRC
// as a single 16-bit CRC is not sufficient for wireless comms.
//
// *****************************************************************

/* Project Includes */
#include "pios.h"

#if defined(PIOS_INCLUDE_RFM22B)

#include <pios_spi_priv.h>
#include <packet_handler.h>
#include <pios_rfm22b_priv.h>

/* Local Defines */
#define STACK_SIZE_BYTES  200

// RTC timer is running at 625Hz (1.6ms or 5 ticks == 8ms).
// A 256 byte message at 56kbps should take less than 40ms
// Note: This timeout should be rate dependent.
#define PIOS_RFM22B_SUPERVISOR_TIMEOUT 65  // ~100ms

// this is too adjust the RF module so that it is on frequency
#define OSC_LOAD_CAP					0x7F	// cap = 12.5pf .. default
#define OSC_LOAD_CAP_1				0x7D	// board 1
#define OSC_LOAD_CAP_2				0x7B	// board 2
#define OSC_LOAD_CAP_3				0x7E	// board 3
#define OSC_LOAD_CAP_4				0x7F	// board 4

// ************************************

#define TX_TEST_MODE_TIMELIMIT_MS		30000	// TX test modes time limit (in ms)

#define TX_PREAMBLE_NIBBLES				12		// 7 to 511 (number of nibbles)
#define RX_PREAMBLE_NIBBLES				6		// 5 to 31 (number of nibbles)

// the size of the rf modules internal FIFO buffers
#define FIFO_SIZE	64

#define TX_FIFO_HI_WATERMARK			62		// 0-63
#define TX_FIFO_LO_WATERMARK			32		// 0-63

#define RX_FIFO_HI_WATERMARK			32		// 0-63

#define PREAMBLE_BYTE					0x55	// preamble byte (preceeds SYNC_BYTE's)

#define SYNC_BYTE_1						0x2D    // RF sync bytes (32-bit in all)
#define SYNC_BYTE_2						0xD4    //
#define SYNC_BYTE_3						0x4B    //
#define SYNC_BYTE_4						0x59    //

// ************************************
// the default RF datarate

//#define RFM22_DEFAULT_RF_DATARATE       500          // 500 bits per sec
//#define RFM22_DEFAULT_RF_DATARATE       1000         // 1k bits per sec
//#define RFM22_DEFAULT_RF_DATARATE       2000         // 2k bits per sec
//#define RFM22_DEFAULT_RF_DATARATE       4000         // 4k bits per sec
//#define RFM22_DEFAULT_RF_DATARATE       8000         // 8k bits per sec
//#define RFM22_DEFAULT_RF_DATARATE       9600         // 9.6k bits per sec
//#define RFM22_DEFAULT_RF_DATARATE       16000        // 16k bits per sec
//#define RFM22_DEFAULT_RF_DATARATE       19200        // 19k2 bits per sec
//#define RFM22_DEFAULT_RF_DATARATE       24000        // 24k bits per sec
//#define RFM22_DEFAULT_RF_DATARATE       32000        // 32k bits per sec
//#define RFM22_DEFAULT_RF_DATARATE       64000        // 64k bits per sec
#define RFM22_DEFAULT_RF_DATARATE       128000       // 128k bits per sec
//#define RFM22_DEFAULT_RF_DATARATE       192000       // 192k bits per sec
//#define RFM22_DEFAULT_RF_DATARATE       256000       // 256k bits per sec .. NOT YET WORKING

// ************************************

#define RFM22_DEFAULT_SS_RF_DATARATE       125			// 128bps

#ifndef RX_LED_ON
#define RX_LED_ON
#define RX_LED_OFF
#define TX_LED_ON
#define TX_LED_OFF
#define LINK_LED_ON
#define LINK_LED_OFF
#define USB_LED_ON
#define USB_LED_OFF
#endif

// ************************************
// Normal data streaming
// GFSK modulation
// no manchester encoding
// data whitening
// FIFO mode
//  5-nibble rx preamble length detection
// 10-nibble tx preamble length
// AFC enabled

/* Local type definitions */
enum pios_rfm22b_dev_magic {
	PIOS_RFM22B_DEV_MAGIC = 0x68e971b6,
};

struct pios_rfm22b_dev {
	enum pios_rfm22b_dev_magic magic;
	struct pios_rfm22b_cfg cfg;

	uint32_t deviceID;

	// The Tx and Rx sepmephores
	xSemaphoreHandle txsem;
	xSemaphoreHandle rxsem;

	// The current Tx packet
	PHPacketHandle tx_packet;
	// the tx data read index
	uint16_t tx_data_rd;
	// the tx data write index
	uint16_t tx_data_wr;
	bool tx_success;

	// The Rx packets
	PHPacketHandle rx_packet;
	uint16_t rx_packet_wr;
	PHPacketHandle rx_packet_prev;
	uint16_t rx_packet_len;
	PHPacketHandle rx_packet_next;

	// The COM callback functions.
	pios_com_callback rx_in_cb;
	uint32_t rx_in_context;
	pios_com_callback tx_out_cb;
	uint32_t tx_out_context;

	// The supervisor countdown timer.
	uint16_t supv_timer;

	// Statistics
	uint16_t resets;
	uint32_t errors;
};

/* Local function forwared declarations */
static bool rfm22_processRxInt(void);
static bool rfm22_processTxInt(void);
static uint8_t rfm22_txStart(struct pios_rfm22b_dev *);
static void rfm22_setRxMode(uint8_t mode);

// A pointer to the device structure.
static struct pios_rfm22b_dev * g_rfm22b_dev;

// xtal 10 ppm, 434MHz
#define LOOKUP_SIZE	 14
const uint32_t            data_rate[] = {   500,  1000,  2000,  4000,  8000,  9600, 16000, 19200, 24000,  32000,  64000, 128000, 192000, 256000};
const uint8_t      modulation_index[] = {    16,     8,     4,     2,     1,     1,     1,     1,     1,      1,      1,      1,      1,      1};
const uint32_t       freq_deviation[] = {  4000,  4000,  4000,  4000,  4000,  4800,  8000,  9600, 12000,  16000,  32000,  64000,  96000, 128000};
const uint32_t         rx_bandwidth[] = { 17500, 17500, 17500, 17500, 17500, 19400, 32200, 38600, 51200,  64100, 137900, 269300, 420200, 518800};
const int8_t        est_rx_sens_dBm[] = {  -118,  -118,  -117,  -116,  -115,  -115,  -112,  -112,  -110,   -109,   -106,   -103,   -101,   -100}; // estimated receiver sensitivity for BER = 1E-3

const uint8_t                reg_1C[] = {  0x37,  0x37,  0x37,  0x37,  0x3A,  0x3B,  0x26,  0x28,  0x2E,   0x16,   0x07,   0x83,   0x8A,   0x8C}; // rfm22_if_filter_bandwidth

const uint8_t                reg_1D[] = {  0x44,  0x44,  0x44,  0x44,  0x44,  0x44,  0x44,  0x44,  0x44,   0x44,   0x44,   0x44,   0x44,   0x44}; // rfm22_afc_loop_gearshift_override
const uint8_t                reg_1E[] = {  0x0A,  0x0A,  0x0A,  0x0A,  0x0A,  0x0A,  0x0A,  0x0A,  0x0A,   0x0A,   0x0A,   0x0A,   0x0A,   0x02}; // rfm22_afc_timing_control

const uint8_t                reg_1F[] = {  0x03,  0x03,  0x03,  0x03,  0x03,  0x03,  0x03,  0x03,  0x03,   0x03,   0x03,   0x03,   0x03,   0x03}; // rfm22_clk_recovery_gearshift_override
const uint8_t                reg_20[] = {  0xE8,  0xF4,  0xFA,  0x70,  0x3F,  0x34,  0x3F,  0x34,  0x2A,   0x3F,   0x3F,   0x5E,   0x3F,   0x2F}; // rfm22_clk_recovery_oversampling_ratio
const uint8_t                reg_21[] = {  0x60,  0x20,  0x00,  0x01,  0x02,  0x02,  0x02,  0x02,  0x03,   0x02,   0x02,   0x01,   0x02,   0x02}; // rfm22_clk_recovery_offset2
const uint8_t                reg_22[] = {  0x20,  0x41,  0x83,  0x06,  0x0C,  0x75,  0x0C,  0x75,  0x12,   0x0C,   0x0C,   0x5D,   0x0C,   0xBB}; // rfm22_clk_recovery_offset1
const uint8_t                reg_23[] = {  0xC5,  0x89,  0x12,  0x25,  0x4A,  0x25,  0x4A,  0x25,  0x6F,   0x4A,   0x4A,   0x86,   0x4A,   0x0D}; // rfm22_clk_recovery_offset0
const uint8_t                reg_24[] = {  0x00,  0x00,  0x00,  0x02,  0x07,  0x07,  0x07,  0x07,  0x07,   0x07,   0x07,   0x05,   0x07,   0x07}; // rfm22_clk_recovery_timing_loop_gain1
const uint8_t                reg_25[] = {  0x0A,  0x23,  0x85,  0x0E,  0xFF,  0xFF,  0xFF,  0xFF,  0xFF,   0xFF,   0xFF,   0x74,   0xFF,   0xFF}; // rfm22_clk_recovery_timing_loop_gain0

const uint8_t                reg_2A[] = {  0x0E,  0x0E,  0x0E,  0x0E,  0x0E,  0x0D,  0x0D,  0x0E,  0x12,   0x17,   0x31,   0x50,   0x50,   0x50}; // rfm22_afc_limiter .. AFC_pull_in_range = ±AFCLimiter[7:0] x (hbsel+1) x 625 Hz

const uint8_t                reg_6E[] = {  0x04,  0x08,  0x10,  0x20,  0x41,  0x4E,  0x83,  0x9D,  0xC4,   0x08,   0x10,   0x20,   0x31,   0x41}; // rfm22_tx_data_rate1
const uint8_t                reg_6F[] = {  0x19,  0x31,  0x62,  0xC5,  0x89,  0xA5,  0x12,  0x49,  0x9C,   0x31,   0x62,   0xC5,   0x27,   0x89}; // rfm22_tx_data_rate0

const uint8_t                reg_70[] = {  0x2D,  0x2D,  0x2D,  0x2D,  0x2D,  0x2D,  0x2D,  0x2D,  0x2D,   0x0D,   0x0D,   0x0D,   0x0D,   0x0D}; // rfm22_modulation_mode_control1
const uint8_t                reg_71[] = {  0x23,  0x23,  0x23,  0x23,  0x23,  0x23,  0x23,  0x23,  0x23,   0x23,   0x23,   0x23,   0x23,   0x23}; // rfm22_modulation_mode_control2

const uint8_t                reg_72[] = {  0x06,  0x06,  0x06,  0x06,  0x06,  0x08,  0x0D,  0x0F,  0x13,   0x1A,   0x33,   0x66,   0x9A,   0xCD}; // rfm22_frequency_deviation

// ************************************
// Scan Spectrum settings
// GFSK modulation
// no manchester encoding
// data whitening
// FIFO mode
//  5-nibble rx preamble length detection
// 10-nibble tx preamble length
#define SS_LOOKUP_SIZE	 2

// xtal 1 ppm, 434MHz
const uint32_t ss_rx_bandwidth[] = {  2600, 10600};

const uint8_t ss_reg_1C[] = {  0x51, 0x32}; // rfm22_if_filter_bandwidth
const uint8_t ss_reg_1D[] = {  0x00, 0x00}; // rfm22_afc_loop_gearshift_override

const uint8_t ss_reg_20[] = {  0xE8, 0x38}; // rfm22_clk_recovery_oversampling_ratio
const uint8_t ss_reg_21[] = {  0x60, 0x02}; // rfm22_clk_recovery_offset2
const uint8_t ss_reg_22[] = {  0x20, 0x4D}; // rfm22_clk_recovery_offset1
const uint8_t ss_reg_23[] = {  0xC5, 0xD3}; // rfm22_clk_recovery_offset0
const uint8_t ss_reg_24[] = {  0x00, 0x07}; // rfm22_clk_recovery_timing_loop_gain1
const uint8_t ss_reg_25[] = {  0x0F, 0xFF}; // rfm22_clk_recovery_timing_loop_gain0

const uint8_t ss_reg_2A[] = {  0xFF, 0xFF}; // rfm22_afc_limiter .. AFC_pull_in_range = ±AFCLimiter[7:0] x (hbsel+1) x 625 Hz

const uint8_t ss_reg_70[] = {  0x24, 0x2D}; // rfm22_modulation_mode_control1
const uint8_t ss_reg_71[] = {  0x2B, 0x23}; // rfm22_modulation_mode_control2

// ************************************

bool initialized = false;

// the RF chips device ID number
uint8_t device_type;
// the RF chips revision number
uint8_t device_version;

// holds our current RF mode
uint8_t rf_mode;

// the minimum RF frequency we can use
uint32_t lower_carrier_frequency_limit_Hz;
// the maximum RF frequency we can use
uint32_t upper_carrier_frequency_limit_Hz;
// the current RF frequency we are on
uint32_t carrier_frequency_hz;

// the RF data rate we are using
uint32_t carrier_datarate_bps;

// the RF bandwidth currently used
uint32_t rf_bandwidth_used;
// the RF bandwidth currently used
uint32_t ss_rf_bandwidth_used;

// holds the hbsel (1 or 2)
uint8_t	hbsel;
// holds the minimum frequency step size
float frequency_step_size;

// current frequency hop channel
uint8_t	frequency_hop_channel;

uint8_t frequency_hop_step_size_reg;

// holds the adc config reg value
uint8_t adc_config;

// device status register
uint8_t device_status;
// interrupt status register 1
uint8_t int_status1;
// interrupt status register 2
uint8_t int_status2;

// afc correction reading
int16_t afc_correction;
// afc correction reading (in Hz)
int32_t afc_correction_Hz;

// the temperature sensor reading
int16_t temperature_reg;

// xtal frequency calibration value
uint8_t osc_load_cap;

// the current RSSI (register value)
uint8_t rssi;
// dBm value
int8_t rssi_dBm;

// the transmit power to use for data transmissions
uint8_t tx_power;

// the received packet
int8_t rx_packet_start_rssi_dBm;
int8_t rx_packet_start_afc_Hz;
// the received packet signal strength
int8_t rx_packet_rssi_dBm;
// the receive packet frequency offset
int8_t rx_packet_afc_Hz;

// set if the RF module has reset itself
bool power_on_reset;

uint16_t timeout_data_ms = 20;

struct pios_rfm22b_dev * rfm22b_dev_g;


// ************************************
// SPI read/write

inline static void rfm22_claimBus()
{
	// chip select line LOW
	while (PIOS_SPI_ClaimBus(PIOS_RFM22_SPI_PORT) < 0)
		;
}

inline static void rfm22_claimBusISR()
{
	// chip select line LOW
	while (PIOS_SPI_ClaimBusISR(PIOS_RFM22_SPI_PORT) < 0)
		;
}

inline static void rfm22_releaseBus()
{
	// chip select line HIGH
	PIOS_SPI_ReleaseBus(PIOS_RFM22_SPI_PORT);
}

inline static void rfm22_startBurstWrite(uint8_t addr, bool inISR)
{
	// wait 1us .. so we don't toggle the CS line to quickly
	PIOS_DELAY_WaituS(1);

	if (inISR)
		rfm22_claimBusISR();
	else
		rfm22_claimBus();

	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 0);
	PIOS_SPI_TransferByte(PIOS_RFM22_SPI_PORT, 0x80 | addr);
}

inline static void rfm22_burstWrite(uint8_t data)
{
	PIOS_SPI_TransferByte(PIOS_RFM22_SPI_PORT, data);
}

inline static void rfm22_endBurstWrite(void)
{
	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 1);
	rfm22_releaseBus();
}

inline static void rfm22_write(uint8_t addr, uint8_t data)
{
	// wait 1us .. so we don't toggle the CS line to quickly
	PIOS_DELAY_WaituS(1);

	rfm22_claimBus();

	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 0);
	PIOS_SPI_TransferByte(PIOS_RFM22_SPI_PORT, 0x80 | addr);
	PIOS_SPI_TransferByte(PIOS_RFM22_SPI_PORT, data);
	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 1);

	rfm22_releaseBus();
}

/**
 * Write a byte to a register without claiming the bus.  Also
 * toggle the NSS line
 */
inline static void rfm22_write_noclaim(uint8_t addr, uint8_t data)
{
	uint8_t buf[2] = {addr | 0x80, data};
	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 0);
	PIOS_SPI_TransferBlock(PIOS_RFM22_SPI_PORT, buf, NULL, 2, NULL);
	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 1);
}

inline static void rfm22_startBurstRead(uint8_t addr, bool inISR)
{
	// wait 1us .. so we don't toggle the CS line to quickly
	PIOS_DELAY_WaituS(1);

	if (inISR)
		rfm22_claimBusISR();
	else
		rfm22_claimBus();

	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 0);
	PIOS_SPI_TransferByte(PIOS_RFM22_SPI_PORT, addr & 0x7f);
}

inline static uint8_t rfm22_burstRead(void)
{
	return PIOS_SPI_TransferByte(PIOS_RFM22_SPI_PORT, 0xff);
}

inline static void rfm22_endBurstRead(void)
{
	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 1);
	rfm22_releaseBus();
}

inline static uint8_t rfm22_read(uint8_t addr)
{
	uint8_t rdata;

	// wait 1us .. so we don't toggle the CS line to quickly
	PIOS_DELAY_WaituS(1);

	rfm22_claimBus();

	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 0);
	PIOS_SPI_TransferByte(PIOS_RFM22_SPI_PORT, addr & 0x7f);
	rdata = PIOS_SPI_TransferByte(PIOS_RFM22_SPI_PORT, 0xff);
	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 1);

	rfm22_releaseBus();

	return rdata;
}

/**
 * Read a byte from a register without claiming the bus.  Also
 * toggle the NSS line
 */
inline static uint8_t rfm22_read_noclaim(uint8_t addr)
{
	uint8_t rdata;

	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 0);
	PIOS_SPI_TransferByte(PIOS_RFM22_SPI_PORT, addr & 0x7f);
	rdata = PIOS_SPI_TransferByte(PIOS_RFM22_SPI_PORT, 0xff);
	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 1);

	return rdata;
}

inline static void rfm22_return_from_ISR(xSemaphoreHandle sem)
{
	// Wake up the Tx task
	portBASE_TYPE pxHigherPriorityTaskWoken;
	if (xSemaphoreGiveFromISR(sem, &pxHigherPriorityTaskWoken) == pdTRUE)
	{
		portEND_SWITCHING_ISR(pxHigherPriorityTaskWoken);
	}
	else
	{
		// Something went fairly seriously wrong
		g_rfm22b_dev->errors++;
	}
}


static bool PIOS_RFM22B_validate(struct pios_rfm22b_dev * rfm22b_dev)
{
	return (rfm22b_dev->magic == PIOS_RFM22B_DEV_MAGIC);
}

static struct pios_rfm22b_dev * PIOS_RFM22B_alloc(void)
{
	struct pios_rfm22b_dev * rfm22b_dev;

	rfm22b_dev = (struct pios_rfm22b_dev *)pvPortMalloc(sizeof(*rfm22b_dev));
	if (!rfm22b_dev) return(NULL);
	rfm22b_dev_g = rfm22b_dev;

	rfm22b_dev->magic = PIOS_RFM22B_DEV_MAGIC;
	return(rfm22b_dev);
}

/**
 * Initialise an RFM22B device
 */
int32_t PIOS_RFM22B_Init(uint32_t *rfm22b_id, const struct pios_rfm22b_cfg *cfg)
{
	PIOS_DEBUG_Assert(rfm22b_id);
	PIOS_DEBUG_Assert(cfg);

	// Allocate the device structure.
	struct pios_rfm22b_dev * rfm22b_dev = (struct pios_rfm22b_dev *) PIOS_RFM22B_alloc();
	if (!rfm22b_dev)
		return(-1);
	g_rfm22b_dev = rfm22b_dev;

	// Bind the configuration to the device instance
	rfm22b_dev->cfg = *cfg;

	*rfm22b_id = (uint32_t)rfm22b_dev;

	// Initialize the packet pointers.
	rfm22b_dev->tx_packet = 0;

	// Allocate the first Rx packet.
	rfm22b_dev->rx_packet = PHGetRXPacket(pios_packet_handler);
	rfm22b_dev->rx_packet_len = 0;
	rfm22b_dev->rx_packet_prev = 0;
	rfm22b_dev->rx_packet_next = 0;
	rfm22b_dev->rx_packet_wr = 0;

	// Create a semaphore to control the tx and rx threads.
	vSemaphoreCreateBinary(rfm22b_dev->txsem);
	vSemaphoreCreateBinary(rfm22b_dev->rxsem);

	// Create our (hopefully) unique 32 bit id from the processor serial number.
	uint8_t crcs[] = { 0, 0, 0, 0 };
	{
		char serial_no_str[33];
		PIOS_SYS_SerialNumberGet(serial_no_str);
		// Create a 32 bit value using 4 8 bit CRC values.
		for (uint8_t i = 0; serial_no_str[i] != 0; ++i)
			crcs[i % 4] = PIOS_CRC_updateByte(crcs[i % 4], serial_no_str[i]);
	}
	rfm22b_dev->deviceID = crcs[0] | crcs[1] << 8 | crcs[2] << 16 | crcs[3] << 24;
	DEBUG_PRINTF(2, "RF device ID: %x\n\r", rfm22b_dev->deviceID);

	// Initialize the supervisor timer.
	rfm22b_dev->supv_timer = PIOS_RFM22B_SUPERVISOR_TIMEOUT;
	rfm22b_dev->resets = 0;
	rfm22b_dev->errors = 0;

	// Initialize the external interrupt.
	PIOS_EXTI_Init(cfg->exti_cfg);

	// Initialize the radio device.
	int initval = rfm22_init_normal(rfm22b_dev->deviceID, cfg->minFrequencyHz, cfg->maxFrequencyHz, 50000);

	if (initval < 0)
	{

		// RF module error .. flash the LED's
#if defined(PIOS_COM_DEBUG)
		DEBUG_PRINTF(2, "RF ERROR res: %d\n\r\n\r", initval);
#endif

		for(unsigned int j = 0; j < 16; j++)
		{
			USB_LED_ON;
			LINK_LED_ON;
			RX_LED_OFF;
			TX_LED_OFF;

			PIOS_DELAY_WaitmS(200);

			USB_LED_OFF;
			LINK_LED_OFF;
			RX_LED_ON;
			TX_LED_ON;

			PIOS_DELAY_WaitmS(200);
		}

		PIOS_DELAY_WaitmS(1000);

		return initval;
	}

	rfm22_setFreqCalibration(cfg->RFXtalCap);
	rfm22_setNominalCarrierFrequency(cfg->frequencyHz);
	rfm22_setDatarate(cfg->maxRFBandwidth, true);
	rfm22_setTxPower(cfg->maxTxPower);

	DEBUG_PRINTF(2, "\n\r");
	DEBUG_PRINTF(2, "RF device ID: %x\n\r", rfm22b_dev->deviceID);
	DEBUG_PRINTF(2, "RF datarate: %dbps\n\r", rfm22_getDatarate());
	DEBUG_PRINTF(2, "RF frequency: %dHz\n\r", rfm22_getNominalCarrierFrequency());
	DEBUG_PRINTF(2, "RF TX power: %d\n\r", rfm22_getTxPower());

	return 0;
}

uint32_t PIOS_RFM22B_DeviceID(uint32_t rfm22b_id)
{
	struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

	return rfm22b_dev->deviceID;
}

int8_t PIOS_RFM22B_RSSI(uint32_t rfm22b_id)
{
	return rfm22_receivedRSSI();
}

int16_t PIOS_RFM22B_Resets(uint32_t rfm22b_id)
{
	struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;

	return rfm22b_dev->resets;
}

uint32_t PIOS_RFM22B_Send_Packet(uint32_t rfm22b_id, PHPacketHandle p, uint32_t max_delay)
{
	struct pios_rfm22b_dev *rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;
	bool valid = PIOS_RFM22B_validate(rfm22b_dev);
	PIOS_Assert(valid);

	// Store the packet handle.
	rfm22b_dev->tx_packet = p;

	// Signal the Rx thread to start the packet transfer.
	xSemaphoreGive(rfm22b_dev->rxsem);

	// Block on the semephone until the transmit is finished.
	if (xSemaphoreTake(rfm22b_dev->txsem,  max_delay / portTICK_RATE_MS) != pdTRUE)
		rf_mode = RX_ERROR_MODE;

	// Did we get send the packet?
	uint32_t ret = 0;
	if (rfm22b_dev->tx_success)
		ret = PH_PACKET_SIZE(p);

	// Success
	return ret;
}

uint32_t PIOS_RFM22B_Receive_Packet(uint32_t rfm22b_id, PHPacketHandle *p, uint32_t max_delay)
{
	struct pios_rfm22b_dev * rfm22b_dev = (struct pios_rfm22b_dev *)rfm22b_id;
	bool valid = PIOS_RFM22B_validate(rfm22b_dev);
	PIOS_Assert(valid);

	// Allocate the next Rx packet
	if (rfm22b_dev->rx_packet_next == 0)
		rfm22b_dev->rx_packet_next = PHGetRXPacket(pios_packet_handler);

	// Block on the semephore until the a packet has been received.
	if (xSemaphoreTake(rfm22b_dev->rxsem,  max_delay / portTICK_RATE_MS) != pdTRUE)
		return 0;

	uint32_t rx_len = 0;
	switch (rf_mode)
	{
	case TX_COMPLETE_MODE:

		// Wake up the Tx thread.
		rfm22b_dev->tx_success = true;
		rfm22b_dev->tx_packet = 0;
		xSemaphoreGive(rfm22b_dev->txsem);
		break;

	case TX_ERROR_MODE:

		// Wake up the Tx thread.
		rfm22b_dev->tx_success = false;
		rfm22b_dev->tx_packet = 0;
		xSemaphoreGive(rfm22b_dev->txsem);
		break;

	case RX_ERROR_MODE:

		// Error receiving packet.
		*p = 0;
		break;

	case RX_COMPLETE_MODE:

		// Return the received packet.
		*p = rfm22b_dev->rx_packet_prev;
		if(*p)
		{
			rx_len = rfm22b_dev->rx_packet_len;
			rfm22b_dev->rx_packet_len = 0;
			rfm22b_dev->rx_packet_prev = 0;
		}
		break;

	default:
		break;
	}

	// Is there a Tx packet to send?
	if (rfm22b_dev->tx_packet)
	{
		// Start transmitting the packet.
		if (rfm22_txStart(rfm22b_dev))
			return 0;
		rfm22b_dev->tx_packet = 0;
	}

	// Switch to receive mode.
	rfm22_setRxMode(RX_WAIT_PREAMBLE_MODE);

	return rx_len;
}

// ************************************
// external interrupt

void PIOS_RFM22_EXT_Int(void)
{
	// this is called from the external interrupt handler

	// we haven't yet been initialized
	if (!initialized || power_on_reset)
		return;

	// Reset the supervisor timer.
	rfm22b_dev_g->supv_timer = PIOS_RFM22B_SUPERVISOR_TIMEOUT;

	// 1. Read the interrupt statuses with burst read
	rfm22_claimBusISR();
	uint8_t write_buf[3] = {RFM22_interrupt_status1 & 0x7f, 0xFF, 0xFF};
	uint8_t read_buf[3];
	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 0);
	PIOS_SPI_TransferBlock(PIOS_RFM22_SPI_PORT, write_buf, read_buf, sizeof(write_buf), NULL);
	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 1);
	int_status1 = read_buf[1];
	int_status2 = read_buf[2];

	// read device status register
	device_status = rfm22_read_noclaim(RFM22_device_status);

	// Read the RSSI if we're in RX mode
	if (rf_mode != TX_DATA_MODE)
	{
		// read rx signal strength .. 45 = -100dBm, 205 = -20dBm
		rssi = rfm22_read_noclaim(RFM22_rssi);
		// convert to dBm
		rssi_dBm = (int8_t)(rssi >> 1) - 122;
	}
	rfm22_releaseBus();

	// the RF module has gone and done a reset - we need to re-initialize the rf module
	if (int_status2 & RFM22_is2_ipor)
	{
		initialized = false;
		power_on_reset = true;
		// Need to do something here!
		return;
	}

	switch (rf_mode)
	{
	case RX_WAIT_PREAMBLE_MODE:
	case RX_WAIT_SYNC_MODE:
	case RX_DATA_MODE:

		if(rfm22_processRxInt())
			// Wake up the Rx task
			rfm22_return_from_ISR(g_rfm22b_dev->rxsem);
		break;

	case TX_DATA_MODE:

		if(rfm22_processTxInt())
			// Wake up the Tx task
			rfm22_return_from_ISR(g_rfm22b_dev->rxsem);
		break;

	default:
		// unknown mode - this should NEVER happen.
		rfm22_setRxMode(RX_WAIT_PREAMBLE_MODE);
		break;
	}
}

static bool rfm22_processRxInt(void)
{

	// FIFO under/over flow error.  Restart RX mode.
	if (device_status & (RFM22_ds_ffunfl | RFM22_ds_ffovfl))
	{
		rf_mode = RX_ERROR_MODE;
		return true;
	}

	// Valid preamble detected
	if (int_status2 & RFM22_is2_ipreaval && (rf_mode == RX_WAIT_PREAMBLE_MODE))
	{
		rf_mode = RX_WAIT_SYNC_MODE;
		RX_LED_ON;
	}

	// Sync word detected
	if (int_status2 & RFM22_is2_iswdet && ((rf_mode == RX_WAIT_PREAMBLE_MODE || rf_mode == RX_WAIT_SYNC_MODE)))
	{
		rf_mode = RX_DATA_MODE;
		RX_LED_ON;

		// read the 10-bit signed afc correction value
		// bits 9 to 2
		afc_correction = (uint16_t)rfm22_read(RFM22_afc_correction_read) << 8;
		// bits 1 & 0
		afc_correction |= (uint16_t)rfm22_read(RFM22_ook_counter_value1) & 0x00c0;
		afc_correction >>= 6;
		// convert the afc value to Hz
		afc_correction_Hz = (int32_t)(frequency_step_size * afc_correction + 0.5f);

		// remember the rssi for this packet
		rx_packet_start_rssi_dBm = rssi_dBm;
		// remember the afc value for this packet
		rx_packet_start_afc_Hz = afc_correction_Hz;
	}

	// RX FIFO almost full, it needs emptying
	if (int_status1 & RFM22_is1_irxffafull)
	{
		if (rf_mode == RX_DATA_MODE)
		{
			// read data from the rf chips FIFO buffer
			// read the total length of the packet data
			uint16_t len = rfm22_read(RFM22_received_packet_length);

			// The received packet is going to be larger than the specified length
			if ((g_rfm22b_dev->rx_packet_wr + RX_FIFO_HI_WATERMARK) > len)
			{
				rf_mode = RX_ERROR_MODE;
				return true;
			}

			// Another packet length error.
			if (((g_rfm22b_dev->rx_packet_wr + RX_FIFO_HI_WATERMARK) >= len) && !(int_status1 & RFM22_is1_ipkvalid))
			{
				rf_mode = RX_ERROR_MODE;
				return true;
			}

			// Fetch the data from the RX FIFO
			rfm22_startBurstRead(RFM22_fifo_access, true);
			for (uint8_t i = 0; i < RX_FIFO_HI_WATERMARK; ++i)
				((uint8_t*)(g_rfm22b_dev->rx_packet))[g_rfm22b_dev->rx_packet_wr++] = rfm22_burstRead();
			rfm22_endBurstRead();
		}
		else
		{	// just clear the RX FIFO
			rfm22_startBurstRead(RFM22_fifo_access, true);
			for (register uint16_t i = RX_FIFO_HI_WATERMARK; i > 0; i--)
				// read a byte from the rf modules RX FIFO buffer
				rfm22_burstRead();
			rfm22_endBurstRead();
		}
	}

	// CRC error .. discard the received data
	if (int_status1 & RFM22_is1_icrerror)
	{
		rf_mode = RX_ERROR_MODE;
		return true;
	}

	// Valid packet received
	if (int_status1 & RFM22_is1_ipkvalid)
	{

		// read the total length of the packet data
		register uint16_t len = rfm22_read(RFM22_received_packet_length);

		// their must still be data in the RX FIFO we need to get
		if (g_rfm22b_dev->rx_packet_wr < len)
		{
			// Fetch the data from the RX FIFO
			rfm22_startBurstRead(RFM22_fifo_access, true);
			while (g_rfm22b_dev->rx_packet_wr < len)
				((uint8_t*)(g_rfm22b_dev->rx_packet))[g_rfm22b_dev->rx_packet_wr++] = rfm22_burstRead();
			rfm22_endBurstRead();
		}

		// we have a packet length error .. discard the packet
		if (g_rfm22b_dev->rx_packet_wr != len)
		{
			rf_mode = RX_ERROR_MODE;
			return true;
		}

		// remember the rssi for this packet
		rx_packet_rssi_dBm = rx_packet_start_rssi_dBm;
		// remember the afc offset for this packet
		rx_packet_afc_Hz = rx_packet_start_afc_Hz;
		// Add the rssi and afc to the end of the packet.
		((uint8_t*)(g_rfm22b_dev->rx_packet))[g_rfm22b_dev->rx_packet_wr++] = rx_packet_start_rssi_dBm;
		((uint8_t*)(g_rfm22b_dev->rx_packet))[g_rfm22b_dev->rx_packet_wr++] = rx_packet_start_afc_Hz;

		// Copy the receive buffer into the receive packet.
		rf_mode = RX_COMPLETE_MODE;
		if ((g_rfm22b_dev->rx_packet_prev == 0) && (g_rfm22b_dev->rx_packet_next != 0))
		{
			g_rfm22b_dev->rx_packet_prev = g_rfm22b_dev->rx_packet;
			g_rfm22b_dev->rx_packet_len = g_rfm22b_dev->rx_packet_wr;
			g_rfm22b_dev->rx_packet = g_rfm22b_dev->rx_packet_next;
			g_rfm22b_dev->rx_packet_next = 0;				
			g_rfm22b_dev->rx_packet_wr = 0;

			return true;
		}
		g_rfm22b_dev->rx_packet_wr = 0;

		return true;
	}

	return false;
}

static bool rfm22_processTxInt(void)
{

	// FIFO under/over flow error.  Back to RX mode.
	if (device_status & (RFM22_ds_ffunfl | RFM22_ds_ffovfl))
	{
		rf_mode = TX_ERROR_MODE;
		return true;
	}

	// TX FIFO almost empty, it needs filling up
	if (int_status1 & RFM22_is1_ixtffaem)
	{
		// top-up the rf chips TX FIFO buffer
		uint16_t max_bytes = FIFO_SIZE - TX_FIFO_LO_WATERMARK - 1;
		rfm22_startBurstWrite(RFM22_fifo_access, true);
		for (uint16_t i = 0; (g_rfm22b_dev->tx_data_rd < g_rfm22b_dev->tx_data_wr) && (i < max_bytes); ++i, ++g_rfm22b_dev->tx_data_rd)
			rfm22_burstWrite(((uint8_t*)(g_rfm22b_dev->tx_packet))[g_rfm22b_dev->tx_data_rd]);
		rfm22_endBurstWrite();
	}

	// Packet has been sent
	if (int_status1 & RFM22_is1_ipksent)
	{
		rf_mode = TX_COMPLETE_MODE;
		return true;
	}

	return false;
}


// ************************************
// set/get the current tx power setting

void rfm22_setTxPower(uint8_t tx_pwr)
{
	switch (tx_pwr)
	{
	case 0: tx_power = RFM22_tx_pwr_txpow_0; break;    // +1dBm ... 1.25mW
	case 1: tx_power = RFM22_tx_pwr_txpow_1; break;    // +2dBm ... 1.6mW
	case 2: tx_power = RFM22_tx_pwr_txpow_2; break;    // +5dBm ... 3.16mW
	case 3: tx_power = RFM22_tx_pwr_txpow_3; break;    // +8dBm ... 6.3mW
	case 4: tx_power = RFM22_tx_pwr_txpow_4; break;    // +11dBm .. 12.6mW
	case 5: tx_power = RFM22_tx_pwr_txpow_5; break;    // +14dBm .. 25mW
	case 6: tx_power = RFM22_tx_pwr_txpow_6; break;    // +17dBm .. 50mW
	case 7: tx_power = RFM22_tx_pwr_txpow_7; break;    // +20dBm .. 100mW
	default: break;
	}
}

uint8_t rfm22_getTxPower(void)
{
	return tx_power;
}

// ************************************

uint32_t rfm22_minFrequency(void)
{
	return lower_carrier_frequency_limit_Hz;
}
uint32_t rfm22_maxFrequency(void)
{
	return upper_carrier_frequency_limit_Hz;
}

void rfm22_setNominalCarrierFrequency(uint32_t frequency_hz)
{

	// *******

	if (frequency_hz < lower_carrier_frequency_limit_Hz)
		frequency_hz = lower_carrier_frequency_limit_Hz;
	else if (frequency_hz > upper_carrier_frequency_limit_Hz)
		frequency_hz = upper_carrier_frequency_limit_Hz;

	if (frequency_hz < 480000000)
		hbsel = 1;
	else
		hbsel = 2;
	uint8_t fb = (uint8_t)(frequency_hz / (10000000 * hbsel));

	uint32_t fc = (uint32_t)(frequency_hz - (10000000 * hbsel * fb));

	fc = (fc * 64u) / (10000ul * hbsel);
	fb -= 24;

	//	carrier_frequency_hz = frequency_hz;
	carrier_frequency_hz = ((uint32_t)fb + 24 + ((float)fc / 64000)) * 10000000 * hbsel;

	if (hbsel > 1)
		fb |= RFM22_fbs_hbsel;

	fb |= RFM22_fbs_sbse;	// is this the RX LO polarity?

	frequency_step_size = 156.25f * hbsel;

	// frequency hopping channel (0-255)
	rfm22_write(RFM22_frequency_hopping_channel_select, frequency_hop_channel);

	// no frequency offset
	rfm22_write(RFM22_frequency_offset1, 0);
	rfm22_write(RFM22_frequency_offset2, 0);

	// set the carrier frequency
	rfm22_write(RFM22_frequency_band_select, fb);
	rfm22_write(RFM22_nominal_carrier_frequency1, fc >> 8);
	rfm22_write(RFM22_nominal_carrier_frequency0, fc & 0xff);
}

uint32_t rfm22_getNominalCarrierFrequency(void)
{
	return carrier_frequency_hz;
}

float rfm22_getFrequencyStepSize(void)
{
	return frequency_step_size;
}

void rfm22_setFreqHopChannel(uint8_t channel)
{	// set the frequency hopping channel
	frequency_hop_channel = channel;
	rfm22_write(RFM22_frequency_hopping_channel_select, frequency_hop_channel);
}

uint8_t rfm22_freqHopChannel(void)
{	// return the current frequency hopping channel
	return frequency_hop_channel;
}

uint32_t rfm22_freqHopSize(void)
{	// return the frequency hopping step size
	return ((uint32_t)frequency_hop_step_size_reg * 10000);
}

// ************************************
// radio datarate about 19200 Baud
// radio frequency deviation 45kHz
// radio receiver bandwidth 67kHz.
//
// Carson's rule:
//  The signal bandwidth is about 2(Delta-f + fm) ..
//
// Delta-f = frequency deviation
// fm = maximum frequency of the signal
//
// This gives 2(45 + 9.6) = 109.2kHz.

void rfm22_setDatarate(uint32_t datarate_bps, bool data_whitening)
{

	int lookup_index = 0;
	while (lookup_index < (LOOKUP_SIZE - 1) && data_rate[lookup_index] < datarate_bps)
		lookup_index++;

	carrier_datarate_bps = datarate_bps = data_rate[lookup_index];

	rf_bandwidth_used = rx_bandwidth[lookup_index];

	// rfm22_if_filter_bandwidth
	rfm22_write(0x1C, reg_1C[lookup_index]);

	// rfm22_afc_loop_gearshift_override
	rfm22_write(0x1D, reg_1D[lookup_index]);
	// RFM22_afc_timing_control
	rfm22_write(0x1E, reg_1E[lookup_index]);

	// RFM22_clk_recovery_gearshift_override
	rfm22_write(0x1F, reg_1F[lookup_index]);
	// rfm22_clk_recovery_oversampling_ratio
	rfm22_write(0x20, reg_20[lookup_index]);
	// rfm22_clk_recovery_offset2
	rfm22_write(0x21, reg_21[lookup_index]);
	// rfm22_clk_recovery_offset1
	rfm22_write(0x22, reg_22[lookup_index]);
	// rfm22_clk_recovery_offset0
	rfm22_write(0x23, reg_23[lookup_index]);
	// rfm22_clk_recovery_timing_loop_gain1
	rfm22_write(0x24, reg_24[lookup_index]);
	// rfm22_clk_recovery_timing_loop_gain0
	rfm22_write(0x25, reg_25[lookup_index]);

	// rfm22_afc_limiter
	rfm22_write(0x2A, reg_2A[lookup_index]);

	if (carrier_datarate_bps < 100000)
		// rfm22_chargepump_current_trimming_override
		rfm22_write(0x58, 0x80);
	else
		// rfm22_chargepump_current_trimming_override
		rfm22_write(0x58, 0xC0);

	// rfm22_tx_data_rate1
	rfm22_write(0x6E, reg_6E[lookup_index]);
	// rfm22_tx_data_rate0
	rfm22_write(0x6F, reg_6F[lookup_index]);

	if (!data_whitening)
		// rfm22_modulation_mode_control1
		rfm22_write(0x70, reg_70[lookup_index] & ~RFM22_mmc1_enwhite);
	else
		// rfm22_modulation_mode_control1
		rfm22_write(0x70, reg_70[lookup_index] |  RFM22_mmc1_enwhite);

	// rfm22_modulation_mode_control2
	rfm22_write(0x71, reg_71[lookup_index]);

	// rfm22_frequency_deviation
	rfm22_write(0x72, reg_72[lookup_index]);

	rfm22_write(RFM22_ook_counter_value1, 0x00);
	rfm22_write(RFM22_ook_counter_value2, 0x00);

	// milliseconds
	timeout_data_ms = (8000ul * 100) / carrier_datarate_bps;
	if (timeout_data_ms < 3)
		// because out timer resolution is only 1ms
		timeout_data_ms = 3;
}

uint32_t rfm22_getDatarate(void)
{
	return carrier_datarate_bps;
}

// ************************************

static void rfm22_setRxMode(uint8_t mode)
{
	// disable interrupts
	rfm22_write(RFM22_interrupt_enable1, 0x00);
	rfm22_write(RFM22_interrupt_enable2, 0x00);

	// Switch to TUNE mode
	rfm22_write(RFM22_op_and_func_ctrl1, RFM22_opfc1_pllon);

	RX_LED_OFF;
	TX_LED_OFF;

	// empty the rx buffer
	g_rfm22b_dev->rx_packet_wr = 0;
	rf_mode = mode;

	// Clear the TX buffer.
	g_rfm22b_dev->tx_data_rd = g_rfm22b_dev->tx_data_wr = 0;

	// clear FIFOs
	rfm22_write(RFM22_op_and_func_ctrl2, RFM22_opfc2_ffclrrx | RFM22_opfc2_ffclrtx);
	rfm22_write(RFM22_op_and_func_ctrl2, 0x00);

	// enable RX interrupts
	rfm22_write(RFM22_interrupt_enable1, RFM22_ie1_encrcerror | RFM22_ie1_enpkvalid |
		    RFM22_ie1_enrxffafull | RFM22_ie1_enfferr);
	rfm22_write(RFM22_interrupt_enable2, RFM22_ie2_enpreainval | RFM22_ie2_enpreaval |
		    RFM22_ie2_enswdet);

	// enable the receiver
	rfm22_write(RFM22_op_and_func_ctrl1, RFM22_opfc1_pllon | RFM22_opfc1_rxon);
}

// ************************************

uint8_t rfm22_txStart(struct pios_rfm22b_dev *rfm22b_dev)
{

	// Initialize the supervisor timer.
	rfm22b_dev_g->supv_timer = PIOS_RFM22B_SUPERVISOR_TIMEOUT;

	// Disable interrrupts.
	PIOS_IRQ_Disable();

	// disable interrupts
	rfm22_write(RFM22_interrupt_enable1, 0x00);
	rfm22_write(RFM22_interrupt_enable2, 0x00);

	// Re-ensable interrrupts.
	PIOS_IRQ_Enable();

	// TUNE mode
	rfm22_write(RFM22_op_and_func_ctrl1, RFM22_opfc1_pllon);

	// Queue the data up for sending
	rfm22b_dev->tx_data_rd = 0;
	rfm22b_dev->tx_data_wr = PH_PACKET_SIZE(rfm22b_dev->tx_packet);

	RX_LED_OFF;

	// Set the destination address in the transmit header.
	// The destination address is the first 4 bytes of the message.
	rfm22_write(RFM22_transmit_header0, ((uint8_t*)(rfm22b_dev->tx_packet))[0]);
	rfm22_write(RFM22_transmit_header1, ((uint8_t*)(rfm22b_dev->tx_packet))[1]);
	rfm22_write(RFM22_transmit_header2, ((uint8_t*)(rfm22b_dev->tx_packet))[2]);
	rfm22_write(RFM22_transmit_header3, ((uint8_t*)(rfm22b_dev->tx_packet))[3]);

	// FIFO mode, GFSK modulation
	uint8_t fd_bit = rfm22_read(RFM22_modulation_mode_control2) & RFM22_mmc2_fd;
	rfm22_write(RFM22_modulation_mode_control2, fd_bit | RFM22_mmc2_dtmod_fifo |
		    RFM22_mmc2_modtyp_gfsk);

	// set the tx power
	rfm22_write(RFM22_tx_power, RFM22_tx_pwr_papeaken | RFM22_tx_pwr_papeaklvl_1 |
		    RFM22_tx_pwr_papeaklvl_0 | RFM22_tx_pwr_lna_sw | tx_power);

	// clear FIFOs
	rfm22_write(RFM22_op_and_func_ctrl2, RFM22_opfc2_ffclrrx | RFM22_opfc2_ffclrtx);
	rfm22_write(RFM22_op_and_func_ctrl2, 0x00);

	// *******************
	// add some data to the chips TX FIFO before enabling the transmitter

	// set the total number of data bytes we are going to transmit
	rfm22_write(RFM22_transmit_packet_length, rfm22b_dev->tx_data_wr);

	// add some data
	rfm22_claimBus();
	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 0);
	PIOS_SPI_TransferByte(PIOS_RFM22_SPI_PORT, RFM22_fifo_access | 0x80);
	int bytes_to_write = (rfm22b_dev->tx_data_wr - rfm22b_dev->tx_data_rd);
	bytes_to_write = (bytes_to_write > FIFO_SIZE) ? FIFO_SIZE:  bytes_to_write;
	PIOS_SPI_TransferBlock(PIOS_RFM22_SPI_PORT, (uint8_t*)(rfm22b_dev->tx_packet) + rfm22b_dev->tx_data_rd, NULL, bytes_to_write, NULL);
	rfm22b_dev->tx_data_rd += bytes_to_write;
	PIOS_SPI_RC_PinSet(PIOS_RFM22_SPI_PORT, 0, 1);
	rfm22_releaseBus();

	// *******************

	rf_mode = TX_DATA_MODE;

	// enable TX interrupts
	rfm22_write(RFM22_interrupt_enable1, RFM22_ie1_enpksent | RFM22_ie1_entxffaem);

	// enable the transmitter
	rfm22_write(RFM22_op_and_func_ctrl1, RFM22_opfc1_pllon | RFM22_opfc1_txon);


	TX_LED_ON;

	return 1;
}

// ************************************

int8_t rfm22_getRSSI(void)
{
	// read rx signal strength .. 45 = -100dBm, 205 = -20dBm
	rssi = rfm22_read(RFM22_rssi);
	// convert to dBm
	rssi_dBm = (int8_t)(rssi >> 1) - 122;
	return rssi_dBm;
}

int8_t rfm22_receivedRSSI(void)
{	// return the packets signal strength
	if (!initialized)
		return -127;
	else
		return rx_packet_rssi_dBm;
}

int32_t rfm22_receivedAFCHz(void)
{	// return the packets offset frequency
	if (!initialized)
		return 0;
	else
		return rx_packet_afc_Hz;
}

// ************************************

// return the current mode
int8_t rfm22_currentMode(void)
{
	return rf_mode;
}

// return true if we are transmitting
bool rfm22_transmitting(void)
{
	return (rf_mode == TX_DATA_MODE);
}

// return true if the channel is clear to transmit on
bool rfm22_channelIsClear(void)
{
	// we haven't yet been initialized
	if (!initialized)
		return false;

	// we are receiving something or we are transmitting or we are scanning the spectrum
	if (rf_mode != RX_WAIT_PREAMBLE_MODE && rf_mode != RX_WAIT_SYNC_MODE)
		return false;

	return true;
}

// ************************************
// set/get the frequency calibration value

void rfm22_setFreqCalibration(uint8_t value)
{
	osc_load_cap = value;

	// we haven't yet been initialized
	if (!initialized || power_on_reset)
		return;

	rfm22_write(RFM22_xtal_osc_load_cap, osc_load_cap);
}

uint8_t rfm22_getFreqCalibration(void)
{
	return osc_load_cap;
}

// ************************************
// reset the RF module

int rfm22_resetModule(uint8_t mode, uint32_t min_frequency_hz, uint32_t max_frequency_hz)
{
	initialized = false;

	power_on_reset = false;

	// ****************
	// software reset the RF chip .. following procedure according to Si4x3x Errata (rev. B)
	rfm22_write(RFM22_op_and_func_ctrl1, RFM22_opfc1_swres);

	// wait 26ms
	PIOS_DELAY_WaitmS(26);

	for (int i = 50; i > 0; i--)
	{
		// wait 1ms
		PIOS_DELAY_WaitmS(1);

		// read the status registers
		int_status1 = rfm22_read(RFM22_interrupt_status1);
		int_status2 = rfm22_read(RFM22_interrupt_status2);
		if (int_status2 & RFM22_is2_ichiprdy) break;
	}

	// ****************

	// read status - clears interrupt
	device_status = rfm22_read(RFM22_device_status);
	int_status1 = rfm22_read(RFM22_interrupt_status1);
	int_status2 = rfm22_read(RFM22_interrupt_status2);

	// disable all interrupts
	rfm22_write(RFM22_interrupt_enable1, 0x00);
	rfm22_write(RFM22_interrupt_enable2, 0x00);

	// ****************

	rf_mode = mode;

	device_status = int_status1 = int_status2 = 0;

	rssi = 0;
	rssi_dBm = -127;

	g_rfm22b_dev->rx_packet_wr = 0;
	rx_packet_rssi_dBm = -127;
	rx_packet_afc_Hz = 0;

	g_rfm22b_dev->tx_data_rd = g_rfm22b_dev->tx_data_wr = 0;

	rf_bandwidth_used = 0;
	ss_rf_bandwidth_used = 0;

	hbsel = 0;
	frequency_step_size = 0.0f;

	frequency_hop_channel = 0;

	afc_correction = 0;
	afc_correction_Hz = 0;

	temperature_reg = 0;

	// ****************
	// read the RF chip ID bytes

	device_type = rfm22_read(RFM22_DEVICE_TYPE) & RFM22_DT_MASK;		// read the device type
	device_version = rfm22_read(RFM22_DEVICE_VERSION) & RFM22_DV_MASK;	// read the device version

#if defined(RFM22_DEBUG)
	DEBUG_PRINTF(2, "rf device type: %d\n\r", device_type);
	DEBUG_PRINTF(2, "rf device version: %d\n\r", device_version);
#endif

	if (device_type != 0x08)
	{
#if defined(RFM22_DEBUG)
		DEBUG_PRINTF(2, "rf device type: INCORRECT - should be 0x08\n\r");
#endif
		return -1;	// incorrect RF module type
	}

	if (device_version != RFM22_DEVICE_VERSION_B1)	// B1
	{
#if defined(RFM22_DEBUG)
		DEBUG_PRINTF(2, "rf device version: INCORRECT\n\r");
#endif
		return -2;	// incorrect RF module version
	}

	// ****************
	// set the minimum and maximum carrier frequency allowed

	if (min_frequency_hz < RFM22_MIN_CARRIER_FREQUENCY_HZ) min_frequency_hz = RFM22_MIN_CARRIER_FREQUENCY_HZ;
	else
		if (min_frequency_hz > RFM22_MAX_CARRIER_FREQUENCY_HZ) min_frequency_hz = RFM22_MAX_CARRIER_FREQUENCY_HZ;

	if (max_frequency_hz < RFM22_MIN_CARRIER_FREQUENCY_HZ) max_frequency_hz = RFM22_MIN_CARRIER_FREQUENCY_HZ;
	else
		if (max_frequency_hz > RFM22_MAX_CARRIER_FREQUENCY_HZ) max_frequency_hz = RFM22_MAX_CARRIER_FREQUENCY_HZ;

	if (min_frequency_hz > max_frequency_hz)
	{	// swap them over
		uint32_t tmp = min_frequency_hz;
		min_frequency_hz = max_frequency_hz;
		max_frequency_hz = tmp;
	}

	lower_carrier_frequency_limit_Hz = min_frequency_hz;
	upper_carrier_frequency_limit_Hz = max_frequency_hz;

	// ****************
	// calibrate our RF module to be exactly on frequency .. different for every module

	osc_load_cap = OSC_LOAD_CAP;	// default
	rfm22_write(RFM22_xtal_osc_load_cap, osc_load_cap);

	// ****************

	// disable Low Duty Cycle Mode
	rfm22_write(RFM22_op_and_func_ctrl2, 0x00);

	rfm22_write(RFM22_cpu_output_clk, RFM22_coc_1MHz);						// 1MHz clock output

	rfm22_write(RFM22_op_and_func_ctrl1, RFM22_opfc1_xton);					// READY mode
	//	rfm22_write(RFM22_op_and_func_ctrl1, RFM22_opfc1_pllon);				// TUNE mode

	// choose the 3 GPIO pin functions
	rfm22_write(RFM22_io_port_config, RFM22_io_port_default);								// GPIO port use default value
	rfm22_write(RFM22_gpio0_config, RFM22_gpio0_config_drv3 | RFM22_gpio0_config_txstate);	// GPIO0 = TX State (to control RF Switch)
	rfm22_write(RFM22_gpio1_config, RFM22_gpio1_config_drv3 | RFM22_gpio1_config_rxstate);	// GPIO1 = RX State (to control RF Switch)
	rfm22_write(RFM22_gpio2_config, RFM22_gpio2_config_drv3 | RFM22_gpio2_config_cca);		// GPIO2 = Clear Channel Assessment

	// ****************

	return 0;	// OK
}

// ************************************
// Initialise this hardware layer module and the rf module

int rfm22_init_normal(uint32_t id, uint32_t min_frequency_hz, uint32_t max_frequency_hz, uint32_t freq_hop_step_size)
{
	int res = rfm22_resetModule(RX_WAIT_PREAMBLE_MODE, min_frequency_hz, max_frequency_hz);
	if (res < 0)
		return res;

	// initialize the frequency hopping step size
	freq_hop_step_size /= 10000;	// in 10kHz increments
	if (freq_hop_step_size > 255) freq_hop_step_size = 255;
	frequency_hop_step_size_reg = freq_hop_step_size;

	// set the RF datarate
	rfm22_setDatarate(RFM22_DEFAULT_RF_DATARATE, true);

	// FIFO mode, GFSK modulation
	uint8_t fd_bit = rfm22_read(RFM22_modulation_mode_control2) & RFM22_mmc2_fd;
	rfm22_write(RFM22_modulation_mode_control2, RFM22_mmc2_trclk_clk_none | RFM22_mmc2_dtmod_fifo | fd_bit | RFM22_mmc2_modtyp_gfsk);

	// setup to read the internal temperature sensor

	// ADC used to sample the temperature sensor
	adc_config = RFM22_ac_adcsel_temp_sensor | RFM22_ac_adcref_bg;
	rfm22_write(RFM22_adc_config, adc_config);

	// adc offset
	rfm22_write(RFM22_adc_sensor_amp_offset, 0);

	// temp sensor calibration .. –40C to +64C 0.5C resolution
	rfm22_write(RFM22_temp_sensor_calib, RFM22_tsc_tsrange0 | RFM22_tsc_entsoffs);

	// temp sensor offset
	rfm22_write(RFM22_temp_value_offset, 0);

	// start an ADC conversion
	rfm22_write(RFM22_adc_config, adc_config | RFM22_ac_adcstartbusy);

	// set the RSSI threshold interrupt to about -90dBm
	rfm22_write(RFM22_rssi_threshold_clear_chan_indicator, (-90 + 122) * 2);

	// enable the internal Tx & Rx packet handlers (without CRC)
	rfm22_write(RFM22_data_access_control, RFM22_dac_enpacrx | RFM22_dac_enpactx);

	// x-nibbles tx preamble
	rfm22_write(RFM22_preamble_length, TX_PREAMBLE_NIBBLES);
	// x-nibbles rx preamble detection
	rfm22_write(RFM22_preamble_detection_ctrl1, RX_PREAMBLE_NIBBLES << 3);

#ifdef PIOS_RFM22_NO_HEADER
	// header control - we are not using the header
	rfm22_write(RFM22_header_control1, RFM22_header_cntl1_bcen_none | RFM22_header_cntl1_hdch_none);

	// no header bytes, synchronization word length 3, 2, 1 & 0 used, packet length included in header.
	rfm22_write(RFM22_header_control2, RFM22_header_cntl2_hdlen_none |
		    RFM22_header_cntl2_synclen_3210 | ((TX_PREAMBLE_NIBBLES >> 8) & 0x01));
#else
	// header control - using a 4 by header with broadcast of 0xffffffff
	rfm22_write(RFM22_header_control1,
		    RFM22_header_cntl1_bcen_0 |
		    RFM22_header_cntl1_bcen_1 |
		    RFM22_header_cntl1_bcen_2 |
		    RFM22_header_cntl1_bcen_3 |
		    RFM22_header_cntl1_hdch_0 |
		    RFM22_header_cntl1_hdch_1 |
		    RFM22_header_cntl1_hdch_2 |
		    RFM22_header_cntl1_hdch_3);
	// Check all bit of all bytes of the header
	rfm22_write(RFM22_header_enable0, 0xff);
	rfm22_write(RFM22_header_enable1, 0xff);
	rfm22_write(RFM22_header_enable2, 0xff);
	rfm22_write(RFM22_header_enable3, 0xff);
	// Set the ID to be checked
	rfm22_write(RFM22_check_header0, id & 0xff);
	rfm22_write(RFM22_check_header1, (id >> 8) & 0xff);
	rfm22_write(RFM22_check_header2, (id >> 16) & 0xff);
	rfm22_write(RFM22_check_header3, (id >> 24) & 0xff);
	// 4 header bytes, synchronization word length 3, 2, 1 & 0 used, packet length included in header.
	rfm22_write(RFM22_header_control2,
		    RFM22_header_cntl2_hdlen_3210 |
		    RFM22_header_cntl2_synclen_3210 |
		    ((TX_PREAMBLE_NIBBLES >> 8) & 0x01));
#endif

	// sync word
	rfm22_write(RFM22_sync_word3, SYNC_BYTE_1);
	rfm22_write(RFM22_sync_word2, SYNC_BYTE_2);
	rfm22_write(RFM22_sync_word1, SYNC_BYTE_3);
	rfm22_write(RFM22_sync_word0, SYNC_BYTE_4);

	rfm22_write(RFM22_agc_override1, RFM22_agc_ovr1_agcen);

	// set frequency hopping channel step size (multiples of 10kHz)
	rfm22_write(RFM22_frequency_hopping_step_size, frequency_hop_step_size_reg);

	// set our nominal carrier frequency
	rfm22_setNominalCarrierFrequency((min_frequency_hz + max_frequency_hz) / 2);

	// set the tx power
	rfm22_write(RFM22_tx_power, RFM22_tx_pwr_papeaken | RFM22_tx_pwr_papeaklvl_0 |
		    RFM22_tx_pwr_lna_sw | tx_power);

	// TX FIFO Almost Full Threshold (0 - 63)
	rfm22_write(RFM22_tx_fifo_control1, TX_FIFO_HI_WATERMARK);

	// TX FIFO Almost Empty Threshold (0 - 63)
	rfm22_write(RFM22_tx_fifo_control2, TX_FIFO_LO_WATERMARK);

	// RX FIFO Almost Full Threshold (0 - 63)
	rfm22_write(RFM22_rx_fifo_control, RX_FIFO_HI_WATERMARK);

	initialized = true;

	return 0;	// ok
}

// ************************************

#endif

/**
 * @}
 * @}
 */
