/**
 ******************************************************************************
 *
 * @file       uavtalk.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVTalkPlugin UAVTalk Plugin
 * @{
 * @brief The UAVTalk protocol plugin
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
#include "uavtalk.h"
#include <QtEndian>
#include <QDebug>

#define SYNC_VAL 0x3C

static const quint32 crc_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};


/**
 * Constructor
 */
UAVTalk::UAVTalk(QIODevice* iodev, UAVObjectManager* objMngr)
{
    io = iodev;

    this->objMngr = objMngr;

    rxState = STATE_SYNC;
    rxPacketLength = 0;

    mutex = new QMutex(QMutex::Recursive);

    respObj = NULL;

    memset(&stats, 0, sizeof(ComStats));

    connect(io, SIGNAL(readyRead()), this, SLOT(processInputStream()));
}

UAVTalk::~UAVTalk()
{
    // According to Qt, it is not necessary to disconnect upon
    // object deletion.
    //disconnect(io, SIGNAL(readyRead()), this, SLOT(processInputStream()));
}


/**
 * Reset the statistics counters
 */
void UAVTalk::resetStats()
{
    QMutexLocker locker(mutex);
    memset(&stats, 0, sizeof(ComStats));
}

/**
 * Get the statistics counters
 */
UAVTalk::ComStats UAVTalk::getStats()
{
    QMutexLocker locker(mutex);
    return stats;
}

/**
 * Called each time there are data in the input buffer
 */
void UAVTalk::processInputStream()
{
    quint8 tmp;

    while (io->bytesAvailable() > 0)
    {
        io->read((char*)&tmp, 1);
        processInputByte(tmp);
    }
}

/**
 * Request an update for the specified object, on success the object data would have been
 * updated by the GCS.
 * \param[in] obj Object to update
 * \param[in] allInstances If set true then all instances will be updated
 * \return Success (true), Failure (false)
 */
bool UAVTalk::sendObjectRequest(UAVObject* obj, bool allInstances)
{
    QMutexLocker locker(mutex);
    return objectTransaction(obj, TYPE_OBJ_REQ, allInstances);
}

/**
 * Send the specified object through the telemetry link.
 * \param[in] obj Object to send
 * \param[in] acked Selects if an ack is required
 * \param[in] allInstances If set true then all instances will be updated
 * \return Success (true), Failure (false)
 */
bool UAVTalk::sendObject(UAVObject* obj, bool acked, bool allInstances)
{
    QMutexLocker locker(mutex);
    if (acked)
    {
        return objectTransaction(obj, TYPE_OBJ_ACK, allInstances);
    }
    else
    {
        return objectTransaction(obj, TYPE_OBJ, allInstances);
    }
}

/**
 * Cancel a pending transaction
 */
void UAVTalk::cancelTransaction()
{
    QMutexLocker locker(mutex);
    respObj = NULL;
}

/**
 * Execute the requested transaction on an object.
 * \param[in] obj Object
 * \param[in] type Transaction type
 * 			  TYPE_OBJ: send object,
 * 			  TYPE_OBJ_REQ: request object update
 * 			  TYPE_OBJ_ACK: send object with an ack
 * \param[in] allInstances If set true then all instances will be updated
 * \return Success (true), Failure (false)
 */
bool UAVTalk::objectTransaction(UAVObject* obj, quint8 type, bool allInstances)
{
    // Send object depending on if a response is needed
    if (type == TYPE_OBJ_ACK || type == TYPE_OBJ_REQ)
    {
        if ( transmitObject(obj, type, allInstances) )
        {
            respObj = obj;
            respAllInstances = allInstances;    
            return true;
        }
        else
        {
            return false;
        }
    }
    else if (type == TYPE_OBJ)
    {
        return transmitObject(obj, TYPE_OBJ, allInstances);
    }
    else
    {
        return false;
    }
}

/**
 * Process an byte from the telemetry stream.
 * \param[in] rxbyte Received byte
 * \return Success (true), Failure (false)
 */
