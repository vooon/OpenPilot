/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_RCTX RCTX Functions
 * @brief PIOS interface to read and write from USB/HID port
 * @{
 *
 * @file       pios_rctx_priv.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      rctx private structures.
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

#ifndef PIOS_RCTX_PRIV_H
#define PIOS_RCTX_PRIV_H

#include <pios.h>

//-------------------------
// Receiver r/c transmitter input
//-------------------------
#define RCTX_PIOS_NUM_INPUTS 8
#define RCTX_HIDRAW_STRING_MAX 30
#define RCTX_TASK_PRIORITY 3
#define RCTX_LOST_CONNECTION_TIME_MS_MAX 2000 /* give 2 sec of grace periode before giving back control to GCS */

//#define RCTX_DEBUG

#ifdef RCTX_DEBUG
struct timeval start_time, end_time;
#endif

/* all errors here */
enum pios_rctx_err
{
	pios_rctx_udev_create_err = 0xDEAD0001,
	pios_rctx_udev_parent_err,
	pios_rctx_transmitter_not_found,
	pios_rctx_transmitter_unplugged_from_adapter,
	pios_rctx_rressource_unavailable,
	pios_rctx_wressource_unavailable,
	pios_rctx_trainer_enabled,
};

/* structure to hold infos on specific USB adapter */
struct pios_rctx_device {
	uint32_t vendor_id;
	uint32_t product_id;
	uint32_t periode_ms;          /* periode of execution of the transmitter callback (in ms) */
	int (*TX_Input_Get)(void);    /* transmitter callback to be executed periodically */
	int (*TX_Status_Get)(void);   /* transmitter connection status */
};

#endif /* PIOS_RCTX_PRIV_H */

/**
 * @}
 * @}
 */
