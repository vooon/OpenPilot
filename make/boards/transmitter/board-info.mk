BOARD_TYPE          := 0x04
BOARD_REVISION      := 0x01
BOOTLOADER_VERSION  := 0x01
HW_TYPE             := 0x01

MCU                 := cortex-m3
#CHIP                := STM32F103CBT
CHIP                := STM32F103C8T
#BOARD               := STM32103CB_Transmitter
BOARD               := STM32103C8_Transmitter
MODEL               := MD
MODEL_SUFFIX        := _Transmitter

OPENOCD_CONFIG      := stm32f1x.cfg

# Note: These must match the values in link_$(BOARD)_memory.ld
BL_BANK_BASE        := 0x08000000  # Start of bootloader flash
BL_BANK_SIZE        := 0x00003000  # Should include BD_INFO region
FW_BANK_BASE        := 0x08003000  # Start of firmware flash
FW_BANK_SIZE        := 0x0000D000  # Should include FW_DESC_SIZE

FW_DESC_SIZE        := 0x00000064
