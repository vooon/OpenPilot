/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_PPM PPM Input Functions
 * @brief Code to measure PPM input and seperate into channels
 * @{
 *
 * @file       pios_ppm.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      PPM Input functions (STM32 dependent)
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
#include "pios_ppm_priv.h"

#if defined(PIOS_INCLUDE_PPM)

/* Provide a RCVR driver */
static int32_t PIOS_PPM_Get(uint32_t chan_id);

const struct pios_rcvr_driver pios_ppm_rcvr_driver = {
	.read = PIOS_PPM_Get,
};

/* Local Variables */
static TIM_ICInitTypeDef TIM_ICInitStructure;
static uint32_t CaptureValue[PIOS_PPM_NUM_INPUTS];

// *************************************************************

#define PPM_IN_MIN_SYNC_PULSE_US         7000                       // microseconds .. Pip's 6-chan TX goes down to 8.8ms
#define PPM_IN_MIN_CHANNEL_PULSE_US      750                        // microseconds
#define PPM_IN_MAX_CHANNEL_PULSE_US      2400                       // microseconds

// *************************************************************


volatile bool ppm_initialising = true;

volatile uint32_t ppm_In_PrevFrames = 0;
volatile uint32_t ppm_In_LastValidFrameTimer = 0;
volatile uint32_t ppm_In_Frames = 0;
volatile uint32_t ppm_In_ErrorFrames = 0;
volatile uint8_t ppm_In_NoisyChannelCounter = 0;
volatile int8_t ppm_In_ChannelsDetected = 0;
volatile int8_t ppm_In_ChannelPulseIndex = -1;
volatile int32_t ppm_In_PreviousValue = -1;
volatile uint32_t ppm_In_PulseWidth = 0;
volatile uint32_t ppm_In_ChannelPulseWidthNew[PIOS_PPM_NUM_INPUTS];
//volatile uint32_t ppm_In_ChannelPulseWidth[PIOS_PPM_NUM_INPUTS];


// *************************************************************



void PIOS_PPM_Init(void)
{
	/* Flush counter variables */

	ppm_initialising = true;

	ppm_In_PrevFrames = 0;
	ppm_In_NoisyChannelCounter = 0;
	ppm_In_LastValidFrameTimer = 0;
	ppm_In_Frames = 0;
	ppm_In_ErrorFrames = 0;
	ppm_In_ChannelsDetected = 0;
	ppm_In_ChannelPulseIndex = -1;
	ppm_In_PreviousValue = -1;
	ppm_In_PulseWidth = 0;

	for (int i = 0; i < PIOS_PPM_NUM_INPUTS; i++)
	{
		ppm_In_ChannelPulseWidthNew[i] = 0;
		CaptureValue[i] = 0;
	}

	NVIC_InitTypeDef NVIC_InitStructure = pios_ppm_cfg.irq.init;

	/* Enable appropriate clock to timer module */
	switch((int32_t) pios_ppm_cfg.timer) {
		case (int32_t)TIM1:
			NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
			break;
		case (int32_t)TIM2:
			NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
			break;
		case (int32_t)TIM3:
			NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
			break;
		case (int32_t)TIM4:
			NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
			break;
#ifdef STM32F10X_HD

		case (int32_t)TIM5:
			NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
			break;
		case (int32_t)TIM6:
			NVIC_InitStructure.NVIC_IRQChannel = TIM6_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
			break;
		case (int32_t)TIM7:
			NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
			RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);
			break;
		case (int32_t)TIM8:
			NVIC_InitStructure.NVIC_IRQChannel = TIM8_CC_IRQn;
			RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
			break;
#endif
	}
	/* Enable timer interrupts */
	NVIC_Init(&NVIC_InitStructure);

	/* Configure input pins */
	GPIO_InitTypeDef GPIO_InitStructure = pios_ppm_cfg.gpio_init;
	GPIO_Init(pios_ppm_cfg.port, &GPIO_InitStructure);

	/* Configure timer for input capture */
	TIM_ICInitStructure = pios_ppm_cfg.tim_ic_init;
	TIM_ICInit(pios_ppm_cfg.timer, &TIM_ICInitStructure);

	/* Configure timer clocks */
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = pios_ppm_cfg.tim_base_init;
	TIM_InternalClockConfig(pios_ppm_cfg.timer);
	TIM_TimeBaseInit(pios_ppm_cfg.timer, &TIM_TimeBaseStructure);

	/* Enable the Capture Compare Interrupt Request */
	TIM_ITConfig(pios_ppm_cfg.timer, pios_ppm_cfg.ccr | TIM_IT_Update, ENABLE);

	/* Enable timers */
	TIM_Cmd(pios_ppm_cfg.timer, ENABLE);

	/* Setup local variable which stays in this scope */
	/* Doing this here and using a local variable saves doing it in the ISR */
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;

	ppm_initialising = false;
}

