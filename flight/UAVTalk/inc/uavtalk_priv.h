/**
 ******************************************************************************
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotLibraries OpenPilot System Libraries
 * @{
 * @file       uavtalk.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Private include file of the UAVTalk library
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef UAVTALK_PRIV_H
#define UAVTALK_PRIV_H

#include "uavobjectsinit.h"
#include "uavtalk.h"

// Private types and constants
#ifdef PIOS_UAVTALK_DEBUG
#define UAVTALK_MIN_HEADER_LENGTH 14 // sync(1), type (1), size(2), id(4), time(2), object ID(4)
#define UAVTALK_MAX_HEADER_LENGTH 16 // sync(1), type (1), size(2), id(4), time(2), object ID (4), instance ID(2, not used in single objects)
#else
#define UAVTALK_MIN_HEADER_LENGTH 8 // sync(1), type (1), size(2), object ID(4)
#define UAVTALK_MAX_HEADER_LENGTH 10 // sync(1), type (1), size(2), object ID (4), instance ID(2, not used in single objects)
#endif

typedef uint8_t uavtalk_checksum;
#define UAVTALK_CHECKSUM_LENGTH	        sizeof(uavtalk_checksum)
#define UAVTALK_MAX_PAYLOAD_LENGTH      (UAVOBJECTS_LARGEST + 1)
#define UAVTALK_MIN_PACKET_LENGTH	UAVTALK_MIN_HEADER_LENGTH + UAVTALK_CHECKSUM_LENGTH
#define UAVTALK_MAX_PACKET_LENGTH       UAVTALK_MAX_HEADER_LENGTH + UAVTALK_CHECKSUM_LENGTH + UAVTALK_MAX_PAYLOAD_LENGTH

typedef struct {
    UAVObjHandle obj;
    uint8_t type;
    uint16_t packet_size;
#ifdef PIOS_UAVTALK_DEBUG
    uint32_t msgId;
    uint16_t time;
#endif
    uint32_t objId;
    uint16_t instId;
    uint32_t length;
    uint8_t instanceLength;
    uint8_t cs;
    int32_t rxCount;
    UAVTalkRxState state;
    uint16_t rxPacketLength;
} UAVTalkInputProcessor;

typedef struct {
    uint8_t canari;
    UAVTalkOutputStream outStream;
    xSemaphoreHandle lock;
    xSemaphoreHandle transLock;
    xSemaphoreHandle respSema;
    UAVObjHandle respObj;
    uint16_t respInstId;
#ifdef PIOS_UAVTALK_DEBUG
    uint32_t curMsgId;
    char *printBuffer;
#endif
    UAVTalkStats stats;
    UAVTalkInputProcessor iproc;
    uint8_t numRxBuffers;
    uint8_t **rxBuffers;
    uint8_t curRxBuffer;
    uint32_t txSize;
    uint8_t *txBuffer;
} UAVTalkConnectionData;

#define UAVTALK_CANARI         0xCA
#define UAVTALK_WAITFOREVER     -1
#define UAVTALK_NOWAIT          0
#define UAVTALK_SYNC_VAL       0x3C

//macros
#define CHECKCONHANDLE(handle,variable,failcommand) \
	variable = (UAVTalkConnectionData*) handle; \
	if (variable == NULL || variable->canari != UAVTALK_CANARI) { \
		failcommand; \
	}

#endif // UAVTALK__PRIV_H
/**
 * @}
 * @}
 */
