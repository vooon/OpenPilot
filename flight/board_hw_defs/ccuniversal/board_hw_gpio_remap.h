
/**
 ******************************************************************************
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotCore OpenPilot Core
 * @{
 *
 * @file       board_hw_gpio_remap.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      GPIO remap compatability between F1 and F3 for the OpenPilot board
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

#ifndef BOARD_HW_GPIO_REMAP_H_
#define BOARD_HW_GPIO_REMAP_H_

/** @brief  AF 0 selection
  *#define GPIO_AF_0    JTCK-SWCLK, JTDI, JTDO/TRACESW0, JTMS-SWDAT,
                                                MCO, NJTRST, TRACED, TRACECK */
/** @brief  AF 1 selection
  *#define GPIO_AF_1    OUT, TIM2, TIM15, TIM16, TIM17 */
/** @brief  AF 2 selection
  *#define GPIO_AF_2    COMP1_OUT, TIM1, TIM2, TIM3, TIM4, TIM8, TIM15 */
/** @brief  AF 3 selection
  *#define GPIO_AF_3    COMP7_OUT, TIM8, TIM15, Touch */
/** @brief  AF 4 selection
  *#define GPIO_AF_4    I2C1, I2C2, TIM1, TIM8, TIM16, TIM17 */
/** @brief  AF 5 selection
  *#define GPIO_AF_5    IR_OUT, I2S2, I2S3, SPI1, SPI2, TIM8, USART4, USART5 */
/** @brief  AF 6 selection
  *#define GPIO_AF_6    IR_OUT, I2S2, I2S3, SPI2, SPI3, TIM1, TIM8 */
/** @brief  AF 7 selection
  *#define GPIO_AF_7    AOP2_OUT, CAN, COMP3_OUT, COMP5_OUT, COMP6_OUT,
                                                USART1, USART2, USART3 */
/** @brief  AF 8 selection
  *#define GPIO_AF_8    COMP1_OUT, COMP2_OUT, COMP3_OUT, COMP4_OUT,
                                                COMP5_OUT, COMP6_OUT */
/** @brief  AF 9 selection
  *#define GPIO_AF_9    AOP4_OUT, CAN, TIM1, TIM8, TIM15 */
/** @brief  AF 10 selection
  *#define GPIO_AF_10   AOP1_OUT, AOP3_OUT, TIM2, TIM3, TIM4, TIM8, TIM17 */
/** @brief  AF 11 selection
  *#define GPIO_AF_11   TIM1, TIM8 */
/** @brief  AF 12 selection
   *#define GPIO_AF_12  TIM1 */
/** @brief  AF 14 selection
  *#define GPIO_AF_14   USBDM, USBDP */
/** @brief  AF 15 selection
  *#define GPIO_AF_15   OUT */


/** @defgroup GPIO_Remap_define
  * @{
  */

