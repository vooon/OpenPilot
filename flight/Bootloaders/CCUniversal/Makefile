 #####
 # Project: OpenPilot
 #    
 #  
 # Makefile for OpenPilot project build PiOS and the AP.
 #
 # The OpenPilot Team, http://www.openpilot.org, Copyright (C) 2009.
 #    
 # 
 # This program is free software; you can redistribute it and/or modify 
 # it under the terms of the GNU General Public License as published by 
 # the Free Software Foundation; either version 3 of the License, or 
 # (at your option) any later version.
 # 
 # This program is distributed in the hope that it will be useful, but 
 # WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 # or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 # for more details.
 # 
 # You should have received a copy of the GNU General Public License along 
 # with this program; if not, write to the Free Software Foundation, Inc., 
 # 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 #####

# Testname is until the main Makefile is revised to pass this
TESTNAME := $(TARGET_MCU)
# STM32F103

WHEREAMI := $(dir $(lastword $(MAKEFILE_LIST)))
TOP      := $(realpath $(WHEREAMI)/../../../)
include $(TOP)/make/firmware-defs.mk
include $(TOP)/make/boards/$(BOARD_NAME)/$(TARGET_MCU)_board-info.mk

# Target MCU pushed to board-info where it belongs

# Target file name (without extension).
TARGET := bl_$(BOARD_NAME)

# Directory for output files (lst, obj, dep, elf, sym, map, hex, bin etc.)
OUTDIR := $(TOP)/build/$(TARGET)

# Set developer code and compile options
# Set to YES to compile for debugging
DEBUG ?= NO

# Set to YES to use the Servo output pins for debugging via scope or logic analyser
ENABLE_DEBUG_PINS ?= NO

# Set to Yes to enable the AUX UART which is mapped on the S1 (Tx) and S2 (Rx) servo outputs  
ENABLE_AUX_UART ?= NO

# Set to YES when using Code Sourcery toolchain
CODE_SOURCERY ?= NO

# Remove command is different for Code Sourcery on Windows
ifeq ($(CODE_SOURCERY), YES)
REMOVE_CMD = cs-rm
else
REMOVE_CMD = rm
endif

FLASH_TOOL = OPENOCD

# Paths
OPSYSTEM = .
OPSYSTEMINC = $(OPSYSTEM)/inc
OPUAVTALK = ./UAVTalk
OPUAVTALKINC = $(OPUAVTALK)/inc
OPUAVOBJ = ./UAVObjects
OPUAVOBJINC = $(OPUAVOBJ)/inc
OPTESTS  = ./Tests
OPMODULEDIR = ./Modules
FLIGHTLIB = ../../Libraries
FLIGHTLIBINC = ../../Libraries/inc
PIOS = ../../PiOS
PIOSINC = $(PIOS)/inc
PIOSTARGETMCU = $(PIOS)/$(TARGETMCU)
PIOSCOMMON = $(PIOS)/Common
PIOSBOARDS = $(PIOS)/Boards
PIOSCOMMONLIB = $(PIOSCOMMON)/Libraries

# New Vesioned Periph
EXTLIBS = ../../ExtLibraries

# Depreciating in favor of versioning and MCU agnostic
STMSPDDIR = $(EXTLIBS)/$(STDPERLIB)_$(STDPLVER)
STMUSBDIR = $(STMLIBDIR)/STM32_USB-FS-Device_Driver

# PIOSCOMMONLIB = $(PIOSCOMMON)/Libraries
PIOSCOMMONLIB = $(STMSPDDIR)/Libraries
# APPLIBDIR = $(PIOSSTM32F10X)/Libraries
APPLIBDIR = $(STMSPDDIR)/Libraries
# APPLIBDIR = $(PIOSTARGETMCU)/Libraries
# STMLIBDIR = $(APPLIBDIR)

# Depreciating in favor of versioning and MCU agnostic
STMSPDSRCDIR = $(EXTLIBS)/$(STDPERLIB)_$(STDPLVER)/$(STDPLSRC)
STMSPDINCDIR = $(EXTLIBS)/$(STDPERLIB)_$(STDPLVER)/$(STDPLINC)

