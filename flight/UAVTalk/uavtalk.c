/* -*- Mode: c; c-basic-offset: 2; tab-width: 2; indent-tabs-mode: t -*- */
/**
 ******************************************************************************
 * @addtogroup OpenPilotSystem OpenPilot System
 * @{
 * @addtogroup OpenPilotLibraries OpenPilot System Libraries
 * @{
 *
 * @file       uavtalk.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      UAVTalk library, implements to telemetry protocol. See the wiki for more details.
 * 	       This library should not be called directly by the application, it is only used by the
 * 	       Telemetry module.
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

#include "openpilot.h"
#include "uavtalk_priv.h"
#include "gcstelemetrystats.h"
#ifdef USE_Debug
#include "debugmod.h"
#else
#define DEBUG_QUEUE_BUF_SIZE 64
#endif

#ifdef PIOS_UAVTALK_DEBUG
#define UAVTALK_MSG_ID_INC 8
#endif

// Private functions
static int32_t objectTransaction(UAVTalkConnectionData *connection, UAVObjHandle objectId, uint16_t instId, uint8_t type, int32_t timeout);
static int32_t sendObject(UAVTalkConnectionData *connection, UAVObjHandle obj, uint16_t instId, uint8_t type, uint32_t msgId);
static int32_t sendSingleObject(UAVTalkConnectionData *connection, UAVObjHandle obj, uint16_t instId, uint8_t type, uint32_t msgId);
static int32_t sendNack(UAVTalkConnectionData *connection, uint32_t objId, uint32_t msgId);
static int32_t receiveObject(UAVTalkConnectionData *connection, uint8_t type, uint32_t objId, uint16_t instId, uint8_t* data, int32_t length);
static void updateAck(UAVTalkConnectionData *connection, UAVObjHandle obj, uint16_t instId);
static uint16_t packetLength(const uint8_t *buf);
static void packetCRC(uint8_t *buf);
static int32_t packHeader(uint8_t *buf, uint32_t buf_length, uint8_t type, uint32_t objId, uint32_t msgId, uint16_t instId, uint8_t singleInstance, uint16_t length);
static void sendPacket(UAVTalkConnectionData *connection, uint8_t* buf, uint16_t length, uint16_t packet_length);
#ifdef PIOS_UAVTALK_DEBUG
static uint8_t packetType(const uint8_t *buf);
static uint32_t packetMsgId(const uint8_t *buf);
static uint16_t packetTime(const uint8_t *buf);
static uint32_t packetObjId(const uint8_t *buf);
static void printDebug(UAVTalkConnectionData *connection, char debugType, uint32_t msgId, uint16_t time, uint8_t type, uint32_t objId);
static void printConnDebug(UAVTalkConnectionData *connection, char debugType);
static void printPacketDebug(UAVTalkConnectionData *connection, char debugType, const uint8_t *buf);
#else
#define printDebug(debugType, msgId, time, type, objId) {}
#define printConnDebug(c, t) {}
#define printPacketDebug(c, d, b) {}
#endif

/**
 * Initialize the UAVTalk library
 * This version of the initialization function support allocating multiple receive buffers.
 * \param[in] outputStream Function pointer that is called to send a data buffer
 * \param[in] numRxBuffers The number of receive buffers to allocate
 * \return the UAVTalkConnection
 * \return 0 Failure
 */
UAVTalkConnection UAVTalkInitializeMultiBuffer(UAVTalkOutputStream outputStream, uint8_t numRxBuffers, uint8_t connectionId)
{
	UAVTalkConnectionData *connection;
	uint8_t i;
	// allocate object
	connection = pvPortMalloc(sizeof(UAVTalkConnectionData));
	if (!connection) return 0;
	connection->canari = UAVTALK_CANARI;
	connection->iproc.rxPacketLength = 0;
	connection->iproc.state = UAVTALK_STATE_SYNC;
	connection->outStream = outputStream;
	connection->lock = xSemaphoreCreateRecursiveMutex();
	connection->transLock = xSemaphoreCreateRecursiveMutex();
	connection->numRxBuffers = numRxBuffers;
	connection->curRxBuffer = 0;
	connection->txSize = UAVTALK_MAX_PACKET_LENGTH;
#ifdef PIOS_UAVTALK_DEBUG
	connection->curMsgId = connectionId;
	connection->printBuffer = (char*)pvPortMalloc(DEBUG_QUEUE_BUF_SIZE);
#endif
	// allocate buffers
	connection->rxBuffers = (UAVTalkBuffer*)pvPortMalloc(numRxBuffers * sizeof(UAVTalkBuffer));
	if (!connection->rxBuffers) return 0;
	for (i = 0; i < numRxBuffers; ++i)
	{
		if (!(connection->rxBuffers[i].buffer = pvPortMalloc(UAVTALK_MAX_PACKET_LENGTH)))
			return 0;
		connection->rxBuffers[i].lock = xSemaphoreCreateRecursiveMutex();
	}
	if (!(connection->txBuffer = pvPortMalloc(UAVTALK_MAX_PACKET_LENGTH)))
		return 0;
	vSemaphoreCreateBinary(connection->respSema);
	xSemaphoreTake(connection->respSema, 0); // reset to zero
	UAVTalkResetStats( (UAVTalkConnection) connection );
	return (UAVTalkConnection) connection;
}

