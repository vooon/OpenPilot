/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DYNAMIXEL_SERVO Code for controlling Dynamixel Servos
 * @brief Deals with the hardware interface to the Dynamixel Servos
 * @{
 *
 * @file       pios_dyanamixel_servo.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      Driver for Dynamixel Servos attached to half-duplex USART
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

#if defined(PIOS_INCLUDE_DYNAMIXEL_SERVO)

static uint8_t PIOS_DYNAMIXEL_checksum (uint8_t * buf, uint8_t len);

#define DYNAMIXEL_SYNC 0xFF

enum pios_dynamixel_instruction {
	DYNAMIXEL_PING       = 0x01,
	DYNAMIXEL_READ_DATA  = 0x02,
	DYNAMIXEL_WRITE_DATA = 0x03,
	DYNAMIXEL_REG_WRITE  = 0x04,
	DYNAMIXEL_ACTION     = 0x05,
	DYNAMIXEL_RESET      = 0x06,
	DYNAMIXEL_SYNC_WRITE = 0x83,
};

bool PIOS_DYNAMIXEL_ReadInfo (uint8_t address, uint16_t * model, uint16_t * fw_version)
{
	PIOS_Assert(model);
	PIOS_Assert(fw_version);

	if (!PIOS_COM_DYNAMIXEL) {
		/* No configured dyanamixel bus */
		return false;
	}

	uint8_t message[] = {
		DYNAMIXEL_SYNC,
		DYNAMIXEL_SYNC,
		address,	      /* servo bus address */
		4,		      /* length */
		DYNAMIXEL_READ_DATA,  /* instruction */
		0x00,		      /* base register address */
		0x04,		      /* length to read */
		0,		      /* placeholder for checksum */
	};

	/* Fill in the checksum */
	message[sizeof(message) - 1] = PIOS_DYNAMIXEL_checksum (message + 2, sizeof(message) - 1 - 2);

	int32_t rc = PIOS_COM_SendBuffer(PIOS_COM_DYNAMIXEL, message, sizeof(message));
	if (rc != sizeof(message)) {
		return false;
	}

	/* FIXME: Wait for the response packet */

	return true;
}

#if 0
static bool PIOS_DYNAMIXEL_ReadResponse (uint8_t * id, uint8_t * error_val, uint8_t * response_data, uint16_t response_data_size, uint8_t max_wait_ms) {

	enum {
		DYNAMIXEL_STATE_SYNC_1,
		DYNAMIXEL_STATE_SYNC_2,
		DYNAMIXEL_STATE_ID,
		DYNAMIXEL_STATE_LEN,
		DYNAMIXEL_STATE_ERROR,
		DYNAMIXEL_STATE_DATA,
		DYNAMIXEL_STATE_CSUM,
	} curr_state = DYNAMIXEL_STATE_SYNC_1;

	bool packet_done = false;
	bool packet_good = false;
	bool error_indication = false;
	bool data_buf_overrun = false;

	uint16_t data_pos = 0;
	uint16_t len = 0;
	uint16_t csum = 0;

	while ((max_wait_ms > 0) && !packet_done) {
		uint8_t b;

		if (PIOS_COM_ReceiveBuffer(PIOS_COM_DYNAMIXEL, &b, 1, 1) == 0) {
			/* Timeout */
			max_wait_ms--;
			continue;
		}

		switch (curr_state) {
		case DYNAMIXEL_STATE_SYNC_1:
			if (b == DYNAMIXEL_SYNC) {
				curr_state = DYNAMIXEL_STATE_SYNC_2;
			}
			break;
		case DYNAMIXEL_STATE_SYNC_2:
			if (b == DYNAMIXEL_SYNC) {
				curr_state = DYNAMIXEL_STATE_ID;
			} else {
				curr_state = DYNAMIXEL_STATE_SYNC_1;
			}
			break;
		case DYNAMIXEL_STATE_ID:
			if (id) {
				*id = b;
			}
			curr_state = DYNAMIXEL_STATE_LEN;
			csum += b;
			break;
		case DYNAMIXEL_STATE_LEN:
			len = b;
			if (len) {
				curr_state = DYNAMIXEL_STATE_ERROR;
			} else {
				curr_state = DYNAMIXEL_STATE_CSUM;
			}
			csum += b;
			break;
		case DYNAMIXEL_STATE_ERROR:
			if (b) {
				error_indication = true;
			}
			if (error_val) {
				*error_val = b;
			}
			curr_state = DYNAMIXEL_STATE_DATA;
			csum += b;
			break;
		case DYNAMIXEL_STATE_DATA:
			if (response_data) {
				/* Only look for overruns if we have a response data buffer */
				if (data_pos < response_data_size) {
					response_data[data_pos] = b;
				} else {
					data_buf_overrun = true;
				}
			}
			data_pos++;

			if (data_pos >= len) {
				curr_state = DYNAMIXEL_STATE_CSUM;
			}
			csum += b;
			break;
		case DYNAMIXEL_STATE_CSUM:
			if ((csum & 0xFF) == b) {
				/* Checksum passed */
				packet_good = true;
			} else {
				/* Checksum failed */
				packet_good = false;
			}
			packet_done = true;
			break;
		}
	}

	return (packet_done && packet_good && !data_buf_overrun);
}

#endif

bool PIOS_DYNAMIXEL_SetPosition (uint8_t address, uint16_t position)
{
	if (!PIOS_COM_DYNAMIXEL) {
		/* No configured dyanamixel bus */
		return false;
	}

	const uint16_t rpm = 512; /* 57RPM ? */

	uint8_t message[] = {
		DYNAMIXEL_SYNC,
		DYNAMIXEL_SYNC,
		address,	      /* servo bus address */
		7,		      /* length */
		DYNAMIXEL_WRITE_DATA, /* instruction */
		0x1E,		      /* base register address */
		/* Goal Position (0x1E) */
		(position & 0x00FF) >> 0,
		(position & 0xFF00) >> 8,
		/* Moving Speed (0x20) */
		(rpm & 0x00FF) >> 0,
		(rpm & 0xFF00) >> 8,
		/* Placeholder for checksum */
		0,
	};

	/* Fill in the checksum */
	message[sizeof(message) - 1] = PIOS_DYNAMIXEL_checksum (message + 2, sizeof(message) - 1 - 2);

	int32_t rc = PIOS_COM_SendBuffer(PIOS_COM_DYNAMIXEL, message, sizeof(message));
	
	if (rc != sizeof(message)) {
		return false;
	}

#if 0
	/* Packet sent OK, parse reply */
	uint8_t error_val = 0;
	if (!PIOS_DYNAMIXEL_ReadResponse(NULL, &error_val, NULL, 0, 1)) {
		/* Failed to read a valid response packet */
		return false;
	}

	if (error_val) {
		/* Got an error packet, parse it */
		return false;
	}
#endif
	return true;
}


static uint8_t PIOS_DYNAMIXEL_checksum (uint8_t * buf, uint8_t len)
{
	uint8_t cs = 0;

	/* compute 8-bit sum over buffer */
	for (uint8_t i = 0; i < len; i++) {
		cs += *buf;
		buf++;
	}

	/* Invert the sum */
	cs = ~cs;

	return cs;
}

#endif	/* PIOS_INCLUDE_DYNAMIXEL_SERVO */

/**
 * @}
 * @}
 */