STMUSBSRCDIR = $(EXTLIBS)/$(USBDIR)/src
STMUSBINCDIR = $(EXTLIBS)/$(USBDIR)/inc
CMSISDIR  = $(EXTLIBS)/$(STDPERLIB)_$(STDPLVER)/$(CMSIS)/$(CMSISVER)
DOSFSDIR  = $(APPLIBDIR)/dosfs
MSDDIR  = $(APPLIBDIR)/msd
RTOSDIR = $(APPLIBDIR)/FreeRTOS
RTOSSRCDIR = $(RTOSDIR)/Source
RTOSINCDIR = $(RTOSSRCDIR)/include
DOXYGENDIR = ../Doc/Doxygen
HWDEFSINC = ../../board_hw_defs/$(BOARD_NAME)

# List C source files here. (C dependencies are automatically generated.)
# use file-extension c for "c-only"-files

## COPTERCONTROL_BL CORE:
SRC += $(OPSYSTEM)/main.c
SRC += $(OPSYSTEMINC)/pios_board.c
SRC += $(OPSYSTEMINC)/op_dfu.c

## PIOS Hardware (STM32F10x)
include $(PIOS)/Common/CMSIS_Rules/$(TARGETMCU)_library.mk
SRC += $(PIOSTARGETMCU)/pios_sys.c
SRC += $(PIOSTARGETMCU)/pios_led.c
SRC += $(PIOSTARGETMCU)/pios_delay.c
SRC += $(PIOSTARGETMCU)/pios_usart.c
SRC += $(PIOSTARGETMCU)/pios_irq.c
SRC += $(PIOSTARGETMCU)/pios_debug.c
SRC += $(PIOSTARGETMCU)/pios_gpio.c
SRC += $(PIOSTARGETMCU)/pios_iap.c
SRC += $(PIOSTARGETMCU)/pios_bl_helper.c

# PIOS USB related files (seperated to make code maintenance more easy)
SRC += $(PIOSTARGETMCU)/pios_usb.c
SRC += $(PIOSTARGETMCU)/pios_usbhook.c
SRC += $(PIOSTARGETMCU)/pios_usb_hid.c
SRC += $(PIOSTARGETMCU)/pios_usb_hid_istr.c
SRC += $(PIOSTARGETMCU)/pios_usb_hid_pwr.c
SRC += $(OPSYSTEMINC)/pios_usb_board_data.c
SRC += $(PIOSCOMMON)/pios_usb_desc_hid_only.c
SRC += $(PIOSCOMMON)/pios_usb_util.c

## PIOS Hardware (Common)
SRC += $(PIOSCOMMON)/pios_board_info.c
SRC += $(PIOSCOMMON)/pios_com_msg.c
SRC += $(PIOSCOMMON)/printf-stdarg.c

## Libraries for flight calculations
SRC += $(FLIGHTLIB)/fifo_buffer.c

## CMSIS
SRC += $(CMSISDIR)/$(CMSISCORE)/core_cm3.c
SRC += $(CMSISDIR)/$(CMSISDEV)/system_$(TARGETMCUL).c

## Used parts of the STM-Library
SRC += $(STMSPDSRCDIR)/$(STDPLSTB)bkp.c
SRC += $(STMSPDSRCDIR)/$(STDPLSTB)crc.c
SRC += $(STMSPDSRCDIR)/$(STDPLSTB)dma.c
SRC += $(STMSPDSRCDIR)/$(STDPLSTB)exti.c
SRC += $(STMSPDSRCDIR)/$(STDPLSTB)flash.c
SRC += $(STMSPDSRCDIR)/$(STDPLSTB)gpio.c
SRC += $(STMSPDSRCDIR)/$(STDPLSTB)pwr.c
SRC += $(STMSPDSRCDIR)/$(STDPLSTB)rcc.c
SRC += $(STMSPDSRCDIR)/$(STDPLSTB)rtc.c
SRC += $(STMSPDSRCDIR)/$(STDPLSTB)spi.c
SRC += $(STMSPDSRCDIR)/$(STDPLSTB)usart.c
SRC += $(STMSPDSRCDIR)/$(STDPLSTB)dbgmcu.c
SRC += $(STMSPDSRCDIR)/misc.c 

