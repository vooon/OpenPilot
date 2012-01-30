/**
 ******************************************************************************
 *
 * @file       pios_board.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Defines board specific static initializers for hardware for the OPOSD board.
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
#include <fifo_buffer.h>
#include <openpilot.h>
#include <uavobjectsinit.h>
#include <hwsettings.h>
#include <manualcontrolsettings.h>
#include <gcsreceiver.h>

#define PIOS_COM_TELEM_RF_RX_BUF_LEN 192
#define PIOS_COM_TELEM_RF_TX_BUF_LEN 192

#define PIOS_COM_HKOSD_RX_BUF_LEN 22
#define PIOS_COM_HKOSD_TX_BUF_LEN 22

#define TxBufferSize3   33


/* Private macro -------------------------------------------------------------*/
#define countof(a)   (sizeof(a) / sizeof(*(a)))

/* Private variables ---------------------------------------------------------*/
//uint8_t TxBuffer2[TxBufferSize2];
uint8_t TxBuffer3[TxBufferSize3];
//uint8_t RxBuffer2[TxBufferSize2];
uint8_t RxBuffer3[TxBufferSize3];
//uint8_t UART1_REVDATA[380];



/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

extern uint16_t *disp_buffer_level;
extern uint16_t *disp_buffer_mask;

DMA_InitTypeDef				DMA_InitStructure;
DMA_InitTypeDef				DMA_InitStructure2;
/*
USART_InitTypeDef USART_InitStructure;
USART_InitTypeDef			USART_InitStructure;
EXTI_InitTypeDef			EXTI_InitStructure;
GPIO_InitTypeDef			GPIO_InitStructure;
TIM_TimeBaseInitTypeDef		TIM_TimeBaseStructure;
TIM_OCInitTypeDef			TIM_OCInitStructure;
NVIC_InitTypeDef			NVIC_InitStructure;
SPI_InitTypeDef				SPI_InitStructure;
*/


#if defined(PIOS_INCLUDE_RTC)
/*
 * Realtime Clock (RTC)
 */
#include <pios_rtc_priv.h>

void PIOS_RTC_IRQ_Handler (void);
void RTC_WKUP_IRQHandler() __attribute__ ((alias ("PIOS_RTC_IRQ_Handler")));
static const struct pios_rtc_cfg pios_rtc_main_cfg = {
	.clksrc = RCC_RTCCLKSource_HSE_Div8, // Divide 8 Mhz crystal down to 1
	// For some reason it's acting like crystal is 16 Mhz.  This clock is then divided
	// by another 16 to give a nominal 62.5 khz clock
	.prescaler = 100, // Every 100 cycles gives 625 Hz
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = RTC_WKUP_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority        = 0,
			.NVIC_IRQChannelCmd                = ENABLE,
		},
	},
};

void PIOS_RTC_IRQ_Handler (void)
{
	PIOS_RTC_irq_handler ();
}

#endif

#include <pios_usart_priv.h>

#if defined(PIOS_INCLUDE_GPS)
/*
 * GPS USART
 */
static const struct pios_usart_cfg pios_usart_gps_cfg = {
	.regs = USART1,
	.remap = GPIO_AF_USART1,
	.init = {
		.USART_BaudRate = 38400,
		.USART_WordLength = USART_WordLength_8b,
		.USART_Parity = USART_Parity_No,
		.USART_StopBits = USART_StopBits_1,
		.USART_HardwareFlowControl =
		USART_HardwareFlowControl_None,
		.USART_Mode = USART_Mode_Rx | USART_Mode_Tx,
	},
	.irq = {
		.init = {
			.NVIC_IRQChannel = USART1_IRQn,
			.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_MID,
			.NVIC_IRQChannelSubPriority = 0,
			.NVIC_IRQChannelCmd = ENABLE,
		},
	},
	.rx = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_10,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
	},
	.tx = {
		.gpio = GPIOA,
		.init = {
			.GPIO_Pin   = GPIO_Pin_9,
			.GPIO_Speed = GPIO_Speed_2MHz,
			.GPIO_Mode  = GPIO_Mode_AF,
			.GPIO_OType = GPIO_OType_PP,
			.GPIO_PuPd  = GPIO_PuPd_UP
		},
	},
};

