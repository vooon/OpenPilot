# Set up a default goal
.DEFAULT_GOAL := help

# Set up some macros for common directories within the tree
ROOT_DIR=$(CURDIR)
TOOLS_DIR=$(ROOT_DIR)/tools
BUILD_DIR=$(ROOT_DIR)/build
DL_DIR=$(ROOT_DIR)/downloads

# Clean out undesirable variables from the environment and command-line
# to remove the chance that they will cause problems with our build
define SANITIZE_VAR
$(if $(filter-out undefined,$(origin $(1))),
  $(info *NOTE*      Sanitized $(2) variable '$(1)' from $(origin $(1)))
  MAKEOVERRIDES = $(filter-out $(1)=%,$(MAKEOVERRIDES))
  override $(1) :=
  unexport $(1)
)
endef

# These specific variables can influence gcc in unexpected (and undesirable) ways
SANITIZE_GCC_VARS := TMPDIR GCC_EXEC_PREFIX COMPILER_PATH LIBRARY_PATH
SANITIZE_GCC_VARS += CFLAGS CPATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH OBJC_INCLUDE_PATH DEPENDENCIES_OUTPUT
$(foreach var, $(SANITIZE_GCC_VARS), $(eval $(call SANITIZE_VAR,$(var),disallowed)))

# These specific variables used to be valid but now they make no sense
SANITIZE_DEPRECATED_VARS := USE_BOOTLOADER
$(foreach var, $(SANITIZE_DEPRECATED_VARS), $(eval $(call SANITIZE_VAR,$(var),deprecated)))

# We almost need to consider autoconf/automake instead of this
# I don't know if windows supports uname :-(
QT_SPEC=win32-g++
UAVOBJGENERATOR="$(BUILD_DIR)/ground/uavobjgenerator/debug/uavobjgenerator.exe"
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
  QT_SPEC=linux-g++
  UAVOBJGENERATOR="$(BUILD_DIR)/ground/uavobjgenerator/uavobjgenerator"
endif
ifeq ($(UNAME), Darwin)
  QT_SPEC=macx-g++
  UAVOBJGENERATOR="$(BUILD_DIR)/ground/uavobjgenerator/uavobjgenerator"
endif

# OpenPilot GCS build configuration (debug | release)
GCS_BUILD_CONF ?= debug

# Set up misc host tools
RM=rm

# Decide on a verbosity level based on the V= parameter
export AT := @

ifndef V
export V0    :=
export V1    := $(AT)
else ifeq ($(V), 0)
export V0    := $(AT)
export V1    := $(AT)
else ifeq ($(V), 1)
endif

