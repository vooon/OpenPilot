#ifndef PIOS_CYRF6936_H
#define PIOS_CYRF6936_H

#include <pios.h>
#if defined(PIOS_INCLUDE_CYRF6936)

#include <pios_stm32.h>
#include <pios_spi_priv.h>


struct pios_cyrf6936_cfg {
    const struct pios_spi_cfg  cyrf_spi;
    const struct pios_exti_cfg *cyrf_irq;
    struct stm32_gpio cyrf_rs;
    struct stm32_gpio cyrf_cs;
/*	TIM_TimeBaseInitTypeDef tim_base_init;
        TIM_OCInitTypeDef tim_oc_init;
        const struct pios_tim_channel * channels;*/
};


enum {
    CYRF_01_TX_LENGTH      = 0x01,
    CYRF_02_TX_CTRL        = 0x02,
    CYRF_03_TX_CFG         = 0x03,
    CYRF_04_TX_IRQ_STATUS  = 0x04,
    CYRF_05_RX_CTRL        = 0x05,
    CYRF_06_RX_CFG         = 0x06,
    CYRF_07_RX_IRG_STATUS  = 0x07,
    CYRF_09_RX_COUNT       = 0x09,
    CYRF_0A_RX_LENGTH      = 0x0A,
    CYRF_0B_PWR_CTRL       = 0x0B,
    CYRF_0C_XTAL_CTRL      = 0x0C,
    CYRF_0D_IO_CFG         = 0x0D,
    CYRF_0E_GPIO_CTRL      = 0x0E,
    CYRF_0F_XACT_CFG       = 0x0F,
    CYRF_10_FRAMING_CFG    = 0x10,
    CYRF_11_DATA32_THOLD   = 0x11,
    CYRF_12_DATA64_THOLD   = 0x12,
    CYRF_13_RSSI = 0x13,
    CYRF_14_EOP_CTRL       = 0x14,
    CYRF_1B_TX_OFFSET_LSB  = 0x1B,
    CYRF_1C_TX_OFFSET_MSB  = 0x1C,
    CYRF_1D_MODE_OVERRIDE  = 0x1D,
    CYRF_1E_RX_OVERRIDE    = 0x1E,
    CYRF_1F_TX_OVERRIDE    = 0x1F,
    CYRF_27_CLK_OVERRIDE   = 0x27,
    CYRF_28_CLK_EN         = 0x28,
    CYRF_29_RX_ABORT       = 0x29,
    CYRF_32_AUTO_CAL_TIME  = 0x32,
    CYRF_35_AUTOCAL_OFFSET = 0x35,
    CYRF_39_ANALOG_CTRL    = 0x39,
};

enum CYRF_PWR {
    CYRF_PWR_100MW,
    CYRF_PWR_10MW,
    CYRF_PWR_DEFAULT,
};


/* SPI CYRF6936 */
extern void CYRF_Initialize(const struct pios_cyrf6936_cfg *cfg);
extern void CYRF_Reset();
extern void CYRF_GetMfgData(u8 data[]);

extern void CYRF_ConfigRxTx(u32 TxRx);
extern void CYRF_ConfigRFChannel(u8 ch);
extern void CYRF_SetPower(u8 power);
extern void CYRF_ConfigCRCSeed(u16 crc);
extern void CYRF_StartReceive();
extern void CYRF_ConfigSOPCode(const u8 *sopcodes);
extern void CYRF_ConfigDataCode(const u8 *datacodes, u8 len);
extern u8 CYRF_ReadRSSI(u32 dodummyread);
extern void CYRF_ReadDataPacket(u8 dpbuffer[]);
extern void CYRF_WriteDataPacket(u8 dpbuffer[]);
extern void CYRF_WriteDataPacketLen(u8 dpbuffer[], u8 len);
extern void CYRF_WriteRegister(u8 address, u8 data);
extern u8 CYRF_ReadRegister(u8 address);
extern void CYRF_WritePreamble(u32 preamble);
extern u8 CYRF_MaxPower();
extern void CYRF_FindBestChannels(u8 *channels, u8 len, u8 minspace, u8 minchan, u8 maxchan);
extern bool PIOS_CYRF_ISR();

extern void PIOS_CYRFTMR_Stop();
extern void PIOS_CYRFTMR_Start();
extern void PIOS_CYRFTMR_Set(u16 timer);

#endif // if defined(PIOS_INCLUDE_CYRF6936)

#endif /* PIOS_CYRF6936_H */
