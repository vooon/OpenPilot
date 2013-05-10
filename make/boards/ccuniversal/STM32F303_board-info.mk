BOARD_TYPE          := 0x04
BOARD_REVISION      := 0x02
BOOTLOADER_VERSION  := 0x03
HW_TYPE             := 0x01

MCU                 := cortex-m3
MCUCORE             := ARM_CM3
# Target MCU upper and lowercase names
TARGETMCU           := STM32F30x
TARGETMCUL          := stm32f30x
TARGETMCUC          := STM32F30X
# Name and Version of the Std Periph Library
STDPERDVR           := STM32F30x_StdPeriph_Driver
STDPERLIB           := STM32F30x_DSP_StdPeriph_Lib
STDPLVER            := V1.0.0
STDPLINC            := Libraries/STM32F30x_StdPeriph_Driver/inc
STDPLSRC            := Libraries/STM32F30x_StdPeriph_Driver/src
STDPLCMSIS          := Libraries/CMSIS/Device/ST/STM32F30x/Include
# Poor naming convention, need filename stub
STDPLSTB            := stm32f30x_
CHIP                := STM32F303CCT
CHIPBOARD           := STM32F303CCT_CC_Rev1
BOARD               := STM32103CB_CC_Rev1
# Model does not mean much, density is is correct and descriptive
MODEL               := MD
MODEL_SUFFIX        := _CC

# RTOS Selection for this board
RTOS                := FreeRTOS
RTOSVER             := V7.4.2
CMSIS               := Libraries/CMSIS
CMSISVER            := Device
CMSISDEV            := Device/ST/STM32F30x/Include
CMSISCORE           := Include
# Compiler corresponds to the FreeRTOS portable compiler directory being used
COMPILER            := GCC

# Cryptic name due to length, CMSIS VENDOR and VERSION
CMVEN               := STMicro
CMVENVER            := UNKNOWN

# Set the actual crystal frequency, defaults to 8MHz if unset
HSE_VALUE = 16000000

# Enable ARM DSP library
USE_DSP_LIB = NO
#CMSISDSP            := Legacy/CMSIS2/DSP_Lib/Source

# USB Drivers
#USBDIR              := Legacy/STM32_USB-FS-Device_Driver
USBLIB              := STM32_USB-FS-Device_Lib
USBVER              := V4.0.0
USBDVR              := STM32_USB-FS-Device_Driver
USBINC              := Libraries/STM32_USB-FS-Device_Driver/inc
USBSRC              := Libraries/STM32_USB-FS-Device_Driver/src
USBDIR              := $(USBLIB)_$(USBVER)/Libraries/$(USBDVR)

# Patches required, make silliness requires funny numbering. This should actually be XML
NUMPATCHES  := 1 2 3 4
TARGET_1    := ../../ExtLibraries/Patches/hw_config.h
PATCH_1     := ../../ExtLibraries/Patches/STM32F30x_USB.diff
TARGET_2    := ../../ExtLibraries/STM32_USB-FS-Device_Lib_V4.0.0/Libraries/STM32_USB-FS-Device_Driver/inc/usb_lib.h
PATCH_2     := ../../ExtLibraries/Patches/hw_config-location.diff
TARGET_3    := ../../ExtLibraries/STM32_USB-FS-Device_Lib_V4.0.0/Libraries/STM32_USB-FS-Device_Driver/inc/usb_type.h
PATCH_3     := ../../ExtLibraries/Patches/STM32F30x_USB-3.diff
TARGET_4    := ../../ExtLibraries/STM32_USB-FS-Device_Lib_V4.0.0/Libraries/STM32F30x_StdPeriph_Driver/inc/stm32f30x_rcc.h
PATCH_4     := ../../ExtLibraries/Patches/STM32F30x_rcc.diff

OPENOCD_JTAG_CONFIG := foss-jtag.revb.cfg
OPENOCD_CONFIG      := stm32f1x.cfg

# Note: These must match the values in link_$(BOARD)_memory.ld
# F302xx and 3xx Are reserved from 0x0004 0000 to 0x0800 0000
BL_BANK_BASE        := 0x08000000  # Start of bootloader flash
BL_BANK_SIZE        := 0x00003000  # Should include BD_INFO region
FW_BANK_BASE        := 0x08003000  # Start of firmware flash
FW_BANK_SIZE        := 0x0001D000  # Should include FW_DESC_SIZE

FW_DESC_SIZE        := 0x00000064
