BOARD_TYPE          := 0x07
BOARD_REVISION      := 0x01
BOOTLOADER_VERSION  := 0x04
HW_TYPE             := 0x01

MCU                 := cortex-m3
CHIP                := STM32F103CBT
BOARD               := STM32103CB_ANYTX
MODEL               := MD
MODEL_SUFFIX        := _PX

OPENOCD_JTAG_CONFIG := floss-jtag.reva.cfg
OPENOCD_CONFIG      := stm32f1x.cfg

# Flash memory map for AnyTX:
# Sector	start			size	use
# 0			0x0800 0000		1k		BL
# 1			0x0800 0400		1k		BL
# ..								..
# 10		0x0800 2C00		1k		BL
# 11		0x0800 3000		1k		FW
# 12		0x0800 1000		1k		FW
# ..								..
# 123 		0x0801 EC00		1k		FW
# 124 		0x0801 F000		1k		EE
# ..								..
# 127 		0x0801 FC00		1k		EE


# Note: These must match the values in link_$(BOARD)_memory.ld
BL_BANK_BASE        := 0x08000000  # Start of bootloader flash
BL_BANK_SIZE        := 0x00003000  # Should include BD_INFO region
FW_BANK_BASE        := 0x08003000  # Start of firmware flash
FW_BANK_SIZE        := 0x0001C000  # Should include FW_DESC_SIZE
EE_BANK_BASE        := 0x0801F000  # EEPROM storage area
EE_BANK_SIZE        := 0x00001000  # Size of EEPROM storage area

FW_DESC_SIZE        := 0x00000064