/**
* Get the value of an input channel
* \param[in] Channel Number of the channel desired
* \output -1 Channel not available
* \output >0 Channel value
*/
static int32_t PIOS_PPM_Get(uint32_t chan_id)
{
	/* Return error if channel not available */
	if (chan_id >= PIOS_PPM_NUM_INPUTS) {
		return -1;
	}
	return CaptureValue[chan_id];
}

/**
* Handle TIMx global interrupt request
* Some work and testing still needed, need to detect start of frame and decode pulses
*
*/
void PIOS_PPM_irq_handler(void)
{
	uint16_t new_value = 0;
	uint32_t period = (uint32_t)pios_ppm_cfg.timer->ARR + 1;

	if (ppm_initialising) {
		// clear the interrupts
		TIM_ClearITPendingBit(pios_ppm_cfg.timer, pios_ppm_cfg.ccr | TIM_IT_Update);
		return;
	}

	// determine the interrupt source(s)
	bool update_int = TIM_GetITStatus(pios_ppm_cfg.timer, TIM_IT_Update) == SET;          // timer/counter overflow occured
	bool capture_int = TIM_GetITStatus(pios_ppm_cfg.timer, pios_ppm_cfg.ccr) == SET;   // PPM input capture

	if (capture_int) {
		switch((int32_t) pios_ppm_cfg.ccr) {
			case (int32_t)TIM_IT_CC1:
				new_value = TIM_GetCapture1(pios_ppm_cfg.timer);
				break;
			case (int32_t)TIM_IT_CC2:
				new_value = TIM_GetCapture2(pios_ppm_cfg.timer);
				break;
			case (int32_t)TIM_IT_CC3:
				new_value = TIM_GetCapture3(pios_ppm_cfg.timer);
				break;
			case (int32_t)TIM_IT_CC4:
				new_value = TIM_GetCapture4(pios_ppm_cfg.timer);
				break;
		}
	}

	// clear the interrupts
	TIM_ClearITPendingBit(pios_ppm_cfg.timer, pios_ppm_cfg.ccr | TIM_IT_Update);

	// ********

	uint32_t ticks = 0;
	if (update_int) {
		// timer/counter overflowed
		if (ppm_In_PreviousValue >= 0) {
			ticks = (period - ppm_In_PreviousValue) + new_value;
		} else {
			ticks = period;
			if (capture_int) ticks += new_value;
		}
		ppm_In_PreviousValue = -1;
	} else if (capture_int) {
		if (ppm_In_PreviousValue >= 0) {
			ticks = new_value - ppm_In_PreviousValue;
		} else {
			ticks += new_value;
		}
	}

	ppm_In_PulseWidth += ticks;
	if (ppm_In_PulseWidth > 0x7fffffff) {
		ppm_In_PulseWidth = 0x7fffffff;                    // prevent overflows
	}

	ppm_In_LastValidFrameTimer += ticks;
	if (ppm_In_LastValidFrameTimer > 0x7fffffff) {
		ppm_In_LastValidFrameTimer = 0x7fffffff;           // prevent overflows
	}

	if (capture_int) {
		ppm_In_PreviousValue = new_value;
	}

	if (ppm_In_LastValidFrameTimer >= 200000 && ppm_In_Frames > 0) {
		// we haven't seen a valid PPM frame for at least 200ms
		for (int i = 0; i < PIOS_PPM_NUM_INPUTS; i++) {
			CaptureValue[i] = 0;
		}
		ppm_In_Frames = 0;
		ppm_In_ErrorFrames = 0;
	}

	if (ppm_In_ChannelPulseIndex < 0 || ppm_In_PulseWidth > PPM_IN_MAX_CHANNEL_PULSE_US) {
		// we are looking for a SYNC pulse, or we are receiving one
		if (ppm_In_ChannelPulseIndex >= 0) {
			// it's either the start of a sync pulse or a noisy channel .. assume it's the end of a PPM frame
			if (ppm_In_ChannelPulseIndex > 0) {
				if (ppm_In_Frames < 0xffffffff) {
					ppm_In_Frames++;                            // update frame counter
				}

				if (ppm_In_ChannelsDetected > 0 && ppm_In_ChannelsDetected == ppm_In_ChannelPulseIndex && ppm_In_NoisyChannelCounter <= 2) {
					// detected same number of channels as in previous PPM frame .. save the new channel PWM values
					for (int i = 0; i < PIOS_PPM_NUM_INPUTS; i++) {
						CaptureValue[i] = ppm_In_ChannelPulseWidthNew[i];
					}

					ppm_In_LastValidFrameTimer = 0;                 // reset timer
				} else {
					if ((ppm_In_ChannelsDetected > 0 && ppm_In_ChannelsDetected != ppm_In_ChannelPulseIndex) ||
							ppm_In_NoisyChannelCounter >= 2)
					{
						if (ppm_In_ErrorFrames < 0xffffffff)
							ppm_In_ErrorFrames++;
					}
				}
				ppm_In_ChannelsDetected = ppm_In_ChannelPulseIndex;     // the number of channels we found in this frame
			}

			ppm_In_ChannelPulseIndex = -1;                              // back to looking for a SYNC pulse
		}

		if (ppm_In_PulseWidth >= PPM_IN_MIN_SYNC_PULSE_US) {
			// SYNC pulse found
			ppm_In_NoisyChannelCounter = 0;                             // reset noisy channel detector
			ppm_In_ChannelPulseIndex = 0;                               // start of PPM frame
		}
	} else if (capture_int) {
		// CHANNEL pulse

		if (ppm_In_PulseWidth < PPM_IN_MIN_CHANNEL_PULSE_US) {
			// bad/noisy channel pulse .. reset state to wait for next SYNC pulse
			ppm_In_ChannelPulseIndex = -1;

			if (ppm_In_ErrorFrames < 0xffffffff)
				ppm_In_ErrorFrames++;
		} else {
			// pulse width is within the accepted tolerance range for a channel
			if (ppm_In_ChannelPulseIndex < PIOS_PPM_NUM_INPUTS) {
				if (ppm_In_ChannelPulseWidthNew[ppm_In_ChannelPulseIndex] > 0) {
					int32_t difference = (int32_t)ppm_In_PulseWidth - ppm_In_ChannelPulseWidthNew[ppm_In_ChannelPulseIndex];
					if (abs(difference) >= 600) {
						ppm_In_NoisyChannelCounter++;                       // possibly a noisy channel - or an RC switch was moved
					}
				}
				ppm_In_ChannelPulseWidthNew[ppm_In_ChannelPulseIndex] = ppm_In_PulseWidth;    // save it
			}

			if (ppm_In_ChannelPulseIndex < 127) {
				ppm_In_ChannelPulseIndex++;                             // next channel
			}
		}
	}

	if (capture_int) {
		ppm_In_PulseWidth = 0;
	}
}

#endif

/**
  * @}
  * @}
  */