/**
 * Initialize the UAVTalk library
 * \param[in] outputStream Function pointer that is called to send a data buffer
 * \return the UAVTalkConnection
 * \return 0 Failure
 */
UAVTalkConnection UAVTalkInitialize(UAVTalkOutputStream outputStream)
{
	return UAVTalkInitializeMultiBuffer(outputStream, 1, 1);
}

/**
 * Set the communication output stream
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] outputStream Function pointer that is called to send a data buffer
 * \return 0 Success
 * \return -1 Failure
 */
int32_t UAVTalkSetOutputStream(UAVTalkConnection connectionHandle, UAVTalkOutputStream outputStream)
{

	UAVTalkConnectionData *connection;
    CHECKCONHANDLE(connectionHandle,connection,return -1);

	// Lock
	xSemaphoreTakeRecursive(connection->lock, portMAX_DELAY);
	
	// set output stream
	connection->outStream = outputStream;
	
	// Release lock
	xSemaphoreGiveRecursive(connection->lock);

	return 0;

}

/**
 * Get current output stream
 * \param[in] connection UAVTalkConnection to be used
 * @return UAVTarlkOutputStream the output stream used
 */
UAVTalkOutputStream UAVTalkGetOutputStream(UAVTalkConnection connectionHandle)
{
	UAVTalkConnectionData *connection;
    CHECKCONHANDLE(connectionHandle,connection,return NULL);
	return connection->outStream;
}

/**
 * Get communication statistics counters
 * \param[in] connection UAVTalkConnection to be used
 * @param[out] statsOut Statistics counters
 */
void UAVTalkGetStats(UAVTalkConnection connectionHandle, UAVTalkStats* statsOut)
{
	UAVTalkConnectionData *connection;
    CHECKCONHANDLE(connectionHandle,connection,return );

	// Lock
	xSemaphoreTakeRecursive(connection->lock, portMAX_DELAY);
	
	// Copy stats
	memcpy(statsOut, &connection->stats, sizeof(UAVTalkStats));
	
	// Release lock
	xSemaphoreGiveRecursive(connection->lock);
}

/**
 * Reset the statistics counters.
 * \param[in] connection UAVTalkConnection to be used
 */
void UAVTalkResetStats(UAVTalkConnection connectionHandle)
{
	UAVTalkConnectionData *connection;
    CHECKCONHANDLE(connectionHandle,connection,return);

	// Lock
	xSemaphoreTakeRecursive(connection->lock, portMAX_DELAY);
	
	// Clear stats
	memset(&connection->stats, 0, sizeof(UAVTalkStats));
	
	// Release lock
	xSemaphoreGiveRecursive(connection->lock);
}

/**
 * Get the type of the current packet.
 * \param[in] connection UAVTalkConnection to be used
 * @param[out] packet type
 */
uint32_t UAVTalkGetPacketType(UAVTalkConnection connectionHandle)
{
	UAVTalkConnectionData *connection;
    CHECKCONHANDLE(connectionHandle,connection,return 0);
	uint32_t type = 0;

	// Lock
	xSemaphoreTakeRecursive(connection->lock, portMAX_DELAY);

	// Only return the object type if it is valid.
	if (connection->iproc.state > UAVTALK_STATE_TYPE)
	{
		// Get the packet type
		type = connection->iproc.type;
	}
	
	// Release lock
	xSemaphoreGiveRecursive(connection->lock);

	// Return the packet type
	return type;
}

/**
 * Get the object ID out of the current packet.
 * \param[in] connection UAVTalkConnection to be used
 * @param[out] objectId The object ID
 */
uint32_t UAVTalkGetObjectID(UAVTalkConnection connectionHandle)
{
	UAVTalkConnectionData *connection;
    CHECKCONHANDLE(connectionHandle,connection,return 0);
	uint32_t obj_id = 0;

	// Lock
	xSemaphoreTakeRecursive(connection->lock, portMAX_DELAY);

	// Only return the object ID if it is valid.
	if (connection->iproc.state > UAVTALK_STATE_OBJID)
	{
		// Get the object ID
		obj_id = connection->iproc.objId;
	}
	
	// Release lock
	xSemaphoreGiveRecursive(connection->lock);

	// Return the object ID
	return obj_id;
}

/**
 * Request an update for the specified object, on success the object data would have been
 * updated by the GCS.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] obj Object to update
 * \param[in] instId The instance ID or UAVOBJ_ALL_INSTANCES for all instances.
 * \param[in] timeout Time to wait for the response, when zero it will return immediately
 * \return 0 Success
 * \return -1 Failure
 */
int32_t UAVTalkSendObjectRequest(UAVTalkConnection connectionHandle, UAVObjHandle obj, uint16_t instId, int32_t timeout)
{
	UAVTalkConnectionData *connection;
    CHECKCONHANDLE(connectionHandle,connection,return -1);
	return objectTransaction(connection, obj, instId, UAVTALK_TYPE_OBJ_REQ, timeout);
}