#define PIOS_COM_GPS_RX_BUF_LEN 192

#endif /* PIOS_INCLUDE_GPS */

uint32_t pios_com_aux_id;
uint32_t pios_com_gps_id;
uint32_t pios_com_telem_usb_id;
uint32_t pios_com_telem_rf_id;


#if defined(PIOS_INCLUDE_VIDEO)

#include <pios_video_priv.h>

static const struct pios_video_cfg pios_osd_cfg = {
	.mask = {
		.regs = SPI3,
		.remap = GPIO_AF_SPI3,
		.init = {
			.SPI_Mode              = SPI_Mode_Master,
			.SPI_Direction         = SPI_Direction_1Line_Tx,
			.SPI_DataSize          = SPI_DataSize_16b,
			.SPI_NSS               = SPI_NSS_Soft,
			.SPI_FirstBit          = SPI_FirstBit_MSB,
			.SPI_CRCPolynomial     = 7,
			.SPI_CPOL              = SPI_CPOL_Low,
			.SPI_CPHA              = SPI_CPHA_2Edge,
			.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4,
		},
		.use_crc = false,
		.dma = {
			.irq = {
				// Note this is the stream ID that triggers interrupts (in this case RX)
				.flags = (DMA_IT_TCIF3 | DMA_IT_TEIF3 | DMA_IT_HTIF3),
				.init = {
					.NVIC_IRQChannel = DMA1_Stream0_IRQn,
					.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
					.NVIC_IRQChannelSubPriority = 0,
					.NVIC_IRQChannelCmd = ENABLE,
				},
			},

			.rx = {
				//not used
				.channel = DMA1_Stream4,
				.init = {
					.DMA_Channel            = DMA_Channel_0,
					.DMA_PeripheralBaseAddr = (uint32_t) & (SPI3->DR),
					.DMA_DIR                = DMA_DIR_PeripheralToMemory,
					.DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
					.DMA_MemoryInc          = DMA_MemoryInc_Enable,
					.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
					.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord,
					.DMA_Mode               = DMA_Mode_Normal,
					.DMA_Priority           = DMA_Priority_Medium,
					//TODO: Enable FIFO
					.DMA_FIFOMode           = DMA_FIFOMode_Disable,
					.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
					.DMA_MemoryBurst        = DMA_MemoryBurst_Single,
					.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
				},
			},
			.tx = {
				.channel = DMA1_Stream5,
				.init = {
					.DMA_Channel            = DMA_Channel_0,
					.DMA_PeripheralBaseAddr = (uint32_t) & (SPI3->DR),
					.DMA_DIR                = DMA_DIR_MemoryToPeripheral,
					.DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
					.DMA_MemoryInc          = DMA_MemoryInc_Enable,
					.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
					.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord,
					.DMA_Mode               = DMA_Mode_Normal,
					.DMA_Priority           = DMA_Priority_High,
					.DMA_FIFOMode           = DMA_FIFOMode_Disable,
					.DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
					.DMA_MemoryBurst        = DMA_MemoryBurst_Single,
					.DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
				},
			},
		},
		.sclk = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin = GPIO_Pin_10,
				.GPIO_Speed = GPIO_Speed_100MHz,
				.GPIO_Mode = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd = GPIO_PuPd_NOPULL
			},
		},
		.miso = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin = GPIO_Pin_11,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_Mode = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd = GPIO_PuPd_NOPULL
			},
		},
		.mosi = {
			.gpio = GPIOC,
			.init = {
				.GPIO_Pin = GPIO_Pin_12,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_Mode = GPIO_Mode_AF,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd = GPIO_PuPd_NOPULL
			},
		},
		.slave_count = 1,
		.ssel = { {
			.gpio = GPIOD,
			.init = {
				.GPIO_Pin = GPIO_Pin_2,
				.GPIO_Speed = GPIO_Speed_50MHz,
				.GPIO_Mode  = GPIO_Mode_OUT,
				.GPIO_OType = GPIO_OType_PP,
				.GPIO_PuPd = GPIO_PuPd_UP
			},
		}, },
	},
	.level = {
			.regs = SPI1,
			.remap = GPIO_AF_SPI1,
			.init   = {
				.SPI_Mode              = SPI_Mode_Slave,
				.SPI_Direction         = SPI_Direction_1Line_Tx,
				.SPI_DataSize          = SPI_DataSize_16b,
				.SPI_NSS               = SPI_NSS_Soft,
				.SPI_FirstBit          = SPI_FirstBit_MSB,
				.SPI_CRCPolynomial     = 7,
				.SPI_CPOL              = SPI_CPOL_Low,
				.SPI_CPHA              = SPI_CPHA_2Edge,
				.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,
			},
			.use_crc = false,
			.dma = {
				.irq = {
					.flags   = (DMA_IT_TCIF0 | DMA_IT_TEIF0 | DMA_IT_HTIF0),
					.init    = {
						.NVIC_IRQChannel                   = DMA2_Stream0_IRQn,
						.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
						.NVIC_IRQChannelSubPriority        = 0,
						.NVIC_IRQChannelCmd                = ENABLE,
					},
				},

				.rx = {
					//not used
					.channel = DMA2_Stream0,
					.init    = {
		                .DMA_Channel            = DMA_Channel_3,
						.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR),
						.DMA_DIR                = DMA_DIR_PeripheralToMemory,
						.DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
						.DMA_MemoryInc          = DMA_MemoryInc_Enable,
						.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
						.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte,
						.DMA_Mode               = DMA_Mode_Normal,
						.DMA_Priority           = DMA_Priority_Medium,
						.DMA_FIFOMode           = DMA_FIFOMode_Disable,
		                /* .DMA_FIFOThreshold */
		                .DMA_MemoryBurst        = DMA_MemoryBurst_Single,
		                .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
					},
				},
				.tx = {
					.channel = DMA2_Stream5,
					.init    = {
		                .DMA_Channel            = DMA_Channel_3,
						.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR),
						.DMA_DIR                = DMA_DIR_MemoryToPeripheral,
						.DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
						.DMA_MemoryInc          = DMA_MemoryInc_Enable,
						.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
						.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord,
						.DMA_Mode               = DMA_Mode_Normal,
						.DMA_Priority           = DMA_Priority_High,
						.DMA_FIFOMode           = DMA_FIFOMode_Disable,
		                /* .DMA_FIFOThreshold */
		                .DMA_MemoryBurst        = DMA_MemoryBurst_Single,
		                .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single,
					},
				},
			},
			.sclk = {
				.gpio = GPIOB,
				.init = {
					.GPIO_Pin   = GPIO_Pin_3,
					.GPIO_Speed = GPIO_Speed_100MHz,
					.GPIO_Mode  = GPIO_Mode_AF,
					.GPIO_OType = GPIO_OType_PP,
					.GPIO_PuPd = GPIO_PuPd_UP
				},
			},
			.miso = {
				.gpio = GPIOB,
				.init = {
					.GPIO_Pin   = GPIO_Pin_4,
					.GPIO_Speed = GPIO_Speed_50MHz,
					.GPIO_Mode  = GPIO_Mode_AF,
					.GPIO_OType = GPIO_OType_PP,
					.GPIO_PuPd = GPIO_PuPd_UP
				},
			},
			.mosi = {
				.gpio = GPIOB,
				.init = {
					.GPIO_Pin   = GPIO_Pin_5,
					.GPIO_Speed = GPIO_Speed_50MHz,
					.GPIO_Mode  = GPIO_Mode_AF,
					.GPIO_OType = GPIO_OType_PP,
					.GPIO_PuPd = GPIO_PuPd_UP
				},
			},
			.slave_count = 1,
			.ssel = { {
				.gpio = GPIOB,
				.init = {
					.GPIO_Pin   = GPIO_Pin_6,
					.GPIO_Speed = GPIO_Speed_50MHz,
					.GPIO_Mode  = GPIO_Mode_OUT,
					.GPIO_OType = GPIO_OType_PP,
					.GPIO_PuPd = GPIO_PuPd_UP
				},
			}, },

	},
	.hsync = {
		.pin_source = PIOS_VIDEO_HSYNC_EXTI_PIN_SOURCE,
		.port_source = PIOS_VIDEO_HSYNC_EXTI_PORT_SOURCE,
		.init = {
			.EXTI_Line = PIOS_VIDEO_HSYNC_EXTI_LINE,
			.EXTI_Mode = EXTI_Mode_Interrupt,
			.EXTI_Trigger = EXTI_Trigger_Rising_Falling,
			.EXTI_LineCmd = ENABLE,
		},
	},
	.vsync = {
		.pin_source = PIOS_VIDEO_VSYNC_EXTI_PIN_SOURCE,
		.port_source = PIOS_VIDEO_VSYNC_EXTI_PORT_SOURCE,
		.init = {
			.EXTI_Line = PIOS_VIDEO_VSYNC_EXTI_LINE,
			.EXTI_Mode = EXTI_Mode_Interrupt,
			.EXTI_Trigger = EXTI_Trigger_Falling,
			.EXTI_LineCmd = ENABLE,
		},
	},
};