bool UAVTalk::processInputByte(quint8 rxbyte)
{
    // Update stats
    stats.rxBytes++;

    rxPacketLength++;   // update packet byte count

    // Receive state machine
    switch (rxState)
    {
        case STATE_SYNC:

            if (rxbyte != SYNC_VAL)
                break;

            // Initialize and update CRC
            rxCS = updateCRC(0, rxbyte);

            rxPacketLength = 1;

            rxState = STATE_TYPE;
            break;

        case STATE_TYPE:

            // Update CRC
            rxCS = updateCRC(rxCS, rxbyte);

            if ((rxbyte & TYPE_MASK) != TYPE_VER)
            {
                rxState = STATE_SYNC;
                break;
            }

            rxType = rxbyte;

            packetSize = 0;

            rxState = STATE_SIZE;
            rxCount = 0;
            break;

        case STATE_SIZE:

            // Update CRC
            rxCS = updateCRC(rxCS, rxbyte);

            if (rxCount == 0)
            {
                packetSize += rxbyte;
                rxCount++;
                break;
            }

            packetSize += (quint32)rxbyte << 8;

            if (packetSize < MIN_HEADER_LENGTH || packetSize > MAX_HEADER_LENGTH + MAX_PAYLOAD_LENGTH)
            {   // incorrect packet size
                rxState = STATE_SYNC;
                break;
            }

            rxCount = 0;
            rxState = STATE_OBJID;
            break;

        case STATE_OBJID:

            // Update CRC
            rxCS = updateCRC(rxCS, rxbyte);

            rxTmpBuffer[rxCount++] = rxbyte;
            if (rxCount < 4)
                break;

            // Search for object, if not found reset state machine
            rxObjId = (qint32)qFromLittleEndian<quint32>(rxTmpBuffer);
            {
                UAVObject *rxObj = objMngr->getObject(rxObjId);
                if (rxObj == NULL && rxType != TYPE_OBJ_REQ)
                {
                    stats.rxErrors++;
                    rxState = STATE_SYNC;
                    break;
                }

                // Determine data length
                if (rxType == TYPE_OBJ_REQ || rxType == TYPE_ACK || rxType == TYPE_NACK)
                    rxLength = 0;
                else
                    rxLength = rxObj->getNumBytes();

                // Check length and determine next state
                if (rxLength >= MAX_PAYLOAD_LENGTH)
                {
                    stats.rxErrors++;
                    rxState = STATE_SYNC;
                    break;
                }

                // Check the lengths match
                if ((rxPacketLength + rxLength) != packetSize)
                {   // packet error - mismatched packet size
                    stats.rxErrors++;
                    rxState = STATE_SYNC;
                    break;
                }

                // Check if this is a single instance object (i.e. if the instance ID field is coming next)
                if (rxObj == NULL)
                {
                   // This is a non-existing object, just skip to checksum
                   // and we'll send a NACK next.
                   rxState   = STATE_CS;
                   rxInstId = 0;
                   rxCount = 0;
                }
                else if (rxObj->isSingleInstance())
                {
                    // If there is a payload get it, otherwise receive checksum
                    if (rxLength > 0)
                        rxState = STATE_DATA;
                    else
                        rxState = STATE_CS;
                    rxInstId = 0;
                    rxCount = 0;
                }
                else
                {
                    rxState = STATE_INSTID;
                    rxCount = 0;
                }
            }

            break;

        case STATE_INSTID:

            // Update CRC
            rxCS = updateCRC(rxCS, rxbyte);

            rxTmpBuffer[rxCount++] = rxbyte;
            if (rxCount < 2)
                break;

            rxInstId = (qint16)qFromLittleEndian<quint16>(rxTmpBuffer);

            rxCount = 0;

            // If there is a payload get it, otherwise receive checksum
            if (rxLength > 0)
                rxState = STATE_DATA;
            else
                rxState = STATE_CS;

            break;

        case STATE_DATA:

            // Update CRC
            rxCS = updateCRC(rxCS, rxbyte);

            rxBuffer[rxCount++] = rxbyte;
            if (rxCount < rxLength)
                break;

            rxState = STATE_CS;
            rxCount = 0;
            break;

        case STATE_CS:

            // The CRC byte
            rxCSPacket = rxbyte;
            rxCS = (quint8) rxCS;

            if (rxCS != rxCSPacket)
            {   // packet error - faulty CRC
                stats.rxErrors++;
                rxState = STATE_SYNC;
                break;
            }

            if (rxPacketLength != packetSize + 1)
            {   // packet error - mismatched packet size
                stats.rxErrors++;
                rxState = STATE_SYNC;
                break;
            }

            mutex->lock();
                receiveObject(rxType, rxObjId, rxInstId, rxBuffer, rxLength);
                stats.rxObjectBytes += rxLength;
                stats.rxObjects++;
            mutex->unlock();

            rxState = STATE_SYNC;
            break;

        default:
            rxState = STATE_SYNC;
            stats.rxErrors++;
    }

    // Done
    return true;
}

/**
 * Receive an object. This function process objects received through the telemetry stream.
 * \param[in] type Type of received message (TYPE_OBJ, TYPE_OBJ_REQ, TYPE_OBJ_ACK, TYPE_ACK, TYPE_NACK)
 * \param[in] obj Handle of the received object
 * \param[in] instId The instance ID of UAVOBJ_ALL_INSTANCES for all instances.
 * \param[in] data Data buffer
 * \param[in] length Buffer length
 * \return Success (true), Failure (false)
 */