.PHONY: help
help:
	@echo
	@echo "   This Makefile is known to work on Linux and Mac in a standard shell environment."
	@echo "   It also works on Windows by following the instructions in make/winx86/README.txt."
	@echo
	@echo "   Here is a summary of the available targets:"
	@echo
	@echo "   [Tool Installers]"
	@echo "     qt_sdk_install       - Install the QT v4.6.2 tools"
	@echo "     arm_sdk_install      - Install the Code Sourcery ARM gcc toolchain"
	@echo "     openocd_install      - Install the OpenOCD JTAG daemon"
	@echo "     stm32flash_install   - Install the stm32flash tool for unbricking boards"
	@echo
	@echo "   [Big Hammer]"
	@echo "     all                  - Generate UAVObjects, build openpilot firmware and gcs"
	@echo "     all_flight           - Build all firmware, bootloaders and bootloader updaters"
	@echo "     all_fw               - Build only firmware for all boards"
	@echo "     all_bl               - Build only bootloaders for all boards"
	@echo "     all_bu               - Build only bootloader updaters for all boards"
	@echo
	@echo "     all_clean            - Remove your build directory ($(BUILD_DIR))"
	@echo "     all_flight_clean     - Remove all firmware, bootloaders and bootloader updaters"
	@echo "     all_fw_clean         - Remove firmware for all boards"
	@echo "     all_bl_clean         - Remove bootlaoders for all boards"
	@echo "     all_bu_clean         - Remove bootloader updaters for all boards"
	@echo
	@echo "     all_<board>          - Build all available images for <board>"
	@echo "     all_<board>_clean    - Remove all available images for <board>"
	@echo
	@echo "   [Firmware]"
	@echo "     <board>              - Build firmware for <board>"
	@echo "                            supported boards are ($(ALL_BOARDS))"
	@echo "     fw_<board>           - Build firmware for <board>"
	@echo "                            supported boards are ($(FW_BOARDS))"
	@echo "     fw_<board>_clean     - Remove firmware for <board>"
	@echo "     fw_<board>_program   - Use OpenOCD + JTAG to write firmware to <board>"
	@echo
	@echo "   [Bootloader]"
	@echo "     bl_<board>           - Build bootloader for <board>"
	@echo "                            supported boards are ($(BL_BOARDS))"
	@echo "     bl_<board>_clean     - Remove bootloader for <board>"
	@echo "     bl_<board>_program   - Use OpenOCD + JTAG to write bootloader to <board>"
	@echo
	@echo "   [Bootloader Updater]"
	@echo "     bu_<board>           - Build bootloader updater for <board>"
	@echo "                            supported boards are ($(BU_BOARDS))"
	@echo "     bu_<board>_clean     - Remove bootloader updater for <board>"
	@echo
	@echo "   [Unbrick a board]"
	@echo "     unbrick_<board>      - Use the STM32's built in boot ROM to write a bootloader to <board>"
	@echo "                            supported boards are ($(BL_BOARDS))"
	@echo
	@echo "   [Simulation]"
	@echo "     sim_posix            - Build OpenPilot simulation firmware for"
	@echo "                            a POSIX compatible system (Linux, Mac OS X, ...)"
	@echo "     sim_posix_clean      - Delete all build output for the POSIX simulation"
	@echo "     sim_win32            - Build OpenPilot simulation firmware for"
	@echo "                            Windows using mingw and msys"
	@echo "     sim_win32_clean      - Delete all build output for the win32 simulation"
	@echo
	@echo "   [GCS]"
	@echo "     gcs                  - Build the Ground Control System (GCS) application"
	@echo "     gcs_clean            - Remove the Ground Control System (GCS) application"
	@echo
	@echo "   [UAVObjects]"
	@echo "     uavobjects           - Generate source files from the UAVObject definition XML files"
	@echo "     uavobjects_test      - parse xml-files - check for valid, duplicate ObjId's, ... "
	@echo "     uavobjects_<group>   - Generate source files from a subset of the UAVObject definition XML files"
	@echo "                            supported groups are ($(UAVOBJ_TARGETS))"
	@echo
	@echo "   Note: All tools will be installed into $(TOOLS_DIR)"
	@echo "         All build output will be placed in $(BUILD_DIR)"
	@echo

.PHONY: all
all: uavobjects all_ground all_flight

.PHONY: all_clean
all_clean:
	[ ! -d "$(BUILD_DIR)" ] || $(RM) -rf "$(BUILD_DIR)"

$(DL_DIR):
	mkdir -p $@

$(TOOLS_DIR):
	mkdir -p $@

$(BUILD_DIR):
	mkdir -p $@

###############################################################
#
# Installers for tools required by the ground and flight builds
#
# NOTE: These are not tied to the default goals
#       and must be invoked manually
#
###############################################################

# Set up QT toolchain
QT_SDK_DIR := $(TOOLS_DIR)/qtsdk-2010.02

.PHONY: qt_sdk_install
qt_sdk_install: QT_SDK_URL  := http://get.qt.nokia.com/qtsdk/qt-sdk-linux-x86-opensource-2010.02.bin
qt_sdk_install: QT_SDK_FILE := $(notdir $(QT_SDK_URL))
# order-only prereq on directory existance:
qt_sdk_install : | $(DL_DIR) $(TOOLS_DIR)
qt_sdk_install: qt_sdk_clean
        # download the source only if it's newer than what we already have
	$(V1) wget -N -P "$(DL_DIR)" "$(QT_SDK_URL)"

        #installer is an executable, make it executable and run it
	$(V1) chmod u+x "$(DL_DIR)/$(QT_SDK_FILE)"
	"$(DL_DIR)/$(QT_SDK_FILE)" --installdir "$(QT_SDK_DIR)"