#endif

void SPI_Config(void)
{
	SPI_InitTypeDef				SPI_InitStructure;

	GPIO_InitTypeDef GPIO_InitStructure;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

	GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource12,GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource3,GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource4,GPIO_AF_SPI1);

	//SPI1 SLAVE FRAMEBUFFER
	//SPI1 MISO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//SPI1 SCK
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//SPI3 MASTER MASKBUFFER
	//SPI3 SCK
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//SPI3 MOSI
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//Set up SPI port.  This acts as a pixel buffer.
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_InitStructure);

	//Set up SPI port.  This acts as a mask buffer.
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // 27 MHz/4
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI3, &SPI_InitStructure);

	SPI_Cmd(SPI1, ENABLE);
	SPI_Cmd(SPI3, ENABLE);
}

void PIOS_VIDEO_DMA_Handler(void);
void DMA1_Stream5_IRQHandler(void) __attribute__ ((alias("PIOS_VIDEO_DMA_Handler")));


void DMA_Config(void)
{
	NVIC_InitTypeDef			NVIC_InitStructure;
	//Set up the DMA to keep the SPI port fed from the framebuffer.

	DMA_DeInit(DMA2_Stream5);
	DMA_InitStructure.DMA_Channel = DMA_Channel_3;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)disp_buffer_level[0];
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_BufferSize = BUFFER_LINE_LENGTH;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream5, &DMA_InitStructure);

	//Set up the DMA to keep the SPI port fed from the framebuffer.
	DMA_DeInit(DMA1_Stream5);
	DMA_InitStructure2.DMA_Channel = DMA_Channel_0;
	DMA_InitStructure2.DMA_PeripheralBaseAddr = (uint32_t)&(SPI3->DR);
	DMA_InitStructure2.DMA_Memory0BaseAddr = (uint32_t)disp_buffer_mask[0];
	DMA_InitStructure2.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure2.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure2.DMA_BufferSize = BUFFER_LINE_LENGTH;
	DMA_InitStructure2.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure2.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure2.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure2.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure2.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure2.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure2.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure2.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure2.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA1_Stream5, &DMA_InitStructure2);
	//

	/* Trigger interrupt when for half conversions too to indicate double buffer */
	//DMA_ITConfig(DMA1_Stream5, DMA_IT_TC, ENABLE);
	DMA_ClearFlag(DMA1_Stream5,DMA_FLAG_TCIF5);
	DMA_ClearITPendingBit(DMA1_Stream5, DMA_IT_TCIF5);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
	SPI_I2S_DMACmd(SPI3, SPI_I2S_DMAReq_Tx, ENABLE);
	/* Configure DMA interrupt */
	/*NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);*/
}


/**
 * @brief Interrupt for half and full buffer transfer
 *
 * This interrupt handler swaps between the two halfs of the double buffer to make
 * sure the ahrs uses the most recent data.  Only swaps data when AHRS is idle, but
 * really this is a pretense of a sanity check since the DMA engine is consantly
 * running in the background.  Keep an eye on the ekf_too_slow variable to make sure
 * it's keeping up.
 */
void PIOS_VIDEO_DMA_Handler(void)
{
	if (DMA_GetFlagStatus(DMA1_Stream5,DMA_FLAG_TCIF5)) {	// whole double buffer filled
		//DMA_ClearFlag(DMA1_FLAG_TC3);
	}
	else if (DMA_GetFlagStatus(DMA1_Stream5,DMA_FLAG_HTIF5)) {
		DMA_ClearFlag(DMA1_Stream5,DMA_FLAG_HTIF5);
	}
	else {

	}
}

static void Clock(uint32_t spektrum_id);

void initUSARTs(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	/* Configure USART Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource0,GPIO_AF_UART4);
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	/* Configure USART Rx as input floating */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource1,GPIO_AF_UART4);
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	USART_InitStructure.USART_BaudRate = 57600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(UART4, &USART_InitStructure);
	USART_DMACmd(UART4, USART_DMAReq_Rx, ENABLE);
	DMA_Cmd(DMA1_Stream2, ENABLE);
	USART_Cmd(UART4, ENABLE);
}

