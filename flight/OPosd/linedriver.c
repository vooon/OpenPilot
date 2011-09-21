/*
 * linedriver.c
 *
 *  Created on: 15.9.2011
 *      Author: Samba
 */
#include <pios.h>
#include "linedriver.h"
#include "textdriver.h"
#include "graphicdriver.h"
#include "videoconf.h"

volatile uint8_t update = 0;
volatile uint8_t activeTextId = 0;
volatile uint16_t activeTextLine = 0;
volatile uint8_t lineType = LINE_TYPE_UNKNOWN;
volatile uint16_t line = 0;

void initLine() {
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Enable DRDY GPIO clock */
	RCC_APB2PeriphClockCmd(PIOS_VIDEO_SYNC_CLK | RCC_APB2Periph_AFIO, ENABLE);

	/* Configure EOC pin as input floating */
	GPIO_InitStructure.GPIO_Pin = PIOS_VIDEO_SYNC_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(PIOS_VIDEO_SYNC_GPIO_PORT, &GPIO_InitStructure);

	/* Configure the End Of Conversion (EOC) interrupt */
	GPIO_EXTILineConfig(PIOS_VIDEO_SYNC_PORT_SOURCE, PIOS_VIDEO_SYNC_PIN_SOURCE);
	EXTI_InitStructure.EXTI_Line = PIOS_VIDEO_SYNC_EXTI_LINE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable and set EOC EXTI Interrupt to the lowest priority */
	NVIC_InitStructure.NVIC_IRQChannel = PIOS_VIDEO_SYNC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PIOS_VIDEO_SYNC_PRIO;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);


	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,GPIO_Pin_4);

	activeTextLine = textLines[activeTextId];
}

void updateLine() {
	PIOS_DELAY_WaituS(5); // wait 5us to see if H or V sync

	if(((PIOS_VIDEO_SYNC_GPIO_PORT->IDR & PIOS_VIDEO_SYNC_GPIO_PIN))) { // H sync
		if (line != 0) {
			switch(lineType) {
				case LINE_TYPE_TEXT:
#ifdef TEXT_ENABLED
					drawTextLine(activeTextId);
#endif //TEXTENABLED
					break;
				case LINE_TYPE_GRAPHICS:
#ifdef GRAPICSENABLED
					drawGrapicsLine();
#endif //GRAPICSENABLED
					break;
			}
		}

		// We save some time in beginning of line by pre-calculating next type.
		lineType = LINE_TYPE_UNKNOWN; // Default case
		line++;
		if (line == LAST_LINE) {
			update = 1;
			return;
		}

		if (line >= activeTextLine && line < (activeTextLine + TEXT_CHAR_HEIGHT * TEXT_SIZE_MULT)) {
	    lineType = LINE_TYPE_TEXT;
		}
		else if (line == (activeTextLine + TEXT_CHAR_HEIGHT * TEXT_SIZE_MULT)) {
		  update = 2;
			activeTextId = (activeTextId+1) % TEXT_LINES;
			activeTextLine = textLines[activeTextId];
			return;
		}
		#ifdef GRAPICSENABLED
		else if (line >= GRAPHICS_LINE && line < (GRAPHICS_LINE + GRAPHICS_HEIGHT)) {
			lineType = LINE_TYPE_GRAPHICS;
		}
    #endif //GRAPICSENABLED
	}
	else { // V sync
		if(line > 200) {
			line = 0;
		}
	}
}

void PIOS_Video_IRQHandler(void)
{
	updateLine();
}
