/**
******************************************************************************
* @addtogroup PIOS PIOS Core hardware abstraction layer
* @{
* @addtogroup   PIOS_RFM22B_COM Radio Functions
* @brief PIOS COM interface for for the RFM22B radio
* @{
*
* @file       pios_rfm22b_com.c
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



/* Project Includes */
#include "pios.h"

#if defined(PIOS_INCLUDE_RFM22B_COM)

#include <pios_usart_priv.h>

/* Provide a COM driver */
static void PIOS_RFM22B_COM_ChangeBaud(uint32_t rfm22b_com_id, uint32_t baud);
static void PIOS_RFM22B_COM_RegisterRxCallback(uint32_t rfm22b_com_id, pios_com_callback rx_in_cb, uint32_t context);
static void PIOS_RFM22B_COM_RegisterTxCallback(uint32_t rfm22b_com_id, pios_com_callback tx_out_cb, uint32_t context);
static void PIOS_RFM22B_COM_TxStart(uint32_t rfm22b_com_id, uint16_t tx_bytes_avail);
static void PIOS_RFM22B_COM_RxStart(uint32_t rfm22b_com_id, uint16_t rx_bytes_avail);
static bool PIOS_RFM22B_COM_Available(uint32_t rfm22b_com_id);

const struct pios_com_driver pios_rfm22b_com_driver = {
	.set_baud   = PIOS_RFM22B_COM_ChangeBaud,
	.tx_start   = PIOS_RFM22B_COM_TxStart,
	.rx_start   = PIOS_RFM22B_COM_RxStart,
	.bind_tx_cb = PIOS_RFM22B_COM_RegisterTxCallback,
	.bind_rx_cb = PIOS_RFM22B_COM_RegisterRxCallback,
	.available  = PIOS_RFM22B_COM_Available,
};

enum pios_rfm22b_com_dev_magic {
	PIOS_RFM22B_COM_DEV_MAGIC = 0x61538F4A,
};

struct pios_rfm22b_com_dev {
	enum pios_rfm22b_com_dev_magic magic;

	uint32_t rfm22b_id;

	pios_com_callback rx_in_cb;
	uint32_t rx_in_context;
	pios_com_callback tx_out_cb;
	uint32_t tx_out_context;
};

/* Local methods */
static void rfm22b_com_receivePacketData(uint8_t *buf, uint8_t len, uint32_t context);

static bool PIOS_RFM22B_COM_validate(struct pios_rfm22b_com_dev * rfm22b_com_dev)
{
	return (rfm22b_com_dev->magic == PIOS_RFM22B_COM_DEV_MAGIC);
}

#if defined(PIOS_INCLUDE_FREERTOS)
static struct pios_rfm22b_com_dev * PIOS_RFM22B_COM_alloc(void)
{
	struct pios_rfm22b_com_dev * rfm22b_com_dev;

	rfm22b_com_dev = (struct pios_rfm22b_com_dev *)pvPortMalloc(sizeof(*rfm22b_com_dev));
	if (!rfm22b_com_dev) return(NULL);

	rfm22b_com_dev->rx_in_cb = 0;
	rfm22b_com_dev->rx_in_context = 0;
	rfm22b_com_dev->tx_out_cb = 0;
	rfm22b_com_dev->tx_out_context = 0;
	rfm22b_com_dev->magic = PIOS_RFM22B_COM_DEV_MAGIC;
	return(rfm22b_com_dev);
}
#else
#error FREERTOS REQUIRED
#endif

/**
* Initialise a single USART device
*/
int32_t PIOS_RFM22B_COM_Init(uint32_t * rfm22b_com_id, uint32_t rfm22b_id)
{
	PIOS_DEBUG_Assert(rfm22b_id);

	struct pios_rfm22b_com_dev * rfm22b_com_dev;

	rfm22b_com_dev = (struct pios_rfm22b_com_dev *) PIOS_RFM22B_COM_alloc();
	if (!rfm22b_com_dev) goto out_fail;

	/* Bind the configuration to the device instance */
	rfm22b_com_dev->rfm22b_id = rfm22b_id;
	*rfm22b_com_id = (uint32_t) rfm22b_com_dev;

	// TODO: Install a callback to this driver whenever a RFM22b data packet is
	// sent or received

	// The receive data handler
	PHRegisterDataHandler(PIOS_PACKET_HANDLER, rfm22b_com_receivePacketData, *rfm22b_com_id);
	return(0);

out_fail:
	return(-1);
}