#define DMA_Channel_USART4_RX    DMA1_Stream2
#define DMA_Channel_USART4_TX    DMA1_Stream4
#define DMA_FLAG_USART3_TC_RX    DMA1_FLAG_TC3
#define DMA_FLAG_USART3_TC_TX    DMA1_FLAG_TC2
#define USART3_DR_Base  0x40004804

void init_USART_dma()
{
	DMA_InitTypeDef				DMA_InitStructure;

	/*RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

   DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART3->DR);
   DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
   DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
   DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
   DMA_InitStructure.DMA_BufferSize = size;
   DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
   DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
   DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
   DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)buff[0];
   DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
   DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;

   DMA_DeInit(DMA_Channel_USART3_RX);
   DMA_Init(DMA_Channel_USART3_RX, &DMA_InitStructure);
   //DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);

   DMA_Cmd(DMA_Channel_USART3_RX, ENABLE);
   USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);*/

	/*DMA Channel2 USART3 TX*/
   DMA_DeInit(DMA1_Stream4);
   DMA_InitStructure.DMA_Channel = DMA_Channel_4;
   DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART4->DR;
   DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)TxBuffer3;
   DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;  /*read from  ram*/
   DMA_InitStructure.DMA_BufferSize = TxBufferSize3;	 /*if content is 0,stop TX*/
   DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
   DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
   DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
   DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
   DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
   //DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
   DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
   //DMA_Init(DMA1_Channel2, &DMA_InitStructure);
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

   /*DMA Channel3 USART3 RX*/
   DMA_DeInit(DMA1_Stream2);
   DMA_InitStructure.DMA_Channel = DMA_Channel_4;
   DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART4->DR;
   DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)RxBuffer3;
   DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
   DMA_InitStructure.DMA_BufferSize = sizeof(RxBuffer3);
   DMA_Init(DMA1_Stream2, &DMA_InitStructure);

}

void PIOS_Board_Init(void) {

	// Delay system
	PIOS_DELAY_Init();

	/* Initialize UAVObject libraries */
	EventDispatcherInitialize();
	UAVObjInitialize();


	/* Initialize the alarms library */
	AlarmsInitialize();

	/* Initialize the task monitor library */
	TaskMonitorInitialize();



#if defined(PIOS_INCLUDE_RTC)
	/* Initialize the real-time clock and its associated tick */
	PIOS_RTC_Init(&pios_rtc_main_cfg);
	if (!PIOS_RTC_RegisterTickCallback(Clock, 0)) {
		PIOS_DEBUG_Assert(0);
	}
#endif

#if defined(PIOS_INCLUDE_COM)
#if defined(PIOS_INCLUDE_GPS)

	uint32_t pios_usart_gps_id;
	if (PIOS_USART_Init(&pios_usart_gps_id, &pios_usart_gps_cfg)) {
		PIOS_Assert(0);
	}

	uint8_t * rx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_GPS_RX_BUF_LEN);
	PIOS_Assert(rx_buffer);
	if (PIOS_COM_Init(&pios_com_gps_id, &pios_usart_com_driver, pios_usart_gps_id,
					  rx_buffer, PIOS_COM_GPS_RX_BUF_LEN,
					  NULL, 0)) {
		PIOS_Assert(0);
	}

#endif	/* PIOS_INCLUDE_GPS */