.PHONY: qt_sdk_clean
qt_sdk_clean:
	$(V1) [ ! -d "$(QT_SDK_DIR)" ] || $(RM) -rf $(QT_SDK_DIR)

# Set up ARM (STM32) SDK
ARM_SDK_DIR := $(TOOLS_DIR)/arm-2011.03

.PHONY: arm_sdk_install
arm_sdk_install: ARM_SDK_URL  := https://sourcery.mentor.com/sgpp/lite/arm/portal/package8736/public/arm-none-eabi/arm-2011.03-42-arm-none-eabi-i686-pc-linux-gnu.tar.bz2
arm_sdk_install: ARM_SDK_FILE := $(notdir $(ARM_SDK_URL))
# order-only prereq on directory existance:
arm_sdk_install: | $(DL_DIR) $(TOOLS_DIR)
arm_sdk_install: arm_sdk_clean
        # download the source only if it's newer than what we already have
	$(V1) wget --no-check-certificate -N -P "$(DL_DIR)" "$(ARM_SDK_URL)"

        # binary only release so just extract it
	$(V1) tar -C $(TOOLS_DIR) -xjf "$(DL_DIR)/$(ARM_SDK_FILE)"

.PHONY: arm_sdk_clean
arm_sdk_clean:
	$(V1) [ ! -d "$(ARM_SDK_DIR)" ] || $(RM) -r $(ARM_SDK_DIR)

# Set up openocd tools
OPENOCD_DIR := $(TOOLS_DIR)/openocd

.PHONY: openocd_install
openocd_install: OPENOCD_URL  := http://sourceforge.net/projects/openocd/files/openocd/0.5.0/openocd-0.5.0.tar.bz2/download
openocd_install: OPENOCD_FILE := openocd-0.5.0.tar.bz2
# order-only prereq on directory existance:
openocd_install: | $(DL_DIR) $(TOOLS_DIR)
openocd_install: openocd_clean
        # download the source only if it's newer than what we already have
	$(V1) wget -N -P "$(DL_DIR)" --trust-server-name "$(OPENOCD_URL)"

        # extract the source
	$(V1) [ ! -d "$(DL_DIR)/openocd-build" ] || $(RM) -r "$(DL_DIR)/openocd-build"
	$(V1) mkdir -p "$(DL_DIR)/openocd-build"
	$(V1) tar -C $(DL_DIR)/openocd-build -xjf "$(DL_DIR)/$(OPENOCD_FILE)"

        # build and install
	$(V1) mkdir -p "$(OPENOCD_DIR)"
	$(V1) ( \
	  cd $(DL_DIR)/openocd-build/openocd-0.5.0 ; \
	  ./configure --prefix="$(OPENOCD_DIR)" --enable-ft2232_libftdi --enable-buspirate; \
	  $(MAKE) ; \
	  $(MAKE) install ; \
	)

        # delete the extracted source when we're done
	$(V1) [ ! -d "$(DL_DIR)/openocd-build" ] || $(RM) -r "$(DL_DIR)/openocd-build"

.PHONY: openocd_clean
openocd_clean:
	$(V1) [ ! -d "$(OPENOCD_DIR)" ] || $(RM) -r "$(OPENOCD_DIR)"

STM32FLASH_DIR := $(TOOLS_DIR)/stm32flash

.PHONY: stm32flash_install
stm32flash_install: STM32FLASH_URL := http://stm32flash.googlecode.com/svn/trunk
stm32flash_install: STM32FLASH_REV := 52
stm32flash_install: stm32flash_clean
        # download the source
	$(V0) @echo " DOWNLOAD     $(STM32FLASH_URL) @ r$(STM32FLASH_REV)"
	$(V1) svn export -q -r "$(STM32FLASH_REV)" "$(STM32FLASH_URL)" "$(STM32FLASH_DIR)"

        # build
	$(V0) @echo " BUILD        $(STM32FLASH_DIR)"
	$(V1) $(MAKE) --silent -C $(STM32FLASH_DIR) all

.PHONY: stm32flash_clean
stm32flash_clean:
	$(V0) @echo " CLEAN        $(STM32FLASH_DIR)"
	$(V1) [ ! -d "$(STM32FLASH_DIR)" ] || $(RM) -r "$(STM32FLASH_DIR)"

##############################
#
# Set up paths to tools
#
##############################

ifeq ($(shell [ -d "$(QT_SDK_DIR)" ] && echo "exists"), exists)
  QMAKE=$(QT_SDK_DIR)/qt/bin/qmake
else
  # not installed, hope it's in the path...
  QMAKE=qmake
endif

ifeq ($(shell [ -d "$(ARM_SDK_DIR)" ] && echo "exists"), exists)
  ARM_SDK_PREFIX := $(ARM_SDK_DIR)/bin/arm-none-eabi-
else
  # not installed, hope it's in the path...
  ARM_SDK_PREFIX ?= arm-none-eabi-
endif

ifeq ($(shell [ -d "$(OPENOCD_DIR)" ] && echo "exists"), exists)
  OPENOCD := $(OPENOCD_DIR)/bin/openocd
else
  # not installed, hope it's in the path...
  OPENOCD ?= openocd
endif

##############################
#
# GCS related components
#
##############################

.PHONY: all_ground
all_ground: openpilotgcs

# Convenience target for the GCS
.PHONY: gcs gcs_clean
gcs: openpilotgcs
gcs_clean: openpilotgcs_clean

.PHONY: openpilotgcs
openpilotgcs:  uavobjects_gcs
	$(V1) mkdir -p $(BUILD_DIR)/ground/$@
	$(V1) ( cd $(BUILD_DIR)/ground/$@ && \
	  $(QMAKE) $(ROOT_DIR)/ground/openpilotgcs/openpilotgcs.pro -spec $(QT_SPEC) -r CONFIG+=$(GCS_BUILD_CONF) && \
	  $(MAKE) -w ; \
	)

.PHONY: openpilotgcs_clean
openpilotgcs_clean:
	$(V0) @echo " CLEAN      $@"
	$(V1) [ ! -d "$(BUILD_DIR)/ground/openpilotgcs" ] || $(RM) -r "$(BUILD_DIR)/ground/openpilotgcs"

.PHONY: uavobjgenerator
uavobjgenerator:
	$(V1) mkdir -p $(BUILD_DIR)/ground/$@
	$(V1) ( cd $(BUILD_DIR)/ground/$@ && \
	  $(QMAKE) $(ROOT_DIR)/ground/uavobjgenerator/uavobjgenerator.pro -spec $(QT_SPEC) -r CONFIG+=debug && \
	  $(MAKE) --no-print-directory -w ; \
	)

UAVOBJ_TARGETS := gcs flight python matlab java
.PHONY:uavobjects
uavobjects:  $(addprefix uavobjects_, $(UAVOBJ_TARGETS))

UAVOBJ_XML_DIR := $(ROOT_DIR)/shared/uavobjectdefinition
UAVOBJ_OUT_DIR := $(BUILD_DIR)/uavobject-synthetics

$(UAVOBJ_OUT_DIR):
	$(V1) mkdir -p $@

uavobjects_%: $(UAVOBJ_OUT_DIR) uavobjgenerator
	$(V1) ( cd $(UAVOBJ_OUT_DIR) && \
	  $(UAVOBJGENERATOR) -$* $(UAVOBJ_XML_DIR) $(ROOT_DIR) ; \
	)

uavobjects_test: $(UAVOBJ_OUT_DIR) uavobjgenerator
	$(V1) $(UAVOBJGENERATOR) -v -none $(UAVOBJ_XML_DIR) $(ROOT_DIR)

uavobjects_clean:
	$(V0) @echo " CLEAN      $@"
	$(V1) [ ! -d "$(UAVOBJ_OUT_DIR)" ] || $(RM) -r "$(UAVOBJ_OUT_DIR)"

