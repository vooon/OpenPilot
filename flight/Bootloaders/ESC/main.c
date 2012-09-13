/**
 ******************************************************************************
 * @addtogroup OpenPilotBL OpenPilot BootLoader
 * @brief These files contain the code to the OpenPilot MB Bootloader.
 *
 * @{
 * @file       main.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      This is the file with the main function of the OpenPilot BootLoader
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
/* Bootloader Includes */
#include <pios.h>
#include <pios_board_info.h>
#include "op_dfu.h"
#include "usb_lib.h"
#include "pios_iap.h"
#include "fifo_buffer.h"
/* Prototype of PIOS_Board_Init() function */
extern void PIOS_Board_Init(void);

/* Private typedef -----------------------------------------------------------*/
typedef void (*pFunction)(void);
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction Jump_To_Application;
uint32_t JumpAddress;

/* Extern variables ----------------------------------------------------------*/
DFUStates DeviceState;
/* Private function prototypes -----------------------------------------------*/
void jump_to_app();
uint32_t LedPWM(uint32_t pwm_period, uint32_t pwm_sweep_steps, uint32_t count);

extern int pios_com_softusart_id;

#define BLUE LED1
#define RED	LED4
int main() {
	/* NOTE: Do NOT modify the following start-up sequence */
	/* Any new initialization functions should be added in OpenPilotInit() */

	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();
	PIOS_IAP_Init();
	PIOS_Board_Init();

	PIOS_DELAY_Init();
	
	PIOS_LED_On(0);
	PIOS_LED_Off(1);
	
	PIOS_DELAY_WaitmS(150);

	bool timeout = false;
	uint32_t period1, period2, sweep_steps1, sweep_steps2;
	uint32_t stopwatch = 0;
	uint32_t prev_ticks = PIOS_DELAY_GetuS();

	while (!timeout) {
		uint8_t c;
		if(PIOS_COM_ReceiveBuffer(pios_com_softusart_id, &c, 1, 0) == 1) {
			PIOS_LED_Toggle(1);
			if (c == 'a')
				timeout = true;
			c++;
			PIOS_COM_SendBuffer(pios_com_softusart_id, &c, 1);
		}

		/* Update the stopwatch */
		uint32_t elapsed_ticks = PIOS_DELAY_GetuSSince(prev_ticks);
		prev_ticks += elapsed_ticks;
		stopwatch += elapsed_ticks;

		switch (DeviceState) {
		case Last_operation_Success:
		case uploadingStarting:
		case DFUidle:
			period1 = 5000;
			sweep_steps1 = 100;
			PIOS_LED_Off(1);
			period2 = 0;
			break;
		case uploading:
			period1 = 5000;
			sweep_steps1 = 100;
			period2 = 2500;
			sweep_steps2 = 50;
			break;
		case downloading:
			period1 = 2500;
			sweep_steps1 = 50;
			PIOS_LED_Off(1);
			period2 = 0;
			break;
		case BLidle:
			period1 = 0;
			PIOS_LED_On(1);
			period2 = 0;
			break;
		default://error
			period1 = 5000;
			sweep_steps1 = 100;
			period2 = 5000;
			sweep_steps2 = 100;
		}

		if (period1 != 0) {
			if (LedPWM(period1, sweep_steps1, stopwatch))
				PIOS_LED_On(1);
			else
				PIOS_LED_Off(1);
		} else
			PIOS_LED_On(1);

		if (period2 != 0) {
			if (LedPWM(period2, sweep_steps2, stopwatch))
				PIOS_LED_On(1);
			else
				PIOS_LED_Off(1);
		} else
			PIOS_LED_Off(1);

		if (stopwatch > 50 * 1000 * 1000)
			stopwatch = 0;
		if ((stopwatch > 6 * 1000 * 1000) && (DeviceState == BLidle))
			timeout = true;
	}
	
	jump_to_app();
	
	// No valid app found
	while(1) {
		PIOS_LED_Toggle(1);
		PIOS_DELAY_WaitmS(250);
		PIOS_LED_Toggle(0);
		PIOS_DELAY_WaitmS(250);
		PIOS_LED_Toggle(0);
		PIOS_LED_Toggle(1);
		PIOS_DELAY_WaitmS(250);
	}
}

#pragma GCC optimize ("O0")
void jump_to_app() {
	const struct pios_board_info * bdinfo = &pios_board_info_blob;
	if (((*(__IO uint32_t*) bdinfo->fw_base) & 0x2FFE0000) == 0x20000000) { /* Jump to user application */
		FLASH_Lock();
		RCC_APB2PeriphResetCmd(0xffffffff, ENABLE);
		RCC_APB1PeriphResetCmd(0xffffffff, ENABLE);
		RCC_APB2PeriphResetCmd(0xffffffff, DISABLE);
		RCC_APB1PeriphResetCmd(0xffffffff, DISABLE);
		_SetCNTR(0); // clear interrupt mask
		_SetISTR(0); // clear all requests
		JumpAddress = *(__IO uint32_t*) (bdinfo->fw_base + 4);
		Jump_To_Application = (pFunction) JumpAddress;
		/* Initialize user application's Stack Pointer */
		__set_MSP(*(__IO uint32_t*) bdinfo->fw_base);
		Jump_To_Application();
	} else {
		DeviceState = failed_jump;
		return;
	}
}


uint32_t LedPWM(uint32_t pwm_period, uint32_t pwm_sweep_steps, uint32_t count) {
	uint32_t curr_step = (count / pwm_period) % pwm_sweep_steps; /* 0 - pwm_sweep_steps */
	uint32_t pwm_duty = pwm_period * curr_step / pwm_sweep_steps; /* fraction of pwm_period */

	uint32_t curr_sweep = (count / (pwm_period * pwm_sweep_steps)); /* ticks once per full sweep */
	if (curr_sweep & 1) {
		pwm_duty = pwm_period - pwm_duty; /* reverse direction in odd sweeps */
	}
	return ((count % pwm_period) > pwm_duty) ? 1 : 0;
}