## STM32 USB Library
SRC += $(STMUSBSRCDIR)/usb_core.c
SRC += $(STMUSBSRCDIR)/usb_init.c
SRC += $(STMUSBSRCDIR)/usb_int.c
SRC += $(STMUSBSRCDIR)/usb_mem.c
SRC += $(STMUSBSRCDIR)/usb_regs.c
SRC += $(STMUSBSRCDIR)/usb_sil.c

# List C source files here which must be compiled in ARM-Mode (no -mthumb).
# use file-extension c for "c-only"-files
## just for testing, timer.c could be compiled in thumb-mode too
SRCARM = 

# List C++ source files here.
# use file-extension .cpp for C++-files (not .C)
CPPSRC = 

# List C++ source files here which must be compiled in ARM-Mode.
# use file-extension .cpp for C++-files (not .C)
#CPPSRCARM = $(TARGET).cpp
CPPSRCARM = 

# List Assembler source files here.
# Make them always end in a capital .S. Files ending in a lowercase .s
# will not be considered source files but generated files (assembler
# output from the compiler), and will be deleted upon "make clean"!
# Even though the DOS/Win* filesystem matches both .s and .S the same,
# it will preserve the spelling of the filenames, and gcc itself does
# care about how the name is spelled on its command-line.
ASRC = $(PIOSTARGETMCU)/startup_$(TARGETMCUL)_$(MODEL)$(MODEL_SUFFIX).S

# List Assembler source files here which must be assembled in ARM-Mode..
ASRCARM = 

# List any extra directories to look for include files here.
#    Each directory must be seperated by a space.
EXTRAINCDIRS  += $(OPSYSTEM)
EXTRAINCDIRS  += $(OPSYSTEMINC)
EXTRAINCDIRS  += $(OPUAVTALK)
EXTRAINCDIRS  += $(OPUAVTALKINC)
EXTRAINCDIRS  += $(OPUAVOBJ)
EXTRAINCDIRS  += $(OPUAVOBJINC)
EXTRAINCDIRS  += $(PIOS)
EXTRAINCDIRS  += $(PIOSINC)
EXTRAINCDIRS  += $(FLIGHTLIBINC)
EXTRAINCDIRS  += $(PIOSTARGETMCU)
EXTRAINCDIRS  += $(PIOSCOMMON)
EXTRAINCDIRS  += $(PIOSBOARDS)
EXTRAINCDIRS  += $(STMSPDINCDIR)
EXTRAINCDIRS  += $(STMUSBINCDIR)
EXTRAINCDIRS  += $(CMSISDIR)
EXTRAINCDIRS  += $(DOSFSDIR)
EXTRAINCDIRS  += $(MSDDIR)
EXTRAINCDIRS  += $(RTOSINCDIR)
EXTRAINCDIRS  += $(APPLIBDIR)
EXTRAINCDIRS  += $(RTOSSRCDIR)/portable/GCC/ARM_CM3
EXTRAINCDIRS  += $(HWDEFSINC)

# List any extra directories to look for library files here.
# Also add directories where the linker should search for
# includes from linker-script to the list
#     Each directory must be seperated by a space.
#EXTRA_LIBDIRS +=

# Extra Libraries
#    Each library-name must be seperated by a space.
#    i.e. to link with libxyz.a, libabc.a and libefsl.a: 
#    EXTRA_LIBS = xyz abc efsl
# for newlib-lpc (file: libnewlibc-lpc.a):
#    EXTRA_LIBS = newlib-lpc
#EXTRA_LIBS +=

# Path to Linker-Scripts
LINKERSCRIPTPATH = $(PIOSTARGETMCU)

# Optimization level, can be [0, 1, 2, 3, s]. 
# 0 = turn off optimization. s = optimize for size.
# (Note: 3 is not always the best optimization level. See avr-libc FAQ.)

ifeq ($(DEBUG),YES)
OPT = 0
else
OPT = s
endif

# Output format. (can be ihex or binary or both)
#  binary to create a load-image in raw-binary format i.e. for SAM-BA, 
#  ihex to create a load-image in Intel hex format
#LOADFORMAT = ihex
#LOADFORMAT = binary
LOADFORMAT = both