##############################
#
# Flight related components
#
##############################

# $(1) = Canonical board name all in lower case (e.g. coptercontrol)
# $(2) = Name of board used in source tree (e.g. CopterControl)
define FW_TEMPLATE
.PHONY: $(1) fw_$(1)
$(1): fw_$(1)_opfw
fw_$(1): fw_$(1)_opfw

fw_$(1)_%: uavobjects_flight
	$(V1) mkdir -p $(BUILD_DIR)/fw_$(1)/dep
	$(V1) cd $(ROOT_DIR)/flight/$(2) && \
		$$(MAKE) -r --no-print-directory \
		BOARD_NAME=$(1) \
		TCHAIN_PREFIX="$(ARM_SDK_PREFIX)" \
		REMOVE_CMD="$(RM)" OOCD_EXE="$(OPENOCD)" \
		$$*

.PHONY: $(1)_clean
$(1)_clean: fw_$(1)_clean
fw_$(1)_clean:
	$(V0) @echo " CLEAN      $$@"
	$(V1) $(RM) -fr $(BUILD_DIR)/fw_$(1)
endef

# $(1) = Canonical board name all in lower case (e.g. coptercontrol)
# $(2) = Name of board used in source tree (e.g. CopterControl)
define BL_TEMPLATE
.PHONY: bl_$(1)
bl_$(1): bl_$(1)_bin
bl_$(1)_bino: bl_$(1)_bin

bl_$(1)_%:
	$(V1) mkdir -p $(BUILD_DIR)/bl_$(1)/dep
	$(V1) cd $(ROOT_DIR)/flight/Bootloaders/$(2) && \
		$$(MAKE) -r --no-print-directory \
		BOARD_NAME=$(1) \
		TCHAIN_PREFIX="$(ARM_SDK_PREFIX)" \
		REMOVE_CMD="$(RM)" OOCD_EXE="$(OPENOCD)" \
		$$*

.PHONY: unbrick_$(1)
unbrick_$(1): bl_$(1)_hex
$(if $(filter-out undefined,$(origin UNBRICK_TTY)),
	$(V0) @echo " UNBRICK    $(1) via $$(UNBRICK_TTY)"
	$(V1) $(STM32FLASH_DIR)/stm32flash \
		-w $(BUILD_DIR)/bl_$(1)/bl_$(1).hex \
		-g 0x0 \
		$$(UNBRICK_TTY)
,
	$(V0) @echo
	$(V0) @echo "ERROR: You must specify UNBRICK_TTY=<serial-device> to use for unbricking."
	$(V0) @echo "       eg. $$(MAKE) $$@ UNBRICK_TTY=/dev/ttyUSB0"
)

.PHONY: bl_$(1)_clean
bl_$(1)_clean:
	$(V0) @echo " CLEAN      $$@"
	$(V1) $(RM) -fr $(BUILD_DIR)/bl_$(1)
endef

# $(1) = Canonical board name all in lower case (e.g. coptercontrol)
define BU_TEMPLATE
.PHONY: bu_$(1)
bu_$(1): bu_$(1)_opfw

bu_$(1)_%: bl_$(1)_bino
	$(V1) mkdir -p $(BUILD_DIR)/bu_$(1)/dep
	$(V1) cd $(ROOT_DIR)/flight/Bootloaders/BootloaderUpdater && \
		$$(MAKE) -r --no-print-directory \
		BOARD_NAME=$(1) \
		TCHAIN_PREFIX="$(ARM_SDK_PREFIX)" \
		REMOVE_CMD="$(RM)" OOCD_EXE="$(OPENOCD)" \
		$$*

.PHONY: bu_$(1)_clean
bu_$(1)_clean:
	$(V0) @echo " CLEAN      $$@"
	$(V1) $(RM) -fr $(BUILD_DIR)/bu_$(1)
endef

# $(1) = Canonical board name all in lower case (e.g. coptercontrol)
define BOARD_PHONY_TEMPLATE
.PHONY: all_$(1)
all_$(1): $$(filter fw_$(1), $$(FW_TARGETS))
all_$(1): $$(filter bl_$(1), $$(BL_TARGETS))
all_$(1): $$(filter bu_$(1), $$(BU_TARGETS))

