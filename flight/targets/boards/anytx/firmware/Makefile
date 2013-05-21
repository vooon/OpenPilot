#
# Copyright (c) 2009-2013, The OpenPilot Team, http://www.openpilot.org
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
#

ifndef OPENPILOT_IS_COOL
    $(error Top level Makefile must be used to build this target)
endif

include ../board-info.mk
include $(ROOT_DIR)/make/firmware-defs.mk

# ARM DSP library
override USE_DSP_LIB := NO

# List of mandatory modules to include
MODULES += AnyTXRadio AnyTXControl FirmwareIAP Telemetry

# List of optional modules to include
OPTMODULES =

# List C source files here (C dependencies are automatically generated).
# Use file-extension c for "c-only"-files
ifndef TESTAPP
	## The standard CMSIS startup
	SRC += $(CMSIS_DEVICEDIR)/system_stm32f10x.c

    ## Application Core
    SRC += ../pios_usb_board_data.c
    SRC += $(OPMODULEDIR)/System/systemmod.c
	SRC += $(OPSYSTEM)/anytx.c
	SRC += $(OPSYSTEM)/pios_board.c
	SRC += $(FLIGHTLIB)/alarms.c
	SRC += $(OPUAVTALK)/uavtalk.c
	SRC += $(OPUAVOBJ)/uavobjectmanager.c
	SRC += $(OPUAVOBJ)/eventdispatcher.c
    SRC += $(OPUAVOBJ)/callbackscheduler.c
	SRC += $(OPSYSTEM)/protocol/devo.c
	SRC += $(OPSYSTEM)/protocol/dsm2.c
	SRC += $(OPSYSTEM)/protocol/wk2x01.c
	SRC += $(OPSYSTEM)/protocol/j6pro.c

    ## UAVObjects
	SRC += $(OPUAVSYNTHDIR)/objectpersistence.c
	SRC += $(OPUAVSYNTHDIR)/anytxcontrolsettings.c
	SRC += $(OPUAVSYNTHDIR)/stabilizationdesired.c
	SRC += $(OPUAVSYNTHDIR)/accessorydesired.c
	SRC += $(OPUAVSYNTHDIR)/gcstelemetrystats.c
	SRC += $(OPUAVSYNTHDIR)/flighttelemetrystats.c
	SRC += $(OPUAVSYNTHDIR)/flightstatus.c
	SRC += $(OPUAVSYNTHDIR)/systemstats.c
	SRC += $(OPUAVSYNTHDIR)/systemalarms.c
	SRC += $(OPUAVSYNTHDIR)/systemsettings.c
	SRC += $(OPUAVSYNTHDIR)/manualcontrolcommand.c
	SRC += $(OPUAVSYNTHDIR)/i2cstats.c
	SRC += $(OPUAVSYNTHDIR)/watchdogstatus.c
	SRC += $(OPUAVSYNTHDIR)/firmwareiapobj.c
	SRC += $(OPUAVSYNTHDIR)/hwsettings.c
	SRC += $(OPUAVSYNTHDIR)/receiveractivity.c
	SRC += $(OPUAVSYNTHDIR)/taskinfo.c
	SRC += $(OPUAVSYNTHDIR)/gcsreceiver.c
	SRC += $(OPUAVSYNTHDIR)/flightbatterystate.c
	EXTRAINCDIRS += $(OPSYSTEM)
else
    ## Test Code
    SRC += $(OPTESTS)/test_common.c
    SRC += $(OPTESTS)/$(TESTAPP).c
endif

CDEFS += -DEE_BANK_BASE=$(EE_BANK_BASE)
CDEFS += -DEE_BANK_SIZE=$(EE_BANK_SIZE)

# Optional component libraries
include $(ROOT_DIR)/make/apps-defs.mk
include $(ROOT_DIR)/make/common-defs.mk
