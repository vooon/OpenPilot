/**
  ******************************************************************************
  * @addtogroup PIOS PIOS Core hardware abstraction layer
  * @{
  * @addtogroup PIOS_HCSR04 HCSR04 Functions
  * @brief Hardware functions to deal with the altitude pressure sensor
  * @{
  *
  * @file       pios_hcsr04.c
  * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
  * @brief      HCSR04 sonar Sensor Routines
  * @see        The GNU Public License (GPL) Version 3
  *
  ******************************************************************************/
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

#if defined(PIOS_INCLUDE_HCSR04)

static const TIM_TimeBaseInitTypeDef tim_8_time_base = {
	.TIM_Prescaler = (PIOS_PERIPHERAL_APB2_CLOCK / 500000) - 1,
	.TIM_ClockDivision = TIM_CKD_DIV1,
	.TIM_CounterMode = TIM_CounterMode_Up,
	.TIM_Period = 0xFFFF,
	.TIM_RepetitionCounter = 0x0000,
};

static const struct pios_tim_clock_cfg tim_8_cfg = {
	.timer = TIM8,
	.time_base_init = &tim_8_time_base,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = TIM8_CC_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

static const struct pios_tim_channel pios_tim_sonar_channel =
		{
			.timer = TIM8,
			.timer_chan = TIM_Channel_3,
			.pin = {
				.gpio = GPIOC,
				.init = {
					.GPIO_Pin = GPIO_Pin_8,
					.GPIO_Speed = GPIO_Speed_2MHz,
					.GPIO_Mode  = GPIO_Mode_AF,
					.GPIO_OType = GPIO_OType_PP,
					.GPIO_PuPd  = GPIO_PuPd_DOWN
				},
				.pin_source = GPIO_PinSource8,
			},
			.remap = GPIO_AF_TIM8,
		};

static void PIOS_sonar_tim_overflow_cb (uint32_t id, uint32_t context, uint8_t channel, uint16_t count);
static void PIOS_sonar_tim_edge_cb (uint32_t id, uint32_t context, uint8_t channel, uint16_t count);
const static struct pios_tim_callbacks sonar_tim_callbacks = {
	.overflow = PIOS_sonar_tim_overflow_cb,
	.edge     = PIOS_sonar_tim_edge_cb,
};

/* Local Variables */
static uint32_t sonar_tim_id;
static uint8_t CaptureState;
static uint16_t RiseValue;
static uint16_t FallValue;
static uint32_t CaptureValue;
static uint32_t CapCounter;

static TIM_ICInitTypeDef TIM_ICInitStructure;

/*
 * IC   = PC8 - TIM8 CH3
 * TRIG = PD10
 *
 */
#define PIOS_HCSR04_TRIG_GPIO_PORT                  GPIOD
#define PIOS_HCSR04_TRIG_PIN                        GPIO_Pin_10

/**
* Initialise the HC-SR04 sensor
*/
void PIOS_HCSR04_Init(void)
{
	/* Init triggerpin */

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Pin = PIOS_HCSR04_TRIG_PIN;
	GPIO_Init(PIOS_HCSR04_TRIG_GPIO_PORT, &GPIO_InitStructure);

	GPIO_ResetBits(PIOS_HCSR04_TRIG_GPIO_PORT,PIOS_HCSR04_TRIG_PIN);
	/* Flush counter variables */
	CaptureState = 0;
	RiseValue = 0;
	FallValue = 0;
	CaptureValue = 0;

	/* Setup RCC */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);

	// using PIOS for timer handling
	PIOS_TIM_InitClock(&tim_8_cfg);
	PIOS_TIM_InitChannels(&sonar_tim_id, &pios_tim_sonar_channel, 1, &sonar_tim_callbacks, 0);

	/* Configure timer for input capture */
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_3;
	TIM_ICInit(TIM8, &TIM_ICInitStructure);

	/* Enable the Capture Compare Interrupt Request */
	//TIM_ITConfig(PIOS_PWM_CH8_TIM_PORT, PIOS_PWM_CH8_CCR, ENABLE);
	TIM_ITConfig(TIM8, TIM_IT_CC3, DISABLE);

	/* Enable timers */
	TIM_Cmd(TIM8, ENABLE);

	/* Setup local variable which stays in this scope */
	/* Doing this here and using a local variable saves doing it in the ISR */
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;
}

int32_t PIOS_HCSR04_Get(void)
{
	return CaptureValue;
}

int32_t PIOS_HCSR04_Completed(void)
{
	return CapCounter;
}
/**
* Trigger sonar sensor
*/
void PIOS_HCSR04_Trigger(void)
{
	CapCounter=0;
	GPIO_SetBits(PIOS_HCSR04_TRIG_GPIO_PORT,PIOS_HCSR04_TRIG_PIN);
	PIOS_DELAY_WaituS(15);
	GPIO_ResetBits(PIOS_HCSR04_TRIG_GPIO_PORT,PIOS_HCSR04_TRIG_PIN);
	TIM_ITConfig(TIM8, TIM_IT_CC3, ENABLE);
}


/**
* Handle TIM8 global interrupt request
*/
void TIM8_Handler(uint16_t count,uint8_t overflow)
{
	CapCounter=1;
	/* Zero value always will be changed but this prevents compiler warning */
	int32_t i = 0;

	/* Do this as it's more efficient */
	//if (TIM_GetITStatus(TIM8, TIM_IT_CC3) == SET) {
	if(overflow!=1)
	{
		i = 7;
		if (CaptureState == 0) {
			RiseValue = count;
		} else {
			FallValue = count;
		}
	}

	/* Simple rise or fall state machine */
	if (CaptureState == 0)
	{
		/* Switch states */
		CaptureState = 1;

		/* Switch polarity of input capture */
		TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
		TIM_ICInitStructure.TIM_Channel = TIM_Channel_3;
		TIM_ICInit(TIM8, &TIM_ICInitStructure);

	} else {
		/* Capture computation */
		if (FallValue > RiseValue) {
			CaptureValue = (FallValue - RiseValue);
		} else {
			CaptureValue = ((0xFFFF - RiseValue) + FallValue);
		}

		/* Switch states */
		CaptureState = 0;

		/* Increase supervisor counter */
		CapCounter++;
		TIM_ITConfig(TIM8, TIM_IT_CC3, DISABLE);

		/* Switch polarity Altitudeof input capture */
		TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
		TIM_ICInitStructure.TIM_Channel = TIM_Channel_3;
		TIM_ICInit(TIM8, &TIM_ICInitStructure);
	}
}

static void PIOS_sonar_tim_overflow_cb (uint32_t tim_id, uint32_t context, uint8_t channel, uint16_t count)
{
	TIM8_Handler(count,1);
}
static void PIOS_sonar_tim_edge_cb (uint32_t tim_id, uint32_t context, uint8_t chan_idx, uint16_t count)
{
	TIM8_Handler(count,0);
}

#endif