/**
 * Send the specified object through the telemetry link.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] obj Object to send
 * \param[in] instId The instance ID or UAVOBJ_ALL_INSTANCES for all instances.
 * \param[in] acked Selects if an ack is required (1:ack required, 0: ack not required)
 * \param[in] timeoutMs Time to wait for the ack, when zero it will return immediately
 * \return 0 Success
 * \return -1 Failure
 */
int32_t UAVTalkSendObject(UAVTalkConnection connectionHandle, UAVObjHandle obj, uint16_t instId, uint8_t acked, int32_t timeoutMs)
{
	UAVTalkConnectionData *connection;
    CHECKCONHANDLE(connectionHandle,connection,return -1);
	// Send object
	if (acked == 1)
	{
		return objectTransaction(connection, obj, instId, UAVTALK_TYPE_OBJ_ACK, timeoutMs);
	}
	else
	{
		return objectTransaction(connection, obj, instId, UAVTALK_TYPE_OBJ, timeoutMs);
	}
}

/**
 * Send the packet through the telemetry link.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] packet The packet
 * \return 0 Success
 * \return -1 Failure
 */
int32_t UAVTalkSendPacket(UAVTalkConnection connectionHandle, UAVTalkBufferHandle buffer)
{
	UAVTalkConnectionData *connection;
    CHECKCONHANDLE(connectionHandle,connection,return -1);
	UAVTalkBuffer *buf = (UAVTalkBuffer*)buffer;
  uint8_t *packet = buf->buffer;
	uint32_t packet_length = packetLength(packet);
	uint32_t length = packet_length - UAVTALK_MIN_HEADER_LENGTH;
	// Send the packet
	xSemaphoreTakeRecursive(connection->lock, portMAX_DELAY);
	sendPacket(connection, packet, length, packet_length);
	xSemaphoreGiveRecursive(connection->lock);
	return 0;
}

/**
 * Execute the requested transaction on an object.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] obj Object
 * \param[in] instId The instance ID of UAVOBJ_ALL_INSTANCES for all instances.
 * \param[in] type Transaction type
 * 			  UAVTALK_TYPE_OBJ: send object,
 * 			  UAVTALK_TYPE_OBJ_REQ: request object update
 * 			  UAVTALK_TYPE_OBJ_ACK: send object with an ack
 * \return 0 Success
 * \return -1 Failure
 */
static int32_t objectTransaction(UAVTalkConnectionData *connection, UAVObjHandle obj, uint16_t instId, uint8_t type, int32_t timeoutMs)
{
	int32_t respReceived;
	uint32_t msgId = 0;
	
	// Send object depending on if a response is needed
	if (type == UAVTALK_TYPE_OBJ_ACK || type == UAVTALK_TYPE_OBJ_REQ)
	{
		// Get transaction lock (will block if a transaction is pending)
		xSemaphoreTakeRecursive(connection->transLock, portMAX_DELAY);
		// Send object
		xSemaphoreTakeRecursive(connection->lock, portMAX_DELAY);
		connection->respObj = obj;
		connection->respInstId = instId;
#ifdef PIOS_UAVTALK_DEBUG
		msgId = (connection->curMsgId += UAVTALK_MSG_ID_INC);
#endif
		sendObject(connection, obj, instId, type, msgId);
		xSemaphoreGiveRecursive(connection->lock);
		// Wait for response (or timeout)
		respReceived = xSemaphoreTake(connection->respSema, timeoutMs/portTICK_RATE_MS);
		// Check if a response was received
		if (respReceived == pdFALSE)
		{
			// Cancel transaction
			xSemaphoreTakeRecursive(connection->lock, portMAX_DELAY);
			xSemaphoreTake(connection->respSema, 0); // non blocking call to make sure the value is reset to zero (binary sema)
			connection->respObj = 0;
			xSemaphoreGiveRecursive(connection->lock);
			xSemaphoreGiveRecursive(connection->transLock);
			return -1;
		}
		else
		{
			xSemaphoreGiveRecursive(connection->transLock);
			return 0;
		}
	}
	else if (type == UAVTALK_TYPE_OBJ)
	{
		xSemaphoreTakeRecursive(connection->lock, portMAX_DELAY);
#ifdef PIOS_UAVTALK_DEBUG
		msgId = (connection->curMsgId += UAVTALK_MSG_ID_INC);
#endif
		sendObject(connection, obj, instId, UAVTALK_TYPE_OBJ, msgId);
		xSemaphoreGiveRecursive(connection->lock);
		return 0;
	}
	else
	{
		return -1;
	}
}

/**
 * Process an byte from the telemetry stream.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] rxbyte Received byte
 * \return 0 Success
 * \return -1 Failure
 */