#define GPIO_Remap_SPI1             GPIO_AF_5  /*!< SPI1 Alternate Function mapping */
#define GPIO_Remap_I2C1             GPIO_AF_4  /*!< I2C1 Alternate Function mapping */
#define GPIO_Remap_USART1           GPIO_AF_7  /*!< USART1 Alternate Function mapping */
#define GPIO_Remap_USART2           GPIO_AF_7  /*!< USART2 Alternate Function mapping */
#define GPIO_PartialRemap_USART3    GPIO_AF_7  /*!< USART3 Partial Alternate Function mapping */
#define GPIO_FullRemap_USART3       GPIO_AF_7  /*!< USART3 Full Alternate Function mapping */
#define GPIO_PartialRemap_TIM1      GPIO_AF_9  /*!< TIM1 Partial Alternate Function mapping */
#define GPIO_FullRemap_TIM1         GPIO_AF_9  /*!< TIM1 Full Alternate Function mapping */
#define GPIO_PartialRemap1_TIM2     GPIO_AF_10  /*!< TIM2 Partial1 Alternate Function mapping */
#define GPIO_PartialRemap2_TIM2     GPIO_AF_10  /*!< TIM2 Partial2 Alternate Function mapping */
#define GPIO_FullRemap_TIM2         GPIO_AF_10  /*!< TIM2 Full Alternate Function mapping */
#define GPIO_PartialRemap_TIM3      GPIO_AF_10  /*!< TIM3 Partial Alternate Function mapping */
#define GPIO_FullRemap_TIM3         GPIO_AF_10  /*!< TIM3 Full Alternate Function mapping */
#define GPIO_Remap_TIM4             GPIO_AF_10  /*!< TIM4 Alternate Function mapping */
#define GPIO_Remap1_CAN1            GPIO_AF_7  /*!< CAN1 Alternate Function mapping */
#define GPIO_Remap2_CAN1            GPIO_AF_7  /*!< CAN1 Alternate Function mapping */
#define GPIO_Remap_PD01             ((uint32_t)0x00008000)  /*!< PD01 Alternate Function mapping */
#define GPIO_Remap_TIM5CH4_LSI      ((uint32_t)0x00200001)  /*!< LSI connected to TIM5 Channel4 input capture for calibration */
#define GPIO_Remap_ADC1_ETRGINJ     ((uint32_t)0x00200002)  /*!< ADC1 External Trigger Injected Conversion remapping */
#define GPIO_Remap_ADC1_ETRGREG     ((uint32_t)0x00200004)  /*!< ADC1 External Trigger Regular Conversion remapping */
#define GPIO_Remap_ADC2_ETRGINJ     ((uint32_t)0x00200008)  /*!< ADC2 External Trigger Injected Conversion remapping */
#define GPIO_Remap_ADC2_ETRGREG     ((uint32_t)0x00200010)  /*!< ADC2 External Trigger Regular Conversion remapping */
#define GPIO_Remap_ETH              ((uint32_t)0x00200020)  /*!< Ethernet remapping (only for Connectivity line devices) */
#define GPIO_Remap_CAN2             GPIO_AF_9  /*!< CAN2 remapping (only for Connectivity line devices) */
#define GPIO_Remap_SWJ_NoJTRST      GPIO_AF_0  /*!< Full SWJ Enabled (JTAG-DP + SW-DP) but without JTRST */
#define GPIO_Remap_SWJ_JTAGDisable  GPIO_AF_0  /*!< JTAG-DP Disabled and SW-DP Enabled */
#define GPIO_Remap_SWJ_Disable      GPIO_AF_0  /*!< Full SWJ Disabled (JTAG-DP + SW-DP) */
#define GPIO_Remap_SPI3             GPIO_AF_6  /*!< SPI3/I2S3 Alternate Function mapping (only for Connectivity line devices) */
#define GPIO_Remap_TIM2ITR1_PTP_SOF ((uint32_t)0x00202000)  /*!< Ethernet PTP output or USB OTG SOF (Start of Frame) connected
                                                                 to TIM2 Internal Trigger 1 for calibration
                                                                 (only for Connectivity line devices) */
#define GPIO_Remap_PTP_PPS          ((uint32_t)0x00204000)  /*!< Ethernet MAC PPS_PTS output on PB05 (only for Connectivity line devices) */

#define GPIO_Remap_TIM15            GPIO_AF_9  /*!< TIM15 Alternate Function mapping (only for Value line devices) */
#define GPIO_Remap_TIM16            GPIO_AF_4  /*!< TIM16 Alternate Function mapping (only for Value line devices) */
#define GPIO_Remap_TIM17            GPIO_AF_10  /*!< TIM17 Alternate Function mapping (only for Value line devices) */
#define GPIO_Remap_CEC              ((uint32_t)0x80000008)  /*!< CEC Alternate Function mapping (only for Value line devices) */
#define GPIO_Remap_TIM1_DMA         ((uint32_t)0x80000010)  /*!< TIM1 DMA requests mapping (only for Value line devices) */

#define GPIO_Remap_TIM9             ((uint32_t)0x80000020)  /*!< TIM9 Alternate Function mapping (only for XL-density devices) */
#define GPIO_Remap_TIM10            ((uint32_t)0x80000040)  /*!< TIM10 Alternate Function mapping (only for XL-density devices) */
#define GPIO_Remap_TIM11            ((uint32_t)0x80000080)  /*!< TIM11 Alternate Function mapping (only for XL-density devices) */
#define GPIO_Remap_TIM13            ((uint32_t)0x80000100)  /*!< TIM13 Alternate Function mapping (only for High density Value line and XL-density devices) */
#define GPIO_Remap_TIM14            ((uint32_t)0x80000200)  /*!< TIM14 Alternate Function mapping (only for High density Value line and XL-density devices) */
#define GPIO_Remap_FSMC_NADV        ((uint32_t)0x80000400)  /*!< FSMC_NADV Alternate Function mapping (only for High density Value line and XL-density devices) */

#define GPIO_Remap_TIM67_DAC_DMA    ((uint32_t)0x80000800)  /*!< TIM6/TIM7 and DAC DMA requests remapping (only for High density Value line devices) */
#define GPIO_Remap_TIM12            ((uint32_t)0x80001000)  /*!< TIM12 Alternate Function mapping (only for High density Value line devices) */
#define GPIO_Remap_MISC             ((uint32_t)0x80002000)  /*!< Miscellaneous Remap (DMA2 Channel5 Position and DAC Trigger remapping,
                                                                 only for High density Value line devices) */


#endif /* BOARD_HW_GPIO_REMAP_H_ */
