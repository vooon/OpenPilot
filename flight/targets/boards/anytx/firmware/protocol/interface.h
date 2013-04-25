 /**
 ******************************************************************************
 * @addtogroup Radio Protocol hardware abstraction layer
 * @{
 * @addtogroup PIOS_CYRF6936 CYRF6936 Radio Protocol Functions
 * @brief CYRF6936 Radio Protocol functionality
 * @{
 *
 * @file       interface.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @brief      CYRF6936 Radio Protocol Functions
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

#ifndef _INTERFACE_H_
#define _INTERFACE_H_
#include <pios.h>

#define ORDER_EATRG { INP_ELEVATOR, INP_AILERON, INP_THROTTLE, INP_RUDDER, INP_GEAR }
#define ORDER_TAERG { INP_THROTTLE, INP_AILERON, INP_ELEVATOR, INP_RUDDER, INP_GEAR }

#ifdef PROTO_HAS_A7105
#include "iface_a7105.h"
void FLYSKY_Initialize();
#endif

#ifdef PROTO_HAS_CYRF6936
void DEVO_Initialize();
extern u16 devo_cb();
extern u16 devo_telemetry_cb();

void WK2x01_Initialize();
extern u16 wk_cb();

void DSM2_Initialize();
extern u16 dsm2_cb();

void J6PRO_Initialize();
extern u16 j6pro_cb();

#endif

#define NUM_CHANNELS 12
#define CHAN_MULTIPLIER 100
#define PCT_TO_RANGE(x) ((s16)(x) * CHAN_MULTIPLIER)
#define RANGE_TO_PCT(x) ((s16)(x) / CHAN_MULTIPLIER)
#define CHAN_MAX_VALUE (100 * CHAN_MULTIPLIER)
#define CHAN_MIN_VALUE (-100 * CHAN_MULTIPLIER)
extern s16 Channels[NUM_CHANNELS];

/* Protocol */
enum Protocols {
    PROTOCOL_NONE,
#ifdef PROTO_HAS_CYRF6936
    PROTOCOL_DEVO,
    PROTOCOL_WK2801,
    PROTOCOL_WK2601,
    PROTOCOL_WK2401,
    PROTOCOL_DSM2,
    PROTOCOL_DSMX,
    PROTOCOL_J6PRO,
#endif
#ifdef PROTO_HAS_A7105
    PROTOCOL_FLYSKY,
#endif
    PROTOCOL_COUNT,
};

enum ModelType {
    MODELTYPE_HELI,
    MODELTYPE_PLANE,
};

enum TxPower {
    TXPOWER_100uW,
    TXPOWER_300uW,
    TXPOWER_1mW,
    TXPOWER_3mW,
    TXPOWER_10mW,
    TXPOWER_30mW,
    TXPOWER_100mW,
    TXPOWER_150mW, //+4dBm
    TXPOWER_LAST,
};

// Private types
struct Model {
    char name[24];
    char icon[20];
    enum ModelType type;
    enum Protocols protocol;
    u8 num_channels;
    u32 fixed_id;
    enum TxPower tx_power;
    u8 template[NUM_CHANNELS];
};
extern struct Model Model;

#define NUM_TELEM 9
#define TELEM_ERROR_TIME 5000
#define TELEM_NUM_ALARMS 6

enum {
    TELEM_VOLT1 = 1,
    TELEM_VOLT2,
    TELEM_VOLT3,
    TELEM_TEMP1,
    TELEM_TEMP2,
    TELEM_TEMP3,
    TELEM_TEMP4,
    TELEM_RPM1,
    TELEM_RPM2,
    TELEM_GPS_LAT,
    TELEM_GPS_LONG,
    TELEM_GPS_ALT,
    TELEM_GPS_SPEED,
    TELEM_GPS_TIME,
};
enum {
    TELEMFLAG_ALARM1 = 0x00,
    TELEMFLAG_ALARM2 = 0x02,
    TELEMFLAG_ALARM3 = 0x04,
    TELEMFLAG_ALARM4 = 0x08,
    TELEMFLAG_ALARM5 = 0x10,
    TELEMFLAG_ALARM6 = 0x20,
};

enum {
    TELEMUNIT_FEET   = 0x40,
    TELEMUNIT_FAREN  = 0x80,
};
struct gps {
    s32 latitude;
    s32 longitude;
    s32 altitude;
    s32 velocity;
    u32 time;
};

struct Telemetry {
    u16 volt[3];
    u8 temp[4];
    u16 rpm[2];
    struct gps gps;
    u32 time[3];
};
extern struct Telemetry Telemetry;
#endif //_INTERFACE_H_