UAVTalkRxState UAVTalkProcessInputStream(UAVTalkConnection connectionHandle, uint8_t rxbyte)
{
	UAVTalkConnectionData *connection;
    CHECKCONHANDLE(connectionHandle,connection,return -1);
	UAVTalkInputProcessor *iproc = &connection->iproc;
	UAVTalkBuffer *rxBuffer = connection->rxBuffers + connection->curRxBuffer;

	// Increment the rxBytes count
	++connection->stats.rxBytes;
	
	if (iproc->rxPacketLength < 0xffff)
		iproc->rxPacketLength++;   // update packet byte count

	// Reset the state machine if there was an error or the last message was complete
	if(iproc->state == UAVTALK_STATE_ERROR || iproc->state == UAVTALK_STATE_COMPLETE)
	{

		// Reuse the current buffer on error.
		if(iproc->state != UAVTALK_STATE_ERROR)
		{
			// Release the buffer.
			xSemaphoreGiveRecursive(rxBuffer->lock);
			// Decrement to the previous buffer.
			if(connection->curRxBuffer == 0)
				connection->curRxBuffer = connection->numRxBuffers - 1;
			else
				--connection->curRxBuffer;
		}

		iproc->state = UAVTALK_STATE_SYNC;
	}

	// Insert this byte into the rx buffer if we're in the middle of a packet.
	else if(iproc->state != UAVTALK_STATE_SYNC) {
		rxBuffer->buffer[iproc->rxCount++] = rxbyte;

		// update the CRC
		if(iproc->state != UAVTALK_STATE_CS)
			iproc->cs = PIOS_CRC_updateByte(iproc->cs, rxbyte);
	}

	// Receive state machine
	switch (iproc->state)
	{
		case UAVTALK_STATE_SYNC:
			if (rxbyte != UAVTALK_SYNC_VAL)
			{
				iproc->rxCount = 0;
				break;
			}

			// Switch to the next RX buffer.
			{
				uint8_t nextRxBuffer = connection->curRxBuffer + 1;
				if (nextRxBuffer >= connection->numRxBuffers)
					nextRxBuffer = 0;
				// Switch to the new buffer.
				connection->curRxBuffer = nextRxBuffer;
				rxBuffer = connection->rxBuffers + nextRxBuffer;
				// Lock the buffer.
				xSemaphoreTakeRecursive(rxBuffer->lock, portMAX_DELAY);
			}
			
			// Initialize and update the CRC
			iproc->cs = PIOS_CRC_updateByte(0, rxbyte);

			// Add this byte to the RX buffer
			rxBuffer->buffer[0] = rxbyte;
			iproc->rxCount = 1;
			iproc->rxPacketLength = 1;

			// Change the state to TYPE
			iproc->state = UAVTALK_STATE_TYPE;

			break;
			
		case UAVTALK_STATE_TYPE:
			
			if ((rxbyte & UAVTALK_TYPE_MASK) != UAVTALK_TYPE_VER)
			{
				iproc->state = UAVTALK_STATE_ERROR;
				printConnDebug(connection, 'T');
				break;
			}
			
			iproc->type = rxbyte;
			
			iproc->packet_size = 0;
			
			iproc->state = UAVTALK_STATE_SIZE;
			break;
			
		case UAVTALK_STATE_SIZE:
			
			if (iproc->rxCount == 3)
			{
				iproc->packet_size += rxbyte;
				break;
			}

			iproc->packet_size += rxbyte << 8;
			
			if (iproc->packet_size < UAVTALK_MIN_HEADER_LENGTH || iproc->packet_size > UAVTALK_MAX_HEADER_LENGTH + UAVTALK_MAX_PAYLOAD_LENGTH)
			{   // incorrect packet size
				iproc->state = UAVTALK_STATE_ERROR;
				printConnDebug(connection, 'L');
				break;
			}
			
#ifdef PIOS_UAVTALK_DEBUG
			iproc->msgId = 0;
			iproc->state = UAVTALK_STATE_MSGID;
#else
			iproc->objId = 0;
			iproc->state = UAVTALK_STATE_OBJID;
#endif
			break;

#ifdef PIOS_UAVTALK_DEBUG
			
		case UAVTALK_STATE_MSGID:
			
			iproc->msgId += rxbyte << (8*(iproc->rxCount - 5));

			if (iproc->rxCount < 8)
				break;

			iproc->state = UAVTALK_STATE_TIME;
			break;

		case UAVTALK_STATE_TIME:
			
			if (iproc->rxCount == 8)
			{
				iproc->time = rxbyte;
				break;
			}

			iproc->time += rxbyte << 8;

			iproc->state = UAVTALK_STATE_OBJID;
			iproc->objId = 0;
			break;

#endif

		case UAVTALK_STATE_OBJID:
			
#ifdef PIOS_UAVTALK_DEBUG
			iproc->objId += rxbyte << (8*(iproc->rxCount - 11));
			if (iproc->rxCount < 14)
				break;
#else
			iproc->objId += rxbyte << (8*(iproc->rxCount - 5));
			if (iproc->rxCount < 8)
				break;
#endif

			// Search for object
			iproc->obj = UAVObjGetByID(iproc->objId);
			
			// Determine data length
			if (iproc->type == UAVTALK_TYPE_OBJ_REQ || iproc->type == UAVTALK_TYPE_ACK || iproc->type == UAVTALK_TYPE_NACK)
			{
				iproc->length = 0;
				iproc->instanceLength = 0;
			}
			else if(iproc->obj != NULL)
			{
				iproc->length = UAVObjGetNumBytes(iproc->obj);
				iproc->instanceLength = (UAVObjIsSingleInstance(iproc->obj) ? 0 : 2);
			}
			else
			{
				iproc->length = iproc->packet_size - iproc->rxPacketLength;
				iproc->instanceLength = 0;
			}

			// Check length and determine next state
			if (iproc->length >= UAVTALK_MAX_PAYLOAD_LENGTH)
			{
				connection->stats.rxErrors++;
				iproc->state = UAVTALK_STATE_ERROR;
				printConnDebug(connection, 'P');
				break;
			}

			// Check the lengths match
			if ((iproc->rxPacketLength + iproc->instanceLength + iproc->length) != iproc->packet_size)
			{   // packet error - mismatched packet size
				connection->stats.rxErrors++;
				iproc->state = UAVTALK_STATE_ERROR;
				printConnDebug(connection, 'M');
				break;
			}

			iproc->instId = 0;
			if (iproc->type == UAVTALK_TYPE_NACK)
			{
				// If this is a NACK, we skip to Checksum
				iproc->state = UAVTALK_STATE_CS;
			}

			// Check if this is a single instance object (i.e. if the instance ID field is coming next)
			else if (iproc->instanceLength == 0)
			{
				// If there is a payload get it, otherwise receive checksum
				if (iproc->length > 0)
					iproc->state = UAVTALK_STATE_DATA;
				else
					iproc->state = UAVTALK_STATE_CS;
			}
			else
			{
				iproc->state = UAVTALK_STATE_INSTID;
			}

			break;

		case UAVTALK_STATE_INSTID:
			
#ifdef PIOS_UAVTALK_DEBUG
			iproc->instId += rxbyte << (8*(iproc->rxCount - 14));
			if (iproc->rxCount < 17)
				break;
#else
			iproc->instId += rxbyte << (8*(iproc->rxCount - 9));
			if (iproc->rxCount < 11)
				break;
#endif
			
			// If there is a payload get it, otherwise receive checksum
			if (iproc->length > 0)
				iproc->state = UAVTALK_STATE_DATA;
			else
				iproc->state = UAVTALK_STATE_CS;
			break;
			
		case UAVTALK_STATE_DATA:
			
			if (iproc->rxCount < (iproc->packet_size + UAVTALK_CHECKSUM_LENGTH - 1))
				break;
			
			iproc->state = UAVTALK_STATE_CS;
			break;
			
		case UAVTALK_STATE_CS:
			
			// the CRC byte
			if (rxbyte != iproc->cs)
			{   // packet error - faulty CRC
				connection->stats.rxErrors++;
				iproc->state = UAVTALK_STATE_ERROR;
				printConnDebug(connection, 'C');
				break;
			}
			
			if (iproc->rxPacketLength != (iproc->packet_size + 1))
			{   // packet error - mismatched packet size
				connection->stats.rxErrors++;
				iproc->state = UAVTALK_STATE_ERROR;
				printConnDebug(connection, 'S');
				break;
			}

			iproc->state = UAVTALK_STATE_COMPLETE;
			iproc->rxCount = 0;
			break;
			
		default:
			connection->stats.rxErrors++;
			iproc->state = UAVTALK_STATE_ERROR;
	}

	if (iproc->state == UAVTALK_STATE_COMPLETE)
	{
		printConnDebug(connection, 'R');
		if (iproc->obj != NULL || ((iproc->type == UAVTALK_TYPE_NACK || iproc->type == UAVTALK_TYPE_OBJ_REQ) && (connection->numRxBuffers == 1)))
		{
			xSemaphoreTakeRecursive(connection->lock, portMAX_DELAY);
			receiveObject(connection, iproc->type, iproc->objId, iproc->instId, rxBuffer->buffer + iproc->instanceLength + UAVTALK_MIN_HEADER_LENGTH, iproc->length);
			xSemaphoreGiveRecursive(connection->lock);
		}
		connection->stats.rxObjectBytes += iproc->length;
		connection->stats.rxObjects++;
		// Release the buffer.
		xSemaphoreGiveRecursive(rxBuffer->lock);
	}
	else if(iproc->state == UAVTALK_STATE_ERROR)
		iproc->rxCount = 0;

	// Done
	return iproc->state;
}

