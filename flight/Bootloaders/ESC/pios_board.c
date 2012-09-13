/**
 ******************************************************************************
 *
 * @file       pios_board.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Defines board specific static initializers for hardware for the PipBee board.
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

#include <pios.h>
#include "board_hw_defs.c"

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */
static bool board_init_complete = false;
void PIOS_Board_Init(void) {
	if (board_init_complete) {
		return;
	}

	PIOS_LED_Init(&pios_led_cfg);

	/* Enable Prefetch Buffer */
	FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

	/* Flash 2 wait state */
	FLASH_SetLatency(FLASH_Latency_2);

	/* Delay system */
	PIOS_DELAY_Init();

	/* Initialize the PiOS library */
	PIOS_GPIO_Init();

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);//TODO Tirar

	// Initialize the components required for the softusart

	PIOS_TIM_InitClock(&tim_4_cfg);
	/* Install the callbacks for hardware timing */

#if defined(PIOS_INCLUDE_SOFTUSART)
	uint32_t pios_softusart_id;
	if (PIOS_SOFTUSART_Init(&pios_softusart_id, &pios_softusart_cfg)) {
		PIOS_Assert(0);
	}
	
	if (PIOS_COM_Init(&pios_com_softusart_id, &pios_softusart_com_driver, pios_softusart_id,
					  uart_softusart_rx_buffer, sizeof(uart_softusart_rx_buffer),
					  uart_softusart_tx_buffer, sizeof(uart_softusart_tx_buffer))) {
		PIOS_Assert(0);
	}
#endif


	board_init_complete = true;
}