# Debugging format.
DEBUGF = dwarf-2

# Place project-specific -D (define) and/or 
# -U options for C here.
CDEFS += -D$(TARGETMCUC)
CDEFS += -D$(TARGETMCUC)_$(MCUDENSITY)
CDEFS += -DUSE_STDPERIPH_DRIVER
CDEFS += -DUSE_$(BOARD)
ifeq ($(ENABLE_DEBUG_PINS), YES)
CDEFS += -DPIOS_ENABLE_DEBUG_PINS
endif
ifeq ($(ENABLE_AUX_UART), YES)
CDEFS += -DPIOS_ENABLE_AUX_UART
endif

# WARNING!!! CASE TRUE FALSE ISSUE WORKAROUND !!! stdbool.h is lowercase
CDEFS += -DTRUE=true
CDEFS += -DFALSE=false

# Provide (only) the bootloader with board-specific defines
BLONLY_CDEFS += -DBOARD_TYPE=$(BOARD_TYPE)
BLONLY_CDEFS += -DBOARD_REVISION=$(BOARD_REVISION)
BLONLY_CDEFS += -DHW_TYPE=$(HW_TYPE)
BLONLY_CDEFS += -DBOOTLOADER_VERSION=$(BOOTLOADER_VERSION)
BLONLY_CDEFS += -DFW_BANK_BASE=$(FW_BANK_BASE)
BLONLY_CDEFS += -DFW_BANK_SIZE=$(FW_BANK_SIZE)
BLONLY_CDEFS += -DFW_DESC_SIZE=$(FW_DESC_SIZE)
# Temporary placement as I am only working with the bootloader right now
BLONLY_CDEFS += -DSTDPLVER=$(STDPLVER)
BLONLY_CDEFS += -DSTDPERLIB=$(STDPERLIB)

# Place project-specific -D and/or -U options for 
# Assembler with preprocessor here.
#ADEFS = -DUSE_IRQ_ASM_WRAPPER
ADEFS = -D__ASSEMBLY__

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
CSTANDARD = -std=gnu99

#-----

# Compiler flags.

#  -g*:          generate debugging information
#  -O*:          optimization level
#  -f...:        tuning, see GCC manual and avr-libc documentation
#  -Wall...:     warning level
#  -Wa,...:      tell GCC to pass this to the assembler.
#    -adhlns...: create assembler listing
#
# Flags for C and C++ (arm-elf-gcc/arm-elf-g++)

ifeq ($(DEBUG),YES)
CFLAGS += -DDEBUG
endif

CFLAGS += -g$(DEBUGF)
CFLAGS += -O$(OPT)
ifeq ($(DEBUG),NO)
CFLAGS += -ffunction-sections
endif

CFLAGS += -mcpu=$(MCU)
CFLAGS += $(CDEFS)
CFLAGS += $(BLONLY_CDEFS)
CFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS)) -I.

CFLAGS += -mapcs-frame 
CFLAGS += -fomit-frame-pointer
ifeq ($(CODE_SOURCERY), YES)
CFLAGS += -fpromote-loop-indices
endif

CFLAGS += -Wall 
CFLAGS += -Werror
CFLAGS += -Wa,-adhlns=$(addprefix $(OUTDIR)/, $(notdir $(addsuffix .lst, $(basename $<))))
# Compiler flags to generate dependency files:
CFLAGS += -MD -MP -MF $(OUTDIR)/dep/$(@F).d

# flags only for C
#CONLYFLAGS += -Wnested-externs 
CONLYFLAGS += $(CSTANDARD)

# Assembler flags.
#  -Wa,...:    tell GCC to pass this to the assembler.
#  -ahlns:     create listing
ASFLAGS  = -mcpu=$(MCU) -I. -x assembler-with-cpp
ASFLAGS += $(ADEFS)
ASFLAGS += -Wa,-adhlns=$(addprefix $(OUTDIR)/, $(notdir $(addsuffix .lst, $(basename $<))))
ASFLAGS += $(patsubst %,-I%,$(EXTRAINCDIRS))

MATH_LIB = -lm

