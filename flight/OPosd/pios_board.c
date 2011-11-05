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
#include <openpilot.h>
#include <uavobjectsinit.h>
#include <hwsettings.h>
#include <manualcontrolsettings.h>
#include <gcsreceiver.h>

#define PIOS_COM_TELEM_RF_RX_BUF_LEN 192
#define PIOS_COM_TELEM_RF_TX_BUF_LEN 192


#if defined(PIOS_INCLUDE_RTC)
/*
 * Realtime Clock (RTC)
 */
#include <pios_rtc_priv.h>

void PIOS_RTC_IRQ_Handler (void);
void RTC_IRQHandler() __attribute__ ((alias ("PIOS_RTC_IRQ_Handler")));
static const struct pios_rtc_cfg pios_rtc_main_cfg = {
	.clksrc = RCC_RTCCLKSource_HSE_Div128,
	.prescaler = 100,
	.irq = {
		.init = {
			.NVIC_IRQChannel                   = RTC_IRQn,
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

// ***********************************************************************************
// SPI

#if defined(PIOS_INCLUDE_SPI)

#include <pios_spi_priv.h>

/* OP Interface
 *
 * NOTE: Leave this declared as const data so that it ends up in the
 * .rodata section (ie. Flash) rather than in the .bss section (RAM).
 */
void PIOS_SPI_port_irq_handler(void);
void DMA1_Channel5_IRQHandler() __attribute__ ((alias ("PIOS_SPI_port_irq_handler")));
void DMA1_Channel4_IRQHandler() __attribute__ ((alias ("PIOS_SPI_port_irq_handler")));

static const struct pios_spi_cfg pios_spi_port_cfg =
{
		.regs = SPI2,
		.init = {
				.SPI_Mode = SPI_Mode_Master,
		//		.SPI_Mode = SPI_Mode_Slave,

		//		.SPI_Direction = SPI_Direction_2Lines_FullDuplex,
		//		.SPI_Direction = SPI_Direction_2Lines_RxOnly,
		//		.SPI_Direction = SPI_Direction_1Line_Rx,
				.SPI_Direction = SPI_Direction_1Line_Tx,

		//		.SPI_DataSize = SPI_DataSize_16b,
				.SPI_DataSize = SPI_DataSize_8b,

				.SPI_NSS = SPI_NSS_Soft,
		//		.SPI_NSS = SPI_NSS_Hard,

				.SPI_FirstBit = SPI_FirstBit_MSB,
		//		.SPI_FirstBit = SPI_FirstBit_LSB,

				.SPI_CRCPolynomial = 0,

				.SPI_CPOL = SPI_CPOL_Low,
		//		.SPI_CPOL = SPI_CPOL_High,

				.SPI_CPHA = SPI_CPHA_1Edge,
		//		.SPI_CPHA = SPI_CPHA_2Edge,

		//		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,		// fastest SCLK
				.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4,
		//		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8,
		//		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16,
		//		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32,
		//		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64,
		//		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128,
		//		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256,		// slowest SCLK
			 },
		.use_crc = FALSE,
		.dma = {
			.ahb_clk = RCC_AHBPeriph_DMA1,

			.irq = {
				.flags =
				(DMA1_FLAG_TC4 | DMA1_FLAG_TE4 | DMA1_FLAG_HT4 |
				 DMA1_FLAG_GL4),
				.init = {
					 .NVIC_IRQChannel = DMA1_Channel4_IRQn,
					 .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
					 .NVIC_IRQChannelSubPriority = 0,
					 .NVIC_IRQChannelCmd = ENABLE,
					 },
				},

			.rx = {
			       .channel = DMA1_Channel4,
			       .init = {
					.DMA_PeripheralBaseAddr =
					(uint32_t) & (SPI2->DR),
					.DMA_DIR = DMA_DIR_PeripheralSRC,
					.DMA_PeripheralInc =
					DMA_PeripheralInc_Disable,
					.DMA_MemoryInc = DMA_MemoryInc_Enable,
					.DMA_PeripheralDataSize =
					DMA_PeripheralDataSize_Byte,
					.DMA_MemoryDataSize =
					DMA_MemoryDataSize_Byte,
					.DMA_Mode = DMA_Mode_Normal,
					.DMA_Priority = DMA_Priority_Medium,
					.DMA_M2M = DMA_M2M_Disable,
					},
			       },
			.tx = {
			       .channel = DMA1_Channel5,
			       .init = {
					.DMA_PeripheralBaseAddr =
					(uint32_t) & (SPI2->DR),
					.DMA_DIR = DMA_DIR_PeripheralDST,
					.DMA_PeripheralInc =
					DMA_PeripheralInc_Disable,
					.DMA_MemoryInc = DMA_MemoryInc_Disable,
					.DMA_PeripheralDataSize =
					DMA_PeripheralDataSize_Byte,
					.DMA_MemoryDataSize =
					DMA_MemoryDataSize_Byte,
					.DMA_Mode = DMA_Mode_Normal,
					.DMA_Priority = DMA_Priority_Medium,
					.DMA_M2M = DMA_M2M_Disable,
					},
			       },
			},
		.ssel = {
			 .gpio = GPIOB,
			 .init = {
				  .GPIO_Pin = GPIO_Pin_12,
				  .GPIO_Speed = GPIO_Speed_10MHz,
				  .GPIO_Mode = GPIO_Mode_Out_PP,
				  },
			 },
		.sclk = {
			 .gpio = GPIOB,
			 .init = {
				  .GPIO_Pin = GPIO_Pin_13,
				  .GPIO_Speed = GPIO_Speed_10MHz,
				  .GPIO_Mode = GPIO_Mode_AF_PP,
				  },
			 },
		.miso = {
			 .gpio = GPIOB,
			 .init = {
				  .GPIO_Pin = GPIO_Pin_14,
				  .GPIO_Speed = GPIO_Speed_10MHz,
				  .GPIO_Mode = GPIO_Mode_IN_FLOATING,
				  },
			 },
		.mosi = {
			 .gpio = GPIOB,
			 .init = {
				  .GPIO_Pin = GPIO_Pin_15,
				  .GPIO_Speed = GPIO_Speed_50MHz,
				  .GPIO_Mode = GPIO_Mode_AF_PP,
				  },
			 },
};

uint32_t pios_spi_port_id;
void PIOS_SPI_port_irq_handler(void)
{
	/* Call into the generic code to handle the IRQ for this specific device */
	PIOS_SPI_IRQ_Handler(pios_spi_port_id);
}

#endif /* PIOS_INCLUDE_SPI */

/*
 * ADC system
 */
#include "pios_adc_priv.h"
extern void PIOS_ADC_handler(void);
void DMA1_Channel1_IRQHandler() __attribute__ ((alias("PIOS_ADC_handler")));
// Remap the ADC DMA handler to this one
static const struct pios_adc_cfg pios_adc_cfg = {
	.dma = {
		.ahb_clk  = RCC_AHBPeriph_DMA1,
		.irq = {
			.flags   = (DMA1_FLAG_TC1 | DMA1_FLAG_TE1 | DMA1_FLAG_HT1 | DMA1_FLAG_GL1),
			.init    = {
				.NVIC_IRQChannel                   = DMA1_Channel1_IRQn,
				.NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGH,
				.NVIC_IRQChannelSubPriority        = 0,
				.NVIC_IRQChannelCmd                = ENABLE,
			},
		},
		.rx = {
			.channel = DMA1_Channel1,
			.init    = {
				.DMA_PeripheralBaseAddr = (uint32_t) & ADC1->DR,
				.DMA_DIR                = DMA_DIR_PeripheralSRC,
				.DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
				.DMA_MemoryInc          = DMA_MemoryInc_Enable,
				.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word,
				.DMA_MemoryDataSize     = DMA_MemoryDataSize_Word,
				.DMA_Mode               = DMA_Mode_Circular,
				.DMA_Priority           = DMA_Priority_High,
				.DMA_M2M                = DMA_M2M_Disable,
			},
		}
	}, 
	.half_flag = DMA1_IT_HT1,
	.full_flag = DMA1_IT_TC1,
};

struct pios_adc_dev pios_adc_devs[] = {
	{
		.cfg = &pios_adc_cfg,
		.callback_function = NULL,
	},
};

uint8_t pios_adc_num_devices = NELEMENTS(pios_adc_devs);

void PIOS_ADC_handler() {
	PIOS_ADC_DMA_Handler();
}


// ***********************************************************************************
// USART

#if defined(PIOS_INCLUDE_USART)

#include <pios_usart_priv.h>

/*
 * SERIAL USART
 */
static const struct pios_usart_cfg pios_usart_serial_cfg =
{
		.regs = USART3,
		.init = {
			 .USART_BaudRate = 57600,
			 .USART_WordLength = USART_WordLength_8b,
			 .USART_Parity = USART_Parity_No,
			 .USART_StopBits = USART_StopBits_1,
			 .USART_HardwareFlowControl =
			 USART_HardwareFlowControl_None,
			 .USART_Mode = USART_Mode_Rx | USART_Mode_Tx,
			 },
		.irq = {
			.init = {
				 .NVIC_IRQChannel = USART3_IRQn,
				 .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_LOW,
				 .NVIC_IRQChannelSubPriority = 0,
				 .NVIC_IRQChannelCmd = ENABLE,
				 },
			},
		.rx = {
		       .gpio = GPIOB,
		       .init = {
				.GPIO_Pin = GPIO_Pin_11,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode = GPIO_Mode_IPU,
				},
		       },
		.tx = {
		       .gpio = GPIOB,
		       .init = {
				.GPIO_Pin = GPIO_Pin_10,
				.GPIO_Speed = GPIO_Speed_2MHz,
				.GPIO_Mode = GPIO_Mode_AF_PP,
				},
		       },
};

#endif /* PIOS_INCLUDE_USART */

// ***********************************************************************************

#if defined(PIOS_INCLUDE_COM)

#include <pios_com_priv.h>

#endif /* PIOS_INCLUDE_COM */

// ***********************************************************************************

extern const struct pios_com_driver pios_usb_com_driver;

uint32_t pios_com_hkosd_id;
uint32_t pios_com_telem_usb_id;

/**
 * PIOS_Board_Init()
 * initializes all the core subsystems on this specific hardware
 * called from System/openpilot.c
 */

uint16_t frameBuffer[GRAPHICS_HEIGHT][GRAPHICS_WIDTH];
GPIO_InitTypeDef			GPIO_InitStructure;
TIM_TimeBaseInitTypeDef		TIM_TimeBaseStructure;
TIM_OCInitTypeDef			TIM_OCInitStructure;
NVIC_InitTypeDef			NVIC_InitStructure;
SPI_InitTypeDef				SPI_InitStructure;
DMA_InitTypeDef				DMA_InitStructure;
USART_InitTypeDef			USART_InitStructure;
EXTI_InitTypeDef			EXTI_InitStructure;




void SPI_Config(void)
{
	//Set up SPI port.  This acts as a pixel buffer.
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;

	SPI_Init(SPI2, &SPI_InitStructure);

	SPI_Cmd(SPI2, ENABLE);
}

void DMA_Config(void)
{
	//Set up the DMA to keep the SPI port fed from the framebuffer.
	DMA_DeInit(DMA1_Channel5);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(SPI2->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)frameBuffer[1];
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;

	DMA_InitStructure.DMA_BufferSize = BUFFER_LINE_LENGTH;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
}
static void Clock(uint32_t spektrum_id);


#define PIOS_COM_HKOSD_RX_BUF_LEN 22
#define PIOS_COM_HKOSD_TX_BUF_LEN 22

void PIOS_Board_Init(void) {
	// Bring up System using CMSIS functions, enables the LEDs.
	PIOS_SYS_Init();

	// turn all the leds on
	USB_LED_ON;

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
	}

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

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz,
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE);
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
