/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_CYRF6936 CYRF6936 Radio Functions
 * @brief CYRF6936 Radio functionality
 * @{
 *
 * @file       pios_cyrf6963.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      CYRF6936 Radio Functions
 *                 Full credits to DeviationTX Project, http://www.deviationtx.com/
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/

/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Deviation is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <pios.h>

#if defined(PIOS_INCLUDE_CYRF6936)

#define CS_HI() (GPIO_SetBits(GPIOA, GPIO_Pin_4))
#define CS_LO() (GPIO_ResetBits(GPIOA, GPIO_Pin_4))
#define RS_HI() GPIO_SetBits(GPIOA, GPIO_Pin_0)
#define RS_LO() GPIO_ResetBits(GPIOA, GPIO_Pin_0)

void Delay(uint32_t);

static uint8_t dummy;
int32_t CYRF_TransferByte(uint16_t b)
{
	uint16_t rx_byte;

	/*
	 * Procedure taken from STM32F10xxx Reference Manual section 23.3.5
	 */

	/* Make sure the RXNE flag is cleared by reading the DR register */
	//dummy = SPI1->DR;

	/* Start the transfer */
	SPI1->DR = b;

	/* Wait until there is a byte to read */
	while (!(SPI1->SR & SPI_I2S_FLAG_RXNE)) ;
	//while (SPI2->SR & SPI_I2S_FLAG_BSY) ;

	/* Read the rx'd byte */
	rx_byte = SPI1->DR;

	/* Wait until the TXE goes high */
	while (!(SPI1->SR & SPI_I2S_FLAG_TXE)) ;

	/* Wait for SPI transfer to have fully completed */
	while (SPI1->SR & SPI_I2S_FLAG_BSY) ;

	/* Return received byte */
	return rx_byte;
}

void CYRF_WriteRegister(u8 address, u8 data)
{
	CS_LO();
	CYRF_TransferByte(0x80 | address);
	CYRF_TransferByte(data);
	CS_HI();
}



static void WriteRegisterMulti(u8 address, const u8 *data, u8 length)
{
    unsigned char i;
	CS_LO();
    CYRF_TransferByte(0x80 | address);
    for(i = 0; i < length; i++)
    {
    	CYRF_TransferByte(data[i]);
    }
    CS_HI();
}

void ReadRegisterMulti(u8 address, u8 *data, u8 length)
{
    unsigned char i;
	CS_LO();
	CYRF_TransferByte(address);
    for(i = 0; i < length; i++)
    {
    	data[i]=CYRF_TransferByte(0x01);
    }
    CS_HI();
}

u8 CYRF_ReadRegister(u8 address)
{
    u8 data;

	CS_LO();
    data = CYRF_TransferByte(address);
    data = CYRF_TransferByte(0x01);
    CS_HI();
    return data;
}

void CYRF_Reset()
{
    /* Reset the CYRF chip */
    RS_HI();
    PIOS_DELAY_WaituS(50);
    RS_LO();
    PIOS_DELAY_WaituS(50);
}

void CYRF_Initialize()
{

	/* Enable SPI2 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    //rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_SPI2EN);
    /* Enable GPIOB */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    //rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);

//reset
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//cs
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//mosi
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//sck
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//miso
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//irq
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

    CS_HI();
    RS_LO();
    //GPIO_PinRemapConfig(GPIO_Remap_SPI1, DISABLE);

	SPI_Cmd(SPI1,DISABLE);
	SPI_InitTypeDef spi_init;
	SPI_StructInit(&spi_init);
	spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spi_init.SPI_Mode = SPI_Mode_Master;
	spi_init.SPI_DataSize = SPI_DataSize_8b;
	spi_init.SPI_CPOL = SPI_CPOL_Low;
	//spi_init.SPI_CPOL = SPI_CPOL_High;
	spi_init.SPI_CPHA = SPI_CPHA_1Edge;
	//spi_init.SPI_CPHA = SPI_CPHA_2Edge;
	spi_init.SPI_NSS = SPI_NSS_Soft;
	spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
	spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
	spi_init.SPI_CRCPolynomial = 7;

	SPI_Init(SPI1,&spi_init);
	//SPI_NSSInternalSoftwareConfig(SPI2, SPI_NSSInternalSoft_Set);
	SPI_CalculateCRC(SPI1, DISABLE);
	SPI_Cmd(SPI1,ENABLE);

	CYRF_Reset();
}

u8 CYRF_MaxPower()
{
    return CYRF_PWR_10MW;
}
/*
 *
 */
void CYRF_GetMfgData(u8 data[])
{
    /* Fuses power on */
    CYRF_WriteRegister(0x25, 0xFF);

    ReadRegisterMulti(0x25, data, 6);

    /* Fuses power off */
    CYRF_WriteRegister(0x25, 0x00);
}
/*
 * 1 - Tx else Rx
 */
void CYRF_ConfigRxTx(u32 TxRx)
{
    if(TxRx)
    {
        CYRF_WriteRegister(0x0E,0x80);
        CYRF_WriteRegister(0x0F,0x2C);
    }
    else
    {
        CYRF_WriteRegister(0x0E,0x20);
        CYRF_WriteRegister(0x0F,0x28);
    }
}
/*
 *
 */
void CYRF_ConfigRFChannel(u8 ch)
{
    CYRF_WriteRegister(0x00,ch);
}

void CYRF_SetPower(u8 power)
{
    u8 val = CYRF_ReadRegister(0x03);
    CYRF_WriteRegister(0x03, val | (power & 0x07));
}

/*
 *
 */
void CYRF_ConfigCRCSeed(u16 crc)
{
    CYRF_WriteRegister(0x15,crc & 0xff);
    CYRF_WriteRegister(0x16,crc >> 8);
}
/*
 * these are the recommended sop codes from Crpress
 * See "WirelessUSB LP/LPstar and PRoC LP/LPstar Technical Reference Manual"
 */
void CYRF_ConfigSOPCode(const u8 *sopcodes)
{
    //NOTE: This can also be implemented as:
    //for(int i = 0; i < 8; i++) CYRF_WriteRegister(0x23, sopcodes[i]);
    WriteRegisterMulti(0x22, sopcodes, 8);
}

void CYRF_ConfigDataCode(const u8 *datacodes, u8 len)
{
    //NOTE: This can also be implemented as:
    //for(int i = 0; i < len; i++) CYRF_WriteRegister(0x23, datacodes[i]);
    WriteRegisterMulti(0x23, datacodes, len);
}

void CYRF_WritePreamble(u32 preamble)
{
	CS_LO();
    CYRF_TransferByte(0x80 | 0x24);
    CYRF_TransferByte(preamble & 0xff);
    CYRF_TransferByte((preamble >> 8) & 0xff);
    CYRF_TransferByte((preamble >> 16) & 0xff);
    CS_HI();
}
/*
 *
 */
void CYRF_StartReceive()
{
    CYRF_WriteRegister(0x05,0x87);
}

void CYRF_ReadDataPacket(u8 dpbuffer[])
{
    ReadRegisterMulti(0x21, dpbuffer, 0x10);
}

void CYRF_WriteDataPacketLen(u8 dpbuffer[], u8 len)
{
    CYRF_WriteRegister(CYRF_01_TX_LENGTH, len);
    CYRF_WriteRegister(0x02, 0x40);
    WriteRegisterMulti(0x20, dpbuffer, len);
    CYRF_WriteRegister(0x02, 0xBF);
}
void CYRF_WriteDataPacket(u8 dpbuffer[])
{
    CYRF_WriteDataPacketLen(dpbuffer, 16);
}

u8 CYRF_ReadRSSI(u32 dodummyread)
{
    u8 result;
    if(dodummyread)
    {
        result = CYRF_ReadRegister(0x13);
    }
    result = CYRF_ReadRegister(0x13);
    if(result & 0x80)
    {
        result = CYRF_ReadRegister(0x13);
    }
    return (result & 0x0F);
}

//NOTE: This routine will reset the CRC Seed
void CYRF_FindBestChannels(u8 *channels, u8 len, u8 minspace, u8 min, u8 max)
{
    #define NUM_FREQ 80
    #define FREQ_OFFSET 4
    u8 rssi[NUM_FREQ];

    if (min < FREQ_OFFSET)
        min = FREQ_OFFSET;
    if (max > NUM_FREQ)
        max = NUM_FREQ;

    int i;
    int j;
    memset(channels, 0, sizeof(u8) * len);
    CYRF_ConfigCRCSeed(0x0000);
    CYRF_ConfigRxTx(0);
    //Wait for pre-amp to switch from sned to receive
    PIOS_DELAY_WaituS(1000);
    for(i = 0; i < NUM_FREQ; i++) {
        CYRF_ConfigRFChannel(i + FREQ_OFFSET);
        CYRF_ReadRegister(0x13);
        CYRF_StartReceive();
        PIOS_DELAY_WaituS(10);
        rssi[i] = CYRF_ReadRegister(0x13);
    }

    for (i = 0; i < len; i++) {
        channels[i] = min;
        for (j = min; j < max; j++) {
            if (rssi[j] < rssi[channels[i]]) {
                channels[i] = j;
            }

        }
        for (j = channels[i] - minspace; j < channels[i] + minspace; j++) {
            //Ensure we don't reuse any channels within minspace of the selected channel again
            if (j < 0 || j >= NUM_FREQ)
                continue;
            rssi[j] = 0xff;
        }
    }
    CYRF_ConfigRxTx(1);
}

#endif