# Linker flags.
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LDFLAGS = -nostartfiles -Wl,-Map=$(OUTDIR)/$(TARGET).map,--cref,--gc-sections
ifeq ($(DEBUG),NO)
LDFLAGS += -Wl,-static
endif
LDFLAGS += $(patsubst %,-L%,$(EXTRA_LIBDIRS))
LDFLAGS += -lc
LDFLAGS += $(patsubst %,-l%,$(EXTRA_LIBS))
LDFLAGS += $(MATH_LIB)
LDFLAGS += -lc -lgcc 

# Set linker-script name depending on selected submodel name
LDFLAGS += -T$(LINKERSCRIPTPATH)/link_$(BOARD)_memory.ld
LDFLAGS += -T$(LINKERSCRIPTPATH)/link_$(BOARD)_BL_sections.ld

# Define programs and commands.
REMOVE  = $(REMOVE_CMD) -f

# List of all source files.
ALLSRC     = $(ASRCARM) $(ASRC) $(SRCARM) $(SRC) $(CPPSRCARM) $(CPPSRC)
# List of all source files without directory and file-extension.
ALLSRCBASE = $(notdir $(basename $(ALLSRC)))

# Define all object files.
ALLOBJ     = $(addprefix $(OUTDIR)/, $(addsuffix .o, $(ALLSRCBASE)))

# Define all listing files (used for make clean).
LSTFILES   = $(addprefix $(OUTDIR)/, $(addsuffix .lst, $(ALLSRCBASE)))
# Define all depedency-files (used for make clean).
DEPFILES   = $(addprefix $(OUTDIR)/dep/, $(addsuffix .o.d, $(ALLSRCBASE)))

# Default target.
all: patch 

patch: 
#    $(V1) echo patch -p0 flight/ExtLibraries/Patches/hw_config.h --input=flight/ExtLibraries/Patches/STM32F10x_USB.diff
	$(shell patch -p0 $(TOP)/flight/ExtLibraries/Patches/hw_config.h --input=$(TOP)/flight/ExtLibraries/Patches/STM32F10x_USB.diff)
	$(shell patch -p0 $(TOP)/flight/ExtLibraries/STM32_USB-FS-Device_Lib_V4.0.0/Libraries/STM32_USB-FS-Device_Driver/inc/usb_lib.h --input=$(TOP)/flight/ExtLibraries/Patches/STM32F10x_USB-2.diff)
	@echo "Done Patching"
ifeq ($(LOADFORMAT),ihex)
build: elf hex sym
else 
ifeq ($(LOADFORMAT),binary)
build: elf bin sym
else 
ifeq ($(LOADFORMAT),both)
build: elf hex bin sym
else 
$(error "$(MSG_FORMATERROR) $(FORMAT)")
endif
endif
endif

${OUTDIR}/InitMods.c: Makefile
	@echo $(MSG_MODINIT) $(call toprel, $@)
	@echo ${quote}// Autogenerated file${quote} > ${OUTDIR}/InitMods.c 
	@echo ${quote}${foreach MOD, ${MODNAMES}, extern unsigned int ${MOD}Initialize(void);}${quote}  >> ${OUTDIR}/InitMods.c
	@echo ${quote}void InitModules() {${quote} >> ${OUTDIR}/InitMods.c
	@echo ${quote}${foreach MOD, ${MODNAMES}, ${MOD}Initialize();}${quote}  >> ${OUTDIR}/InitMods.c
	@echo ${quote}}${quote} >> ${OUTDIR}/InitMods.c

# Link: create ELF output file from object files.
$(eval $(call LINK_TEMPLATE, $(OUTDIR)/$(TARGET).elf, $(ALLOBJ)))

# Assemble: create object files from assembler source files.
$(foreach src, $(ASRC), $(eval $(call ASSEMBLE_TEMPLATE, $(src)))) 

# Assemble: create object files from assembler source files. ARM-only
$(foreach src, $(ASRCARM), $(eval $(call ASSEMBLE_ARM_TEMPLATE, $(src)))) 

# Compile: create object files from C source files.
$(foreach src, $(SRC), $(eval $(call COMPILE_C_TEMPLATE, $(src)))) 

# Compile: create object files from C source files. ARM-only
$(foreach src, $(SRCARM), $(eval $(call COMPILE_C_ARM_TEMPLATE, $(src)))) 