#if defined(PIOS_INCLUDE_COM_AUX)
	uint32_t pios_usart_aux_id;

    if (PIOS_USART_Init(&pios_usart_aux_id, &pios_usart_aux_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
	if (PIOS_COM_Init(&pios_com_aux_id, &pios_usart_com_driver, pios_usart_aux_id,
					  pios_com_aux_rx_buffer, sizeof(pios_com_aux_rx_buffer),
					  pios_com_aux_tx_buffer, sizeof(pios_com_aux_tx_buffer))) {
		PIOS_DEBUG_Assert(0);
	}
#endif

#if defined(PIOS_INCLUDE_COM_TELEM)
{ /* Eventually add switch for this port function */
	uint32_t pios_usart_telem_rf_id;
	if (PIOS_USART_Init(&pios_usart_telem_rf_id, &pios_usart_telem_main_cfg)) {
		PIOS_Assert(0);
	}

	uint8_t * rx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_TELEM_RF_RX_BUF_LEN);
	uint8_t * tx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_TELEM_RF_TX_BUF_LEN);
	PIOS_Assert(rx_buffer);
	PIOS_Assert(tx_buffer);
	if (PIOS_COM_Init(&pios_com_telem_rf_id, &pios_usart_com_driver, pios_usart_telem_rf_id,
					  rx_buffer, PIOS_COM_TELEM_RF_RX_BUF_LEN,
					  tx_buffer, PIOS_COM_TELEM_RF_TX_BUF_LEN)) {
		PIOS_Assert(0);
	}
}
#endif	/* PIOS_INCLUDE_COM_AUX */

#endif	/* PIOS_INCLUDE_COM */

/*
	uint32_t pios_usart_hkosd_id;
	if (PIOS_USART_Init(&pios_usart_hkosd_id, &pios_usart_serial_cfg)) {
		PIOS_Assert(0);
	}
	uint8_t * rx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_HKOSD_RX_BUF_LEN);
	PIOS_Assert(rx_buffer);

	uint8_t * tx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_HKOSD_TX_BUF_LEN);
	PIOS_Assert(tx_buffer);
	if (PIOS_COM_Init(&pios_com_hkosd_id, &pios_usart_com_driver, pios_usart_hkosd_id,
				rx_buffer, PIOS_COM_HKOSD_RX_BUF_LEN,
				tx_buffer, PIOS_COM_HKOSD_TX_BUF_LEN)) {
		PIOS_Assert(0);
	}*/

/*
	uint32_t pios_usart_serial_id;
	if (PIOS_USART_Init(&pios_usart_serial_id, &pios_usart_serial_cfg)) {
		PIOS_DEBUG_Assert(0);
	}
	uint8_t * rx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_TELEM_RF_RX_BUF_LEN);
	uint8_t * tx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_TELEM_RF_TX_BUF_LEN);
	PIOS_Assert(rx_buffer);
	PIOS_Assert(tx_buffer);

	if (PIOS_COM_Init(&pios_com_serial_id, &pios_usart_com_driver, pios_usart_serial_id,
			  rx_buffer, PIOS_COM_TELEM_RF_RX_BUF_LEN,
			  tx_buffer, PIOS_COM_TELEM_RF_TX_BUF_LEN)) {
		PIOS_Assert(0);
	}*/


	// ADC system
	// PIOS_ADC_Init();

	// SPI link to master
	/*if (PIOS_SPI_Init(&pios_spi_port_id, &pios_spi_port_cfg)) {
		PIOS_DEBUG_Assert(0);
	}*/

	//Setup the SPI port (video is output by its shift register).
	SPI_Config();

	//Setup the DMA to keep the spi port fed
	DMA_Config();

	//uint8_t * rx_buffer = (uint8_t *) pvPortMalloc(PIOS_COM_HKOSD_RX_BUF_LEN);

	//uint8_t test[16];
	init_USART_dma();
	initUSARTs();
	extern t_fifo_buffer rx;

	fifoBuf_init(&rx,RxBuffer3,sizeof(RxBuffer3));


    initLine();

	//fillFrameBuffer();
}

uint16_t supv_timer=0;

static void Clock(uint32_t spektrum_id) {
	/* 125hz */
	++supv_timer;
	if(supv_timer >= 625) {
		supv_timer = 0;
		time.sec++;
	}
	if (time.sec >= 60) {
		time.sec = 0;
		time.min++;
	}
	if (time.min >= 60) {
		time.min = 0;
		time.hour++;
	}
	if (time.hour >= 99) {
		time.hour = 0;
	}
}
