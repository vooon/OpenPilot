//
//  uavtalk.cpp
//  gcs_osx
//
//  Created by Vova Starikh on 2/13/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#include "uavtalk.h"
#include "../uavobject/uavobject.h"
#include "../uavobject/uavobjectmanager.h"
#include "../uavobject/uavdataobject.h"
#include "io/CIODevice"
#include "Logger"
#include <assert.h>

//#define UAVTALK_DEBUG

#define SYNC_VAL 0x3C

const opuint8 UAVTalk::crc_table[256] = {
    0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
    0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
    0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
    0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
    0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
    0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
    0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
    0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
    0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
    0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
    0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
    0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
    0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
    0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
    0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
    0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
};

UAVTalk::UAVTalk(CIODevice* ioDev, UAVObjectManager* objMngr)
{
    m_ioDev   = ioDev;
    m_objMngr = objMngr;

    if (m_ioDev)
    {
        m_slotReadyRead = m_ioDev->signal_readyRead().connect(this, &UAVTalk::processStream);
        m_slotDeviceClosed = m_ioDev->signal_deviceClosed().connect(this, &UAVTalk::deviceClosed);
    }

    m_rxState        = STATE_SYNC;
    m_rxPacketLength = 0;
    
    memset(&m_stats, 0, sizeof(m_stats));
    
//    connect(io, SIGNAL(readyRead()), this, SLOT(processInputStream()));
//    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
//    Core::Internal::GeneralSettings * settings=pm->getObject<Core::Internal::GeneralSettings>();
//    useUDPMirror=settings->useUDPMirror();
//    qDebug()<<"USE UDP:::::::::::."<<useUDPMirror;
//    if(useUDPMirror)
//    {
//        udpSocketTx=new QUdpSocket(this);
//        udpSocketRx=new QUdpSocket(this);
//        udpSocketTx->bind(9000);
//        udpSocketRx->connectToHost(QHostAddress::LocalHost,9000);
//        connect(udpSocketTx,SIGNAL(readyRead()),this,SLOT(dummyUDPRead()));
//        connect(udpSocketRx,SIGNAL(readyRead()),this,SLOT(dummyUDPRead()));
//    }
}

