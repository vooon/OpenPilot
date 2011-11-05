BOARD_TYPE          := 0x05
BOARD_REVISION      := 0x01
BOOTLOADER_VERSION  := 0x00
HW_TYPE             := 0x01

MCU                 := cortex-m3
CHIP                := STM32F103CBT
BOARD               := STM32103CB_OPOSD
MODEL               := MD
MODEL_SUFFIX        := _CC

# Note: These must match the values in link_$(BOARD)_memory.ld
BL_BANK_BASE        := 0x08000000  # Start of bootloader flash
BL_BANK_SIZE        := 0x00003000  # Should include BD_INFO region
FW_BANK_BASE        := 0x08003000  # Start of firmware flash
FW_BANK_SIZE        := 0x0001CC00  # Should include FW_DESC_SIZE
EE_BANK_BASE        := 0x0801FC00  # EEPROM storage area
EE_BANK_SIZE        := 0x00000400  # Size of EEPROM storage area

FW_DESC_SIZE        := 0x00000064