//! Enable reception of data packets
static void PIOS_RFM22B_COM_RxStart(uint32_t rfm22b_com_id, uint16_t rx_bytes_avail)
{
	struct pios_rfm22b_com_dev * rfm22b_com_dev = (struct pios_rfm22b_com_dev *)rfm22b_com_id;
	
	bool valid = PIOS_RFM22B_COM_validate(rfm22b_com_dev);
	PIOS_Assert(valid);

	// TODO: If 
	//if (rfm22b_com_dev->rx_in_cb) {
	//	(void) (rfm22b_com_dev->rx_in_cb)(rfm22b_com_dev->rx_in_context, &byte, 1, NULL, &rx_need_yield);
	//if (rx_need_yield)
	//	vPortYieldFromISR();
}

//! Enable transmission of data packets
static void PIOS_RFM22B_COM_TxStart(uint32_t rfm22b_com_id, uint16_t tx_bytes_avail)
{
	struct pios_rfm22b_com_dev * rfm22b_com_dev = (struct pios_rfm22b_com_dev *)rfm22b_com_id;
	
	bool valid = PIOS_RFM22B_COM_validate(rfm22b_com_dev);
	PIOS_Assert(valid);
	
	// TODO: If data is pending, send some
	if (rfm22b_com_dev->tx_out_cb) {
		bool tx_need_yield;
		const uint32_t MAX_SIZE = 40;
		uint8_t buffer[MAX_SIZE];

		// Copy data out of the com fifo that should be sent
	    uint32_t bytes_to_send = (rfm22b_com_dev->tx_out_cb)(rfm22b_com_dev->tx_out_context, buffer, MAX_SIZE, NULL, &tx_need_yield);
	    if (bytes_to_send)
	    	PHTransmitData(PIOS_PACKET_HANDLER, buffer, bytes_to_send);
		if (tx_need_yield)
			vPortYieldFromISR();
	}

}

/**
* Changes the RF baud rate of the RFM22b device.
* \param[in] rfm22b_com_id The com device id
* \param[in] baud Requested baud rate
*/
static void PIOS_RFM22B_COM_ChangeBaud(uint32_t rfm22b_com_id, uint32_t baud)
{
	struct pios_rfm22b_com_dev * rfm22b_com_dev = (struct pios_rfm22b_com_dev *)rfm22b_com_id;
	
	bool valid = PIOS_RFM22B_COM_validate(rfm22b_com_dev);
	PIOS_Assert(valid);

	// TODO: Change the baud rate of the RFM22b device
}

static void PIOS_RFM22B_COM_RegisterRxCallback(uint32_t rfm22b_com_id, pios_com_callback rx_in_cb, uint32_t context)
{
	struct pios_rfm22b_com_dev * rfm22b_com_dev = (struct pios_rfm22b_com_dev *)rfm22b_com_id;
	
	bool valid = PIOS_RFM22B_COM_validate(rfm22b_com_dev);
	PIOS_Assert(valid);
	
	/* 
	 * Order is important in these assignments since ISR uses _cb
	 * field to determine if it's ok to dereference _cb and _context
	 */
	rfm22b_com_dev->rx_in_context = context;
	rfm22b_com_dev->rx_in_cb = rx_in_cb;
}

static void PIOS_RFM22B_COM_RegisterTxCallback(uint32_t rfm22b_com_id, pios_com_callback tx_out_cb, uint32_t context)
{
	struct pios_rfm22b_com_dev * rfm22b_com_dev = (struct pios_rfm22b_com_dev *)rfm22b_com_id;
	
	bool valid = PIOS_RFM22B_COM_validate(rfm22b_com_dev);
	PIOS_Assert(valid);
	
	/* 
	 * Order is important in these assignments since ISR uses _cb
	 * field to determine if it's ok to dereference _cb and _context
	 */
	rfm22b_com_dev->tx_out_context = context;
	rfm22b_com_dev->tx_out_cb = tx_out_cb;
}

static bool PIOS_RFM22B_COM_Available(uint32_t rfm22b_com_id)
{
	// TODO: Check link status, true if connected
	return false;
}

/**
 * Callback from the packet handler which adds data to the PIOS_COM FIFO
 */
static void rfm22b_com_receivePacketData(uint8_t *buf, uint8_t len, uint32_t context)
{
	struct pios_rfm22b_com_dev * rfm22b_com_dev = (struct pios_rfm22b_com_dev *)context;
	
	bool valid = PIOS_RFM22B_COM_validate(rfm22b_com_dev);
	PIOS_Assert(valid);

	bool rx_need_yield;
	(void) (rfm22b_com_dev->rx_in_cb)(rfm22b_com_dev->rx_in_context, buf, len, NULL, &rx_need_yield);
	if (rx_need_yield)
		vPortYieldFromISR();
}

#endif /* PIOS_INCLUDE_RFM22B_COM */

/**
  * @}
  * @}
  */