UAVTalk::~UAVTalk()
{
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
    CMutexSection locker(&m_mutex);
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
 * Request an update for the specified object, on success the object data would have been
 * updated by the GCS.
 * \param[in] obj Object to update
 * \param[in] allInstances If set true then all instances will be updated
 * \return Success (true), Failure (false)
 */
bool UAVTalk::sendObjectRequest(UAVObject* obj, bool allInstances)
{
    CMutexSection locker(&m_mutex);
    return objectTransaction(obj, TYPE_OBJ_REQ, allInstances);
}

void UAVTalk::cancelTransaction(UAVObject* obj)
{
    CMutexSection locker(&m_mutex);
    opuint32 objId = obj->getObjID();
#pragma message("need rewrite")
//    if(io.isNull())
//        return;
    opmap<opuint32, Transaction*>::iterator itr = m_transMap.find(objId);
    if (m_transMap.count(objId))
    {
        m_transMap.erase(itr);
        delete (*itr).second;
    }
}

UAVTalk::ComStats UAVTalk::getStats()
{
    CMutexSection locker(&m_mutex);
    return m_stats;
}

void UAVTalk::resetStats()
{
    CMutexSection locker(&m_mutex);
    memset(&m_stats, 0, sizeof(m_stats));
}

/////////////////////////////////////////////////////////////////////
//                              signals                            //
/////////////////////////////////////////////////////////////////////

CL_Signal_v2<UAVObject*, bool /*sucess*/>& UAVTalk::signal_transactionComplete()
{
    return m_signalTransactionComplete;
}

//===================================================================
//  p r i v a t e   f u n c t i o n s
//===================================================================

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
bool UAVTalk::objectTransaction(UAVObject* obj, opuint8 type, bool allInstances)
{
    // Send object depending on if a response is needed
    if (type == TYPE_OBJ_ACK || type == TYPE_OBJ_REQ)
    {
        if (transmitObject(obj, type, allInstances))
        {
            Transaction *trans = new Transaction();
            trans->obj = obj;
            trans->allInstances = allInstances;
            m_transMap.insert(std::make_pair(obj->getObjID(), trans));
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

bool UAVTalk::processInputByte(opuint8 rxbyte)
{
    // Update stats
    m_stats.rxBytes++;

    m_rxPacketLength++;   // update packet byte count

//    if(useUDPMirror)
//        rxDataArray.append(rxbyte);

    // Receive state machine
    switch (m_rxState)
    {
        case STATE_SYNC:

            if (rxbyte != SYNC_VAL)
            {
#ifdef UAVTALK_DEBUG
                Logger() << "UAVTalk: Sync->Sync (" << rxbyte <<  " " + CString("0x%1").arg(rxbyte,2,16,'0') + ")";
#endif
                break;
            }

            // Initialize and update CRC
            m_rxCS = updateCRC(0, rxbyte);

            m_rxPacketLength = 1;

//            if(useUDPMirror)
//            {
//                rxDataArray.clear();
//                rxDataArray.append(rxbyte);
//            }

            m_rxState = STATE_TYPE;
#ifdef UAVTALK_DEBUG
            Logger() << "UAVTalk: Sync->Type";
#endif
            break;

        case STATE_TYPE:

            // Update CRC
            m_rxCS = updateCRC(m_rxCS, rxbyte);

            if ((rxbyte & TYPE_MASK) != TYPE_VER)
            {
                m_rxState = STATE_SYNC;
#ifdef UAVTALK_DEBUG
                Logger() <<"UAVTalk: Type->Sync";
#endif
                break;
            }

            m_rxType = rxbyte;

            m_packetSize = 0;

            m_rxState = STATE_SIZE;
#ifdef UAVTALK_DEBUG
            Logger() << "UAVTalk: Type->Size";
#endif
            m_rxCount = 0;
            break;

        case STATE_SIZE:

            // Update CRC
            m_rxCS = updateCRC(m_rxCS, rxbyte);

            if (m_rxCount == 0)
            {
                m_packetSize += rxbyte;
                m_rxCount++;
#ifdef UAVTALK_DEBUG
                Logger() << "UAVTalk: Size->Size";
#endif
                break;
            }

            m_packetSize += (opuint32)rxbyte << 8;

            if (m_packetSize < MIN_HEADER_LENGTH || m_packetSize > MAX_HEADER_LENGTH + MAX_PAYLOAD_LENGTH)
            {   // incorrect packet size
                m_rxState = STATE_SYNC;
#ifdef UAVTALK_DEBUG
                Logger() << "UAVTalk: Size->Sync";
#endif
                break;
            }

            m_rxCount = 0;
            m_rxState = STATE_OBJID;
#ifdef UAVTALK_DEBUG
            Logger() << "UAVTalk: Size->ObjID";
#endif
            break;

        case STATE_OBJID:

            // Update CRC
            m_rxCS = updateCRC(m_rxCS, rxbyte);

            m_rxTmpBuffer[m_rxCount++] = rxbyte;
            if (m_rxCount < 4)
            {
#ifdef UAVTALK_DEBUG
                Logger() << "UAVTalk: ObjID->ObjID";
#endif
                break;
            }

            // Search for object, if not found reset state machine
            m_rxObjId = *((opint32*)m_rxTmpBuffer);
            {
                UAVObject *rxObj = m_objMngr->getObject(m_rxObjId);
                if (rxObj == NULL && m_rxType != TYPE_OBJ_REQ)
                {
                    m_stats.rxErrors++;
                    m_rxState = STATE_SYNC;
#ifdef UAVTALK_DEBUG
                    Logger() << "UAVTalk: ObjID->Sync (badtype)";
#endif
                    break;
                }

                // Determine data length
                if (m_rxType == TYPE_OBJ_REQ || m_rxType == TYPE_ACK || m_rxType == TYPE_NACK)
                {
                    m_rxLength = 0;
                    m_rxInstanceLength = 0;
                }
                else
                {
                    if (rxObj) {                      // <- this if needed only for stupid static analizer code
                        m_rxLength = rxObj->getNumBytes();
                        m_rxInstanceLength = (rxObj->isSingleInstance() ? 0 : 2);
                    }
                }

                // Check length and determine next state
                if (m_rxLength >= MAX_PAYLOAD_LENGTH)
                {
                    m_stats.rxErrors++;
                    m_rxState = STATE_SYNC;
#ifdef UAVTALK_DEBUG
                    Logger() << "UAVTalk: ObjID->Sync (oversize)";
#endif
                    break;
                }

                // Check the lengths match
                if ((m_rxPacketLength + m_rxInstanceLength + m_rxLength) != m_packetSize)
                {   // packet error - mismatched packet size
                    m_stats.rxErrors++;
                    m_rxState = STATE_SYNC;
#ifdef UAVTALK_DEBUG
                    Logger() << "UAVTalk: ObjID->Sync (length mismatch)";
#endif
                    break;
                }

                // Check if this is a single instance object (i.e. if the instance ID field is coming next)
                if (rxObj == NULL)
                {
                   // This is a non-existing object, just skip to checksum
                   // and we'll send a NACK next.
                   m_rxState   = STATE_CS;
#ifdef UAVTALK_DEBUG
                   Logger() << "UAVTalk: ObjID->CSum (no obj)";
#endif
                   m_rxInstId = 0;
                   m_rxCount = 0;
                }
                else if (rxObj->isSingleInstance())
                {
                    // If there is a payload get it, otherwise receive checksum
                    if (m_rxLength > 0)
                    {
                        m_rxState = STATE_DATA;
#ifdef UAVTALK_DEBUG
                        Logger() << "UAVTalk: ObjID->Data (needs data)";
#endif
                    }
                    else
                    {
                        m_rxState = STATE_CS;
#ifdef UAVTALK_DEBUG
                        Logger() << "UAVTalk: ObjID->Checksum";
#endif
                    }
                    m_rxInstId = 0;
                    m_rxCount = 0;
                }
                else
                {
                    m_rxState = STATE_INSTID;
#ifdef UAVTALK_DEBUG
                    Logger() << "UAVTalk: ObjID->InstID";
#endif
                    m_rxCount = 0;
                }
            }

            break;

        case STATE_INSTID:

            // Update CRC
            m_rxCS = updateCRC(m_rxCS, rxbyte);

            m_rxTmpBuffer[m_rxCount++] = rxbyte;
            if (m_rxCount < 2)
            {
#ifdef UAVTALK_DEBUG
                Logger() << "UAVTalk: InstID->InstID";
#endif
                break;
            }

            m_rxInstId = *((opint16*)m_rxTmpBuffer);

            m_rxCount = 0;

            // If there is a payload get it, otherwise receive checksum
            if (m_rxLength > 0)
            {
                m_rxState = STATE_DATA;
#ifdef UAVTALK_DEBUG
                Logger() << "UAVTalk: InstID->Data";
#endif
            }
            else
            {
                m_rxState = STATE_CS;
#ifdef UAVTALK_DEBUG
                Logger() << "UAVTalk: InstID->CSum";
#endif
            }
            break;

        case STATE_DATA:

            // Update CRC
            m_rxCS = updateCRC(m_rxCS, rxbyte);

            m_rxBuffer[m_rxCount++] = rxbyte;
            if (m_rxCount < m_rxLength)
            {
#ifdef UAVTALK_DEBUG
                Logger() << "UAVTalk: Data->Data";
#endif
                break;
            }

            m_rxState = STATE_CS;
#ifdef UAVTALK_DEBUG
            Logger() << "UAVTalk: Data->CSum";
#endif
            m_rxCount = 0;
            break;

        case STATE_CS:

            // The CRC byte
            m_rxCSPacket = rxbyte;

            if (m_rxCS != m_rxCSPacket)
            {   // packet error - faulty CRC
                m_stats.rxErrors++;
                m_rxState = STATE_SYNC;
#ifdef UAVTALK_DEBUG
                Logger() << "UAVTalk: CSum->Sync (badcrc)";
#endif
                break;
            }

            if (m_rxPacketLength != m_packetSize + 1)
            {   // packet error - mismatched packet size
                m_stats.rxErrors++;
                m_rxState = STATE_SYNC;
#ifdef UAVTALK_DEBUG
                Logger() << "UAVTalk: CSum->Sync (length mismatch)";
#endif
                break;
            }

            {
                CMutexSection locker(&m_mutex);
                receiveObject(m_rxType, m_rxObjId, m_rxInstId, m_rxBuffer, m_rxLength);
//                if(useUDPMirror)
//                {
//                    udpSocketTx->writeDatagram(rxDataArray,QHostAddress::LocalHost,udpSocketRx->localPort());
//                }
                m_stats.rxObjectBytes += m_rxLength;
                m_stats.rxObjects++;
            }

            m_rxState = STATE_SYNC;
#ifdef UAVTALK_DEBUG
            Logger() << "UAVTalk: CSum->Sync (OK)";
#endif
            break;

        default:
            m_rxState = STATE_SYNC;
            m_stats.rxErrors++;
#ifdef UAVTALK_DEBUG
            Logger() << "UAVTalk: \?\?\?->Sync"; //Use the escape character for '?' so that the tripgraph isn't triggered.
#endif
            break;
    }
    // Done
    return true;
}

bool UAVTalk::receiveObject(opuint8 type, opuint32 objId, opuint16 instId, opuint8* data, opint32 /*length*/)
{
//    Q_UNUSED(length);
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
            obj = m_objMngr->getObject(objId);
        }
        else
        {
            obj = m_objMngr->getObject(objId, instId);
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
            obj = m_objMngr->getObject(objId, instId);
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
            obj = m_objMngr->getObject(objId, instId);
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

UAVObject* UAVTalk::updateObject(opuint32 objId, opuint16 instId, opuint8* data)
{
    // Get object
    UAVObject* obj = m_objMngr->getObject(objId, instId);
    // If the instance does not exist create it
    if (obj == NULL)
    {
        // Get the object type
        UAVObject* tobj = m_objMngr->getObject(objId);
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
        if ( !m_objMngr->registerObject(instobj) )
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

void UAVTalk::updateAck(UAVObject* obj)
{
    opuint32 objId = obj->getObjID();
    opmap<opuint32, Transaction*>::iterator itr = m_transMap.find(objId);
    if (itr != m_transMap.end() && ((*itr).second->obj->getInstID() == obj->getInstID() || (*itr).second->allInstances))
    {
        delete (*itr).second;
        m_transMap.erase(objId);
        m_signalTransactionComplete.invoke(obj, true);
    }
}

void UAVTalk::updateNack(UAVObject* obj)
{
    assert(obj);
    if (!obj)
        return;
    opuint32 objId = obj->getObjID();
    opmap<opuint32, Transaction*>::iterator itr = m_transMap.find(objId);
    if (itr != m_transMap.end() && ((*itr).second->obj->getInstID() == obj->getInstID() || (*itr).second->allInstances))
    {
        delete (*itr).second;
        m_transMap.erase(objId);
        m_signalTransactionComplete.invoke(obj, false);
    }
}

bool UAVTalk::transmitNack(opuint32 objId)
{
    int dataOffset = 8;

    m_txBuffer[0] = SYNC_VAL;
    m_txBuffer[1] = TYPE_NACK;

    *((opuint32*)&m_txBuffer[4]) = objId;

    *((opuint16*)&m_txBuffer[2]) = dataOffset;

    // Calculate checksum
    m_txBuffer[dataOffset] = updateCRC(0, m_txBuffer, dataOffset);

    int len = m_ioDev->write(m_txBuffer, dataOffset+CHECKSUM_LENGTH);
    if (len != dataOffset+CHECKSUM_LENGTH) {
        ++m_stats.txErrors;
        return false;
    }
//    if(useUDPMirror)
//    {
//        udpSocketRx->writeDatagram((const char*)txBuffer,dataOffset+CHECKSUM_LENGTH,QHostAddress::LocalHost,udpSocketTx->localPort());
//    }

    // Update stats
    m_stats.txBytes += 8+CHECKSUM_LENGTH;

    // Done
    return true;
}

bool UAVTalk::transmitObject(UAVObject* obj, opuint8 type, bool allInstances)
{
    // If all instances are requested on a single instance object it is an error
    if (allInstances && obj->isSingleInstance())
    {
        allInstances = false;
    }

    // Process message type
    if (type == TYPE_OBJ || type == TYPE_OBJ_ACK)
    {
        if (allInstances)
        {
            // Get number of instances
            opuint32 numInst = m_objMngr->getNumInstances(obj->getObjID());
            // Send all instances
            for (opuint32 instId = 0; instId < numInst; ++instId)
            {
                UAVObject* inst = m_objMngr->getObject(obj->getObjID(), instId);
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

bool UAVTalk::transmitSingleObject(UAVObject* obj, opuint8 type, bool allInstances)
{
    opint32 length;
    opint32 dataOffset;
    opuint32 objId;
    opuint16 instId;
    opuint16 allInstId = ALL_INSTANCES;

    // Setup type and object id fields
    objId = obj->getObjID();

    m_txBuffer[0] = SYNC_VAL;
    m_txBuffer[1] = type;
    *((opuint32*)&m_txBuffer[4]) = objId;

    // Setup instance ID if one is required
    if (obj->isSingleInstance())
    {
        dataOffset = 8;
    }
    else
    {
        // Check if all instances are requested
        if (allInstances) {
            *((opuint16*)&m_txBuffer[8]) = allInstId;
        }
        else
        {
            instId = obj->getInstID();
            *((opuint16*)&m_txBuffer[8]) = instId;
        }
        dataOffset = 10;
    }

    // Determine data length
    if (type == TYPE_OBJ_REQ || type == TYPE_ACK) {
        length = 0;
    } else {
        length = obj->getNumBytes();
    }

    // Check length
    if (length >= MAX_PAYLOAD_LENGTH) {
        return false;
    }

    // Copy data (if any)
    if (length > 0)
    {
        if (!obj->pack(&m_txBuffer[dataOffset])) {
            return false;
        }
    }

    *((opuint16*)&m_txBuffer[2]) = dataOffset + length;

    // Calculate checksum
    m_txBuffer[dataOffset+length] = updateCRC(0, m_txBuffer, dataOffset + length);

    int len = m_ioDev->write(m_txBuffer, dataOffset+length+CHECKSUM_LENGTH);
    if (len != dataOffset+length+CHECKSUM_LENGTH) {
        ++m_stats.txErrors;
        return false;
    }
//    if(useUDPMirror)
//    {
//        udpSocketRx->writeDatagram((const char*)txBuffer,dataOffset+length+CHECKSUM_LENGTH,QHostAddress::LocalHost,udpSocketTx->localPort());
//    }

    // Update stats
    ++m_stats.txObjects;
    m_stats.txBytes += dataOffset+length+CHECKSUM_LENGTH;
    m_stats.txObjectBytes += length;

    // Done
    return true;
}

opuint8 UAVTalk::updateCRC(opuint8 crc, const opuint8 data)
{
    return crc_table[crc ^ data];
}

opuint8 UAVTalk::updateCRC(opuint8 crc, const opuint8* data, opint32 length)
{
    while (length--)
        crc = crc_table[crc ^ *data++];
    return crc;
}

//-------------------------------------------------------------------
//  s l o t s
//-------------------------------------------------------------------

void UAVTalk::processStream(CIODevice* device)
{
    if (device != m_ioDev)
        return;
    CByteArray data;
    data.setSize(m_ioDev->bytesAvailable());
    device->read((void*)data.data(), data.size());
    const opuint8* buffer = (const opuint8*)data.data();
    for (int i = 0;i<int (data.size());i++)
    {
        processInputByte(buffer[i]);
    }
}

void UAVTalk::deviceClosed(CIODevice* device)
{
    if (device != m_ioDev)
        return;
    delete device;
    delete this;
}