/**
 * Returns a pointer to the buffer containing the last packet received on a UAVTalk connection.
 * The sending connection must be in the COMPLETE state.
 *
 * \param[in] connection UAVTalkConnection to extract the packet from
 * \return A pointer to the buffer
 */
UAVTalkBufferHandle UAVTalkGetCurrentBuffer(UAVTalkConnection connection)
{
	UAVTalkConnectionData *con;
    CHECKCONHANDLE(connection,con,return 0);

	// Return the current packet
	return &(con->rxBuffers[con->curRxBuffer]);
}

/**
 * Locks the given buffer.
 *
 * \param[in] buffer The buffer to lock.
 */
void UAVTalkLockBuffer(UAVTalkBufferHandle buffer)
{
	UAVTalkBuffer *buf = (UAVTalkBuffer*)buffer;
	xSemaphoreTakeRecursive(buf->lock, portMAX_DELAY);
}

/**
 * Releases the lock on the given buffer.
 *
 * \param[in] buffer The buffer to lock.
 */
void UAVTalkReleaseBuffer(UAVTalkBufferHandle buffer)
{
	UAVTalkBuffer *buf = (UAVTalkBuffer*)buffer;
	xSemaphoreGiveRecursive(buf->lock);
}

/**
 * Receive an object. This function process objects received through the telemetry stream.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] type Type of received message (UAVTALK_TYPE_OBJ, UAVTALK_TYPE_OBJ_REQ, UAVTALK_TYPE_OBJ_ACK, UAVTALK_TYPE_ACK, UAVTALK_TYPE_NACK)
 * \param[in] objId ID of the object to work on
 * \param[in] instId The instance ID of UAVOBJ_ALL_INSTANCES for all instances.
 * \param[in] data Data buffer
 * \param[in] length Buffer length
 * \return 0 Success
 * \return -1 Failure
 */