bool UAVTalk::receiveObject(quint8 type, quint32 objId, quint16 instId, quint8* data, qint32 length)
{
    Q_UNUSED(length);

    UAVObject* obj = NULL;
    bool error = false;
    bool allInstances =  (instId == ALL_INSTANCES);

    // Process message type
    switch (type) {
    case TYPE_OBJ:
        // All instances, not allowed for OBJ messages
        if (!allInstances)
        {
            // Get object and update its data
            obj = updateObject(objId, instId, data);
            // Check if an ack is pending
            if ( obj != NULL )
            {
                updateAck(obj);
            }
            else
            {
                error = true;
            }
        }
        else
        {
            error = true;
        }
        break;
    case TYPE_OBJ_ACK:
        // All instances, not allowed for OBJ_ACK messages
        if (!allInstances)
        {
            // Get object and update its data
            obj = updateObject(objId, instId, data);
            // Transmit ACK
            if ( obj != NULL )
            {
               transmitObject(obj, TYPE_ACK, false);
            }
            else
            {
                error = true;
            }
        }
        else
        {
            error = true;
        }
        break;
    case TYPE_OBJ_REQ:
        // Get object, if all instances are requested get instance 0 of the object
        if (allInstances)
        {
            obj = objMngr->getObject(objId);
        }
        else
        {
            obj = objMngr->getObject(objId, instId);
        }
        // If object was found transmit it
        if (obj != NULL)
        {
            transmitObject(obj, TYPE_OBJ, allInstances);
        }
        else
        {
            // Object was not found, transmit a NACK with the
            // objId which was not found.
            transmitNack(objId);
            error = true;
        }
        break;
    case TYPE_NACK:
        // All instances, not allowed for NACK messages
        if (!allInstances)
        {
            // Get object
            obj = objMngr->getObject(objId, instId);
            // Check if object exists:
            if (obj != NULL)
            {
                updateNack(obj);
            }
            else
            {
             error = true;
            }
        }
        break;
    case TYPE_ACK:
        // All instances, not allowed for ACK messages
        if (!allInstances)
        {
            // Get object
            obj = objMngr->getObject(objId, instId);
            // Check if an ack is pending
            if (obj != NULL)
            {
                updateAck(obj);
            }
            else
            {
                error = true;
            }
        }
        break;
    default:
        error = true;
    }
    // Done
    return !error;
}

/**
 * Update the data of an object from a byte array (unpack).
 * If the object instance could not be found in the list, then a
 * new one is created.
 */
UAVObject* UAVTalk::updateObject(quint32 objId, quint16 instId, quint8* data)
{
    // Get object
    UAVObject* obj = objMngr->getObject(objId, instId);
    // If the instance does not exist create it
    if (obj == NULL)
    {
        // Get the object type
        UAVObject* tobj = objMngr->getObject(objId);
        if (tobj == NULL)
        {
            return NULL;
        }
        // Make sure this is a data object
        UAVDataObject* dobj = dynamic_cast<UAVDataObject*>(tobj);
        if (dobj == NULL)
        {
            return NULL;
        }
        // Create a new instance, unpack and register
        UAVDataObject* instobj = dobj->clone(instId);
        if ( !objMngr->registerObject(instobj) )
        {
            return NULL;
        }
        instobj->unpack(data);
        return instobj;
    }
    else
    {
        // Unpack data into object instance
        obj->unpack(data);
        return obj;
    }
}

/**
 * Check if a transaction is pending and if yes complete it.
 */
void UAVTalk::updateNack(UAVObject* obj)
{
    if (respObj != NULL && respObj->getObjID() == obj->getObjID() && (respObj->getInstID() == obj->getInstID() || respAllInstances))
    {
        respObj = NULL;
        emit transactionCompleted(obj, false);
    }
}


/**
 * Check if a transaction is pending and if yes complete it.
 */
void UAVTalk::updateAck(UAVObject* obj)
{
    if (respObj != NULL && respObj->getObjID() == obj->getObjID() && (respObj->getInstID() == obj->getInstID() || respAllInstances))
    {
        respObj = NULL;
        emit transactionCompleted(obj, true);
    }
}


/**
 * Send an object through the telemetry link.
 * \param[in] obj Object to send
 * \param[in] type Transaction type
 * \param[in] allInstances True is all instances of the object are to be sent
 * \return Success (true), Failure (false)
 */