.PHONY: all_$(1)_clean
all_$(1)_clean: $$(addsuffix _clean, $$(filter fw_$(1), $$(FW_TARGETS)))
all_$(1)_clean: $$(addsuffix _clean, $$(filter bl_$(1), $$(BL_TARGETS)))
all_$(1)_clean: $$(addsuffix _clean, $$(filter bu_$(1), $$(BU_TARGETS)))
endef

ALL_BOARDS := openpilot ahrs coptercontrol pipxtreme ins

# Friendly names of each board (used to find source tree)
openpilot_friendly     := OpenPilot
coptercontrol_friendly := CopterControl
pipxtreme_friendly     := PipXtreme
ins_friendly           := INS
ahrs_friendly          := AHRS

# Start out assuming that we'll build fw, bl and bu for all boards
FW_BOARDS  := $(ALL_BOARDS)
BL_BOARDS  := $(ALL_BOARDS)
BU_BOARDS  := $(ALL_BOARDS)

# FIXME: The INS build doesn't have a bootloader or bootloader
#        updater yet so we need to filter them out to prevent errors.
BL_BOARDS  := $(filter-out ins, $(ALL_BOARDS))
BU_BOARDS  := $(filter-out ins, $(ALL_BOARDS))

# Generate the targets for whatever boards are left in each list
FW_TARGETS := $(addprefix fw_, $(FW_BOARDS))
BL_TARGETS := $(addprefix bl_, $(BL_BOARDS))
BU_TARGETS := $(addprefix bu_, $(BU_BOARDS))

.PHONY: all_fw all_fw_clean
all_fw:        $(addsuffix _opfw,  $(FW_TARGETS))
all_fw_clean:  $(addsuffix _clean, $(FW_TARGETS))

.PHONY: all_bl all_bl_clean
all_bl:        $(addsuffix _bin,   $(BL_TARGETS))
all_bl_clean:  $(addsuffix _clean, $(BL_TARGETS))

.PHONY: all_bu all_bu_clean
all_bu:        $(addsuffix _opfw,  $(BU_TARGETS))
all_bu_clean:  $(addsuffix _clean, $(BU_TARGETS))

.PHONY: all_flight all_flight_clean
all_flight:       all_fw all_bl all_bu
all_flight_clean: all_fw_clean all_bl_clean all_bu_clean

# Expand the groups of targets for each board
$(foreach board, $(ALL_BOARDS), $(eval $(call BOARD_PHONY_TEMPLATE,$(board))))

# Expand the bootloader updater rules
$(foreach board, $(ALL_BOARDS), $(eval $(call BU_TEMPLATE,$(board),$($(board)_friendly))))

# Expand the firmware rules
$(foreach board, $(ALL_BOARDS), $(eval $(call FW_TEMPLATE,$(board),$($(board)_friendly))))

# Expand the bootloader rules
$(foreach board, $(ALL_BOARDS), $(eval $(call BL_TEMPLATE,$(board),$($(board)_friendly))))

.PHONY: sim_posix
sim_posix: sim_posix_elf

sim_posix_%: uavobjects_flight
	$(V1) mkdir -p $(BUILD_DIR)/sitl_posix
	$(V1) $(MAKE) --no-print-directory \
		-C $(ROOT_DIR)/flight/OpenPilot --file=$(ROOT_DIR)/flight/OpenPilot/Makefile.posix $*

.PHONY: sim_win32
sim_win32: sim_win32_exe

sim_win32_%: uavobjects_flight
	$(V1) mkdir -p $(BUILD_DIR)/sitl_win32
	$(V1) $(MAKE) --no-print-directory \
		-C $(ROOT_DIR)/flight/OpenPilot --file=$(ROOT_DIR)/flight/OpenPilot/Makefile.win32 $*

##############################
#
# Packaging components
#
##############################
.PHONY: package
package:
	$(V1) cd $@ && $(MAKE) --no-print-directory $@