static int32_t receiveObject(UAVTalkConnectionData *connection, uint8_t type, uint32_t objId, uint16_t instId, uint8_t* data, int32_t length)
{
	UAVObjHandle obj;
	int32_t ret = 0;

	// Get the handle to the Object. Will be zero
	// if object does not exist.
	obj = UAVObjGetByID(objId);
	
	// Process message type
	switch (type)
	{
		case UAVTALK_TYPE_OBJ:
			// All instances, not allowed for OBJ messages
			if (instId != UAVOBJ_ALL_INSTANCES)
			{
				// Unpack object, if the instance does not exist it will be created!
				UAVObjUnpack(obj, instId, data);
				// Check if an ack is pending
				updateAck(connection, obj, instId);
			}
			else
			{
				ret = -1;
			}
			break;
		case UAVTALK_TYPE_OBJ_ACK:
			// All instances, not allowed for OBJ_ACK messages
			if (instId != UAVOBJ_ALL_INSTANCES)
			{
				// Unpack object, if the instance does not exist it will be created!
				if ( UAVObjUnpack(obj, instId, data) == 0 )
				{
					// Transmit ACK
					sendObject(connection, obj, instId, UAVTALK_TYPE_ACK, connection->iproc.msgId);
				}
				else
				{
					ret = -1;
				}
			}
			else
			{
				ret = -1;
			}
			break;
		case UAVTALK_TYPE_OBJ_REQ:
			// Send requested object if message is of type OBJ_REQ
			if (obj == 0)
				sendNack(connection, objId, connection->iproc.msgId);
			else
				sendObject(connection, obj, instId, UAVTALK_TYPE_OBJ, connection->iproc.msgId);
			break;
		case UAVTALK_TYPE_NACK:
			// Do nothing on flight side, let it time out.
			break;
		case UAVTALK_TYPE_ACK:
			// All instances, not allowed for ACK messages
			if (instId != UAVOBJ_ALL_INSTANCES)
			{
				// Check if an ack is pending
				updateAck(connection, obj, instId);
			}
			else
			{
				ret = -1;
			}
			break;
		default:
			ret = -1;
	}
	// Done
	return ret;
}

/**
 * Extract the packet length from a packet buffer.
 * \param[in] buf The packet buffer
 * \return The packet length
 */
static uint16_t packetLength(const uint8_t *buf) {
	return ((uint16_t)(buf[3]) << 8) + buf[2];
	
}

/**
 * Calculate the checksum of a packet buffer
 * \param[out] buf The packet buffer
 */
static void
packetCRC(uint8_t *buf) {
	uint16_t packet_length = packetLength(buf);
	buf[packet_length] = PIOS_CRC_updateCRC(0, buf, packet_length);
}

/**
 * Pack a packet header into a buffer.
 *
 * A packet consists of:
 *
 * bytes	field
 * -----	-----
 * 1	sync byte
 * 1	type
 * 2	packet length
 * 4	object ID
 * 2	instance ID (optional)
 * var	object
 *
 * \param[out] buf The output buffer
 * \param[in]  buf_length The length of the output buffer
 * \param[in]  type The packet type
 * \param[in]  objId The packet object ID
 * \param[in]  instId The instance ID
 * \param[in]  singleInstance Is the object a single instance object?
 * \param[in]  lenth the data (object) length
 * \return 0 on success
 * \return 1 on failure
 */
