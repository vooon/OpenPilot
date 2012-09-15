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
#include "ssp.h"

/* Prototype of PIOS_Board_Init() function */
extern void PIOS_Board_Init(void);

#define SSP_TIMER	TIM1
uint32_t ssp_time = 0;
#define MAX_PACKET_DATA_LEN	255
#define MAX_PACKET_BUF_SIZE	(1+1+MAX_PACKET_DATA_LEN+2)
#define UART_BUFFER_SIZE 1024
uint8_t rx_buffer[UART_BUFFER_SIZE] __attribute__ ((aligned(4)));
// align to 32-bit to try and provide speed improvement;
// master buffers...
uint8_t SSP_TxBuf[MAX_PACKET_BUF_SIZE];
uint8_t SSP_RxBuf[MAX_PACKET_BUF_SIZE];
void SSP_CallBack(uint8_t *buf, uint16_t len);
int16_t SSP_SerialRead(void);
void SSP_SerialWrite( uint8_t);
uint32_t SSP_GetTime(void);

// Since we are using the MCU timer at 72e6 this timeout len is 0.5 sec
PortConfig_t SSP_PortConfig = { .rxBuf = SSP_RxBuf,
		.rxBufSize = MAX_PACKET_DATA_LEN, .txBuf = SSP_TxBuf,
		.txBufSize = MAX_PACKET_DATA_LEN, .max_retry = 10, .timeoutLen = 360000000,
		.pfCallBack = SSP_CallBack, .pfSerialRead = SSP_SerialRead,
		.pfSerialWrite = SSP_SerialWrite, .pfGetTime = SSP_GetTime, };
Port_t ssp_port;
t_fifo_buffer ssp_buffer;
int16_t status = 0;

int packets = 0;

/* Private typedef -----------------------------------------------------------*/
typedef void (*pFunction)(void);
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction Jump_To_Application;
uint32_t JumpAddress;
static uint8_t mReceive_Buffer[63];
extern Port_t ssp_port;

/* Extern variables ----------------------------------------------------------*/
DFUStates DeviceState;
/* Private function prototypes -----------------------------------------------*/
void jump_to_app();
uint32_t LedPWM(uint32_t pwm_period, uint32_t pwm_sweep_steps, uint32_t count);

extern int pios_com_softusart_id;
int  last_time2;

int main() {
	/* NOTE: Do NOT modify the following start-up sequence */
	/* Any new initialization functions should be added in OpenPilotInit() */

	/* Brings up System using CMSIS functions, enables the LEDs. */
	PIOS_SYS_Init();
	PIOS_Board_Init();

	// Bootup logic:
	// Check for DFU request.  If there is one stay here.
	// Check the firmware.  If there is a correct CRC _AND_ a quick boot flag go to code
	// If the firmware is not OK stay here
	// Otherwise wait 2 seconds for a serial command
	DeviceState = BLidle;
	if (PIOS_IAP_CheckRequest() == TRUE) {
		DeviceState = DFUidle;
		PIOS_IAP_ClearRequest();
	}

	PIOS_LED_On(0);
	PIOS_LED_Off(1);
	
	PIOS_DELAY_WaitmS(150);

	bool timeout = false;
	uint32_t period1, sweep_steps1;
	uint32_t stopwatch = 0;
	uint32_t prev_ticks = PIOS_DELAY_GetuS();

	// Sign of life
	PIOS_LED_Toggle(1);
	PIOS_DELAY_WaitmS(100);
	PIOS_LED_Toggle(1);
	PIOS_DELAY_WaitmS(100);
	PIOS_LED_Toggle(1);

	// Initialize the SSP layer between serial port and DFU
	fifoBuf_init(&ssp_buffer, rx_buffer, UART_BUFFER_SIZE);
	ssp_Init(&ssp_port, &SSP_PortConfig);

	while (!timeout) {
		last_time2 = PIOS_DELAY_GetRaw() / 72;
		ssp_ReceiveProcess(&ssp_port);
		status = ssp_SendProcess(&ssp_port);
		while ((status != SSP_TX_IDLE) && (status != SSP_TX_ACKED)) {
			ssp_ReceiveProcess(&ssp_port);
			status = ssp_SendProcess(&ssp_port);
		}

		if (fifoBuf_getUsed(&ssp_buffer) >= 63) {
			for (int32_t x = 0; x < 63; ++x) {
				mReceive_Buffer[x] = fifoBuf_getByte(&ssp_buffer);
			}
			packets ++;
			processComand(mReceive_Buffer);
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
			break;
		case uploading:
			period1 = 5000;
			sweep_steps1 = 100;
			break;
		case downloading:
			period1 = 2500;
			sweep_steps1 = 50;
			PIOS_LED_Off(1);
			break;
		case BLidle:
			period1 = 1000;
			sweep_steps1 = 100;
			PIOS_LED_On(1);
			break;
		default://error
			period1 = 5000;
			sweep_steps1 = 100;
			}

		if (period1 != 0) {
			if (LedPWM(period1, sweep_steps1, stopwatch))
				PIOS_LED_On(1);
			else
				PIOS_LED_Off(1);
		} else
			PIOS_LED_On(1);
		
		if (stopwatch > 50 * 1000 * 1000)
			stopwatch = 0;
		if ((stopwatch > 3 * 1000 * 1000) && (DeviceState == BLidle))
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

int last_time;
int sspTimeSource() {
	last_time = PIOS_DELAY_GetRaw();
	return last_time;
}
void SSP_CallBack(uint8_t *buf, uint16_t len) {
	fifoBuf_putData(&ssp_buffer, buf, len);
}

int query_count;
int byte_count;
uint8_t byte_record[8];
int brp = 0;
int16_t SSP_SerialRead(void) {
	query_count++;
	uint8_t byte;
	if (PIOS_COM_ReceiveBuffer(pios_com_softusart_id, &byte, 1, 0) == 1) {
		byte_record[(brp++) % 8] = byte; 
		PIOS_LED_Toggle(1);
		PIOS_DELAY_WaituS(100);
		byte_count++;
		return byte;
	} else {
		return -1;
	}
}

int written_bytes = 0;
uint8_t w_byte_record[8];
int w_brp = 0;
void SSP_SerialWrite(uint8_t value) {
	written_bytes++;
	w_byte_record[(w_brp++) % 8] = value; 
	PIOS_COM_SendChar(pios_com_softusart_id, value);
	PIOS_DELAY_WaituS(2500);
}
uint32_t SSP_GetTime(void) {
	return sspTimeSource();
}