bool UAVTalk::transmitObject(UAVObject* obj, quint8 type, bool allInstances)
{   
    // If all instances are requested on a single instance object it is an error
    if (allInstances && obj->isSingleInstance())
    {
        allInstances = false;
    }

    // Process message type
    if ( type == TYPE_OBJ || type == TYPE_OBJ_ACK )
    {
        if (allInstances)
        {
            // Get number of instances
            quint32 numInst = objMngr->getNumInstances(obj->getObjID());
            // Send all instances
            for (quint32 instId = 0; instId < numInst; ++instId)
            {
                UAVObject* inst = objMngr->getObject(obj->getObjID(), instId);
                transmitSingleObject(inst, type, false);
            }
            return true;
        }
        else
        {
            return transmitSingleObject(obj, type, false);
        }
    }
    else if (type == TYPE_OBJ_REQ)
    {
        return transmitSingleObject(obj, TYPE_OBJ_REQ, allInstances);
    }
    else if (type == TYPE_ACK)
    {
        if (!allInstances)
        {
            return transmitSingleObject(obj, TYPE_ACK, false);
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

/**
 * Transmit a NACK through the telemetry link.
 * \param[in] objId the ObjectID we rejected
 */
bool UAVTalk::transmitNack(quint32 objId)
{
    int dataOffset = 8;

    txBuffer[0] = SYNC_VAL;
    txBuffer[1] = TYPE_NACK;
    qToLittleEndian<quint32>(objId, &txBuffer[4]);

    // Calculate checksum
    txBuffer[dataOffset] = (quint8) updateCRC(0, txBuffer, dataOffset);

    qToLittleEndian<quint16>(dataOffset, &txBuffer[2]);


    // Send buffer, check that the transmit backlog does not grow above limit
    if ( io->bytesToWrite() < TX_BUFFER_SIZE )
    {
        io->write((const char*)txBuffer, dataOffset+CHECKSUM_LENGTH);
    }
    else
    {
        ++stats.txErrors;
        return false;
    }

    // Update stats
    stats.txBytes += 8+CHECKSUM_LENGTH;

    // Done
    return true;

}


/**
 * Send an object through the telemetry link.
 * \param[in] obj Object handle to send
 * \param[in] type Transaction type
 * \return Success (true), Failure (false)
 */
bool UAVTalk::transmitSingleObject(UAVObject* obj, quint8 type, bool allInstances)
{
    qint32 length;
    qint32 dataOffset;
    quint32 objId;
    quint16 instId;
    quint16 allInstId = ALL_INSTANCES;

    // Setup type and object id fields
    objId = obj->getObjID();
    txBuffer[0] = SYNC_VAL;
    txBuffer[1] = type;
    qToLittleEndian<quint32>(objId, &txBuffer[4]);

    // Setup instance ID if one is required
    if ( obj->isSingleInstance() )
    {
        dataOffset = 8;
    }
    else
    {
        // Check if all instances are requested
        if (allInstances)
        {
            qToLittleEndian<quint16>(allInstId, &txBuffer[8]);
        }
        else
        {
            instId = obj->getInstID();
            qToLittleEndian<quint16>(instId, &txBuffer[8]);
        }
        dataOffset = 10;
    }

    // Determine data length
    if (type == TYPE_OBJ_REQ || type == TYPE_ACK)
    {
        length = 0;
    }
    else
    {
        length = obj->getNumBytes();
    }

    // Check length
    if (length >= MAX_PAYLOAD_LENGTH)
    {
        return false;
    }

    // Copy data (if any)
    if (length > 0)
    {
        if ( !obj->pack(&txBuffer[dataOffset]) )
        {
            return false;
        }
    }

    qToLittleEndian<quint16>(dataOffset + length, &txBuffer[2]);

    // Calculate checksum
    txBuffer[dataOffset+length] = (quint8) updateCRC(0, txBuffer, dataOffset + length);

    // Send buffer, check that the transmit backlog does not grow above limit
    if ( io->bytesToWrite() < TX_BUFFER_SIZE )
    {
        io->write((const char*)txBuffer, dataOffset+length+CHECKSUM_LENGTH);
    }
    else
    {
        ++stats.txErrors;
        return false;
    }

    // Update stats
    ++stats.txObjects;
    stats.txBytes += dataOffset+length+CHECKSUM_LENGTH;
    stats.txObjectBytes += length;

    // Done
    return true;
}

/**
 * Update the crc value with new data.
 *
 * \param crc      The current crc value.
 * \param data     Pointer to a buffer of \a data_len bytes.
 * \param data_len Number of bytes in the \a data buffer.
 * \return         The updated crc value.
 *****************************************************************************/
quint32 crc_update(quint32 crc, const unsigned char *data, size_t data_len)
{
    unsigned int tbl_idx;

    while (data_len--) {
        tbl_idx = (crc ^ *data) & 0xff;
        crc = (crc_table[tbl_idx] ^ (crc >> 8)) & 0xffffffff;

        data++;
    }
    return crc & 0xffffffff;
}


quint32 UAVTalk::updateCRC(quint32 crc, const quint8 data)
{
    return crc_update(crc, &data, 1);
}
quint32 UAVTalk::updateCRC(quint32 crc, const quint8* data, qint32 length)
{
    return crc_update(crc, data, length);
}