static int32_t
packHeader(uint8_t *buf, uint32_t buf_length, uint8_t type, uint32_t msgId,
					 uint32_t objId, uint16_t instId, uint8_t singleInstance,
					 uint16_t length)
{
	int32_t dataOffset = (singleInstance ? UAVTALK_MIN_HEADER_LENGTH : UAVTALK_MAX_HEADER_LENGTH);
	uint16_t packet_length = length + dataOffset;

	// Return an error if the packet is longer than the buffer length.
	if(packet_length >= buf_length)
		return -1;

	// Setup type and object id fields
	uint8_t cntr = 0;
	buf[cntr++] = UAVTALK_SYNC_VAL;  // sync byte
	buf[cntr++] = type;
	buf[cntr++] = (uint8_t)(packet_length & 0xFF);
	buf[cntr++] = (uint8_t)((packet_length >> 8) & 0xFF);
#ifdef PIOS_UAVTALK_DEBUG
	buf[cntr++] = (uint8_t)(msgId & 0xFF);
	buf[cntr++] = (uint8_t)((msgId >> 8) & 0xFF);
	buf[cntr++] = (uint8_t)((msgId >> 16) & 0xFF);
	buf[cntr++] = (uint8_t)((msgId >> 24) & 0xFF);
	uint16_t time = (uint16_t)(xTaskGetTickCount() & 0xffff);
	buf[cntr++] = (uint8_t)(time & 0xFF);
	buf[cntr++] = (uint8_t)((time >> 8) & 0xFF);
#endif
	buf[cntr++] = (uint8_t)(objId & 0xFF);
	buf[cntr++] = (uint8_t)((objId >> 8) & 0xFF);
	buf[cntr++] = (uint8_t)((objId >> 16) & 0xFF);
	buf[cntr++] = (uint8_t)((objId >> 24) & 0xFF);

	// Setup instance ID if one is required
	if (!singleInstance)
	{
		buf[cntr++] = (uint8_t)(instId & 0xFF);
		buf[cntr++] = (uint8_t)((instId >> 8) & 0xFF);
	}
	
	// Done
	return 0;
}

/**
 * Check if an ack is pending on an object and give response semaphore
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] obj Object
 * \param[in] instId The instance ID of UAVOBJ_ALL_INSTANCES for all instances.
 */
static void updateAck(UAVTalkConnectionData *connection, UAVObjHandle obj, uint16_t instId)
{
	if (connection->respObj == obj && (connection->respInstId == instId || connection->respInstId == UAVOBJ_ALL_INSTANCES))
	{
		xSemaphoreGive(connection->respSema);
		connection->respObj = 0;
	}
}

/**
 * Send an object through the telemetry link.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] obj Object handle to send
 * \param[in] instId The instance ID or UAVOBJ_ALL_INSTANCES for all instances
 * \param[in] type Transaction type
 * \return 0 Success
 * \return -1 Failure
 */