# Compile: create object files from C++ source files.
$(foreach src, $(CPPSRC), $(eval $(call COMPILE_CPP_TEMPLATE, $(src)))) 

# Compile: create object files from C++ source files. ARM-only
$(foreach src, $(CPPSRCARM), $(eval $(call COMPILE_CPP_ARM_TEMPLATE, $(src)))) 

# Compile: create assembler files from C source files. ARM/Thumb
$(eval $(call PARTIAL_COMPILE_TEMPLATE, SRC))

# Compile: create assembler files from C source files. ARM only
$(eval $(call PARTIAL_COMPILE_ARM_TEMPLATE, SRCARM))

$(OUTDIR)/$(TARGET).bin.o: $(OUTDIR)/$(TARGET).bin

# Add jtag targets (program and wipe)
$(eval $(call JTAG_TEMPLATE,$(OUTDIR)/$(TARGET).bin,$(BL_BANK_BASE),$(BL_BANK_SIZE),$(OPENOCD_JTAG_CONFIG),$(OPENOCD_CONFIG)))

.PHONY: elf lss sym hex bin bino
elf: $(OUTDIR)/$(TARGET).elf
lss: $(OUTDIR)/$(TARGET).lss 
sym: $(OUTDIR)/$(TARGET).sym
hex: $(OUTDIR)/$(TARGET).hex
bin: $(OUTDIR)/$(TARGET).bin
bino: $(OUTDIR)/$(TARGET).bin.o

# Display sizes of sections.
$(eval $(call SIZE_TEMPLATE, $(OUTDIR)/$(TARGET).elf))

# Generate Doxygen documents
docs:
	doxygen  $(DOXYGENDIR)/doxygen.cfg

# Install: install binary file with prefix/suffix into install directory
install: $(OUTDIR)/$(TARGET).bin
ifneq ($(INSTALL_DIR),)
	@echo $(MSG_INSTALLING) $(call toprel, $<)
	$(V1) mkdir -p $(INSTALL_DIR)
	$(V1) $(INSTALL) $< $(INSTALL_DIR)/$(INSTALL_PFX)$(TARGET)$(INSTALL_SFX).bin
else
	$(error INSTALL_DIR must be specified for $@)
endif

# Target: clean project.
clean: clean_list

clean_list :
	@echo $(MSG_CLEANING)
	$(V1) $(REMOVE) $(OUTDIR)/$(TARGET).map
	$(V1) $(REMOVE) $(OUTDIR)/$(TARGET).elf
	$(V1) $(REMOVE) $(OUTDIR)/$(TARGET).hex
	$(V1) $(REMOVE) $(OUTDIR)/$(TARGET).bin
	$(V1) $(REMOVE) $(OUTDIR)/$(TARGET).sym
	$(V1) $(REMOVE) $(OUTDIR)/$(TARGET).lss
	$(V1) $(REMOVE) $(OUTDIR)/$(TARGET).bin.o
	$(V1) $(REMOVE) $(ALLOBJ)
	$(V1) $(REMOVE) $(LSTFILES)
	$(V1) $(REMOVE) $(DEPFILES)
	$(V1) $(REMOVE) $(SRC:.c=.s)
	$(V1) $(REMOVE) $(SRCARM:.c=.s)
	$(V1) $(REMOVE) $(CPPSRC:.cpp=.s)
	$(V1) $(REMOVE) $(CPPSRCARM:.cpp=.s)


# Create output files directory
# all known MS Windows OS define the ComSpec environment variable
ifdef ComSpec
$(shell md $(subst /,\\,$(OUTDIR)) 2>NUL)
else
$(shell mkdir -p $(OUTDIR) 2>/dev/null)
endif

# Include the dependency files.
ifdef ComSpec
-include $(shell md $(subst /,\\,$(OUTDIR))\dep 2>NUL) $(wildcard $(OUTDIR)/dep/*)
else
-include $(shell mkdir -p $(OUTDIR) 2>/dev/null) $(shell mkdir $(OUTDIR)/dep 2>/dev/null) $(wildcard $(OUTDIR)/dep/*)
endif

# Listing of phony targets.
.PHONY : all patch build clean clean_list install