static int32_t sendObject(UAVTalkConnectionData *connection, UAVObjHandle obj, uint16_t instId, uint8_t type, uint32_t msgId)
{
	uint32_t numInst;
	uint32_t n;
	
	// If all instances are requested and this is a single instance object, force instance ID to zero
	if ( instId == UAVOBJ_ALL_INSTANCES && UAVObjIsSingleInstance(obj) )
	{
		instId = 0;
	}
	
	// Process message type
	if ( type == UAVTALK_TYPE_OBJ || type == UAVTALK_TYPE_OBJ_ACK )
	{
		if (instId == UAVOBJ_ALL_INSTANCES)
		{
			// Get number of instances
			numInst = UAVObjGetNumInstances(obj);
			// Send all instances
			for (n = 0; n < numInst; ++n)
			{
				sendSingleObject(connection, obj, n, type, msgId);
			}
			return 0;
		}
		else
		{
			return sendSingleObject(connection, obj, instId, type, msgId);
		}
	}
	else if (type == UAVTALK_TYPE_OBJ_REQ)
	{
		return sendSingleObject(connection, obj, instId, UAVTALK_TYPE_OBJ_REQ, msgId);
	}
	else if (type == UAVTALK_TYPE_ACK)
	{
		if ( instId != UAVOBJ_ALL_INSTANCES )
		{
			return sendSingleObject(connection, obj, instId, UAVTALK_TYPE_ACK, msgId);
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

/**
 * Send an object through the telemetry link.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] obj Object handle to send
 * \param[in] instId The instance ID (can NOT be UAVOBJ_ALL_INSTANCES, use sendObject() instead)
 * \param[in] type Transaction type
 * \return 0 Success
 * \return -1 Failure
 */
static int32_t sendSingleObject(UAVTalkConnectionData *connection, UAVObjHandle obj, uint16_t instId, uint8_t type, uint32_t msgId)
{
	uint8_t *buf = connection->txBuffer;
	uint32_t buf_size = connection->txSize;
	uint8_t singleInstance = UAVObjIsSingleInstance(obj);
	int32_t length;
	int32_t dataOffset;
	uint32_t objId;

	if (!connection->outStream) return -1;

	// Get the object ID
	objId = UAVObjGetID(obj);
	
	// Determine data length
	if (type == UAVTALK_TYPE_OBJ_REQ || type == UAVTALK_TYPE_ACK)
	{
		length = 0;
	}
	else
	{
		length = UAVObjGetNumBytes(obj);
	}

	// Determine the data offset (header size)
	dataOffset = singleInstance ? UAVTALK_MIN_HEADER_LENGTH : UAVTALK_MAX_HEADER_LENGTH;
	
	// Check length
	if (length >= UAVTALK_MAX_PAYLOAD_LENGTH)
	{
		return -1;
	}

	// Pack the packet into the txBuffer.
	if(packHeader(buf, buf_size, type, msgId, objId, instId, singleInstance, length) < 0)
		return -1;
	
	// Copy data (if any)
	if (length > 0)
		if ( UAVObjPack(obj, instId, buf + dataOffset) < 0 )
			return -1;

	// Calculate checksum
	packetCRC(buf);

	// Send the packet
	sendPacket(connection, buf, length, dataOffset + length);
	
	// Done
	return 0;
}

/**
 * Send a NACK through the telemetry link.
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] objId Object ID to send a NACK for
 * \return 0 Success
 * \return -1 Failure
 */
static int32_t sendNack(UAVTalkConnectionData *connection, uint32_t objId, uint32_t msgId)
{
	int32_t dataOffset = UAVTALK_MIN_HEADER_LENGTH;

	if (!connection->outStream) return -1;

	// Create the packet header.
	packHeader(connection->txBuffer, connection->txSize, UAVTALK_TYPE_NACK, msgId, objId, 0, 1, 0);

	// Calculate checksum
	packetCRC(connection->txBuffer);

	// Send buffer
	sendPacket(connection, connection->txBuffer, 0, dataOffset);

	// Done
	return 0;
}

/**
 * Send a packet of the connection
 * \param[in] connection UAVTalkConnection to be used
 * \param[in] buf The buffer to send
 * \param[in] length the data length
 * \param[in] packet_length the total packet length (not including CRC)
 */
static void sendPacket(UAVTalkConnectionData *connection, uint8_t* buf, uint16_t length, uint16_t packet_length)
{
	// Print a debug message.
	printPacketDebug(connection, 'S', buf);

	// Send buffer (partially if needed)
	uint32_t send_length = packet_length + UAVTALK_CHECKSUM_LENGTH;
	uint32_t sent=0;
	while (sent < send_length) {
		uint32_t sending = send_length - sent;
		if ( sending > connection->txSize ) sending = connection->txSize;
		if ( connection->outStream != NULL ) {
			(*connection->outStream)(buf+sent, sending);
		}
		sent += sending;
	}

	// Update stats
	++connection->stats.txObjects;
	connection->stats.txBytes += send_length;
	connection->stats.txObjectBytes += length;
}

#ifdef PIOS_UAVTALK_DEBUG
/**
 * Extract the packet type from a packet buffer.
 * \param[in] buf The packet buffer
 * \return The packet type
 */
static uint8_t packetType(const uint8_t *buf) {
	return (uint8_t)buf[1];
}

/**
 * Extract the MsgId from a packet buffer.
 * \param[in] buf The packet buffer
 * \return The MsgId
 */
static uint32_t packetMsgId(const uint8_t *buf) {
	return ((uint32_t)buf[7] << 24) + ((uint32_t)buf[6] << 16) + ((uint32_t)buf[5] << 8) + (uint32_t)buf[4];
}

/**
 * Extract the time from a packet buffer.
 * \param[in] buf The packet buffer
 * \return The time
 */
static uint16_t packetTime(const uint8_t *buf) {
	return ((uint32_t)buf[9] << 8) + (uint32_t)buf[8];
}

/**
 * Extract the ObjId from a packet buffer.
 * \param[in] buf The packet buffer
 * \return The ObjId
 */
static uint32_t packetObjId(const uint8_t *buf) {
#ifdef PIOS_UAVTALK_DEBUG
	return ((uint32_t)buf[13] << 24) + ((uint32_t)buf[12] << 16) + ((uint32_t)buf[11] << 8) + (uint32_t)buf[10];
#else
	return ((uint32_t)buf[7] << 24) + ((uint32_t)buf[6] << 16) + ((uint32_t)buf[5] << 8) + (uint32_t)buf[4];
#endif
}

/**
 * Print a line of debug for message tracking.
 */
static void printDebug(UAVTalkConnectionData *connection, char debugType, uint32_t msgId, uint16_t time, uint8_t type, uint32_t objId)
{
	uint16_t curTime = (uint16_t)(xTaskGetTickCount() &0xffff);
  sprintf(connection->printBuffer, "%c 0x%x 0x%x 0x%x 0x%x 0x%x\n\r", debugType, (unsigned int)curTime, (unsigned int)msgId, (unsigned int)time, type, (unsigned int)objId);
#ifdef USE_Debug
	InsertDebugQueue(connection->printBuffer);
#else
	PIOS_COM_SendString(PIOS_COM_DEBUG, connection->printBuffer);
#endif
}
static void printConnDebug(UAVTalkConnectionData *connection, char debugType)
{
	UAVTalkInputProcessor *iproc = &connection->iproc;
  printDebug(connection, debugType, iproc->msgId, iproc->time, iproc->type, iproc->objId);
}
static void printPacketDebug(UAVTalkConnectionData *connection, char debugType, const uint8_t *buf) {
	printDebug(connection, debugType, packetMsgId(buf), packetTime(buf), packetType(buf), packetObjId(buf));
}
#endif

/**
 * @}
 * @}
 */
