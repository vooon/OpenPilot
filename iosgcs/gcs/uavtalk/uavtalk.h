//
//  uavtalk.h
//  gcs_osx
//
//  Created by Vova Starikh on 2/13/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#ifndef __gcs_osx__uavtalk__
#define __gcs_osx__uavtalk__

#include "OPTypes"
#include "signals/Signals"
#include "thread/CMutex"
#include "OPMap"

class CIODevice;

class UAVObjectManager;
class UAVObject;

class UAVTalk
{
private:
    typedef struct {
        UAVObject* obj;
        bool allInstances;
    } Transaction;
    
    // Constants
    static const int TYPE_MASK = 0xF8;
    static const int TYPE_VER = 0x20;
    static const int TYPE_OBJ = (TYPE_VER | 0x00);
    static const int TYPE_OBJ_REQ = (TYPE_VER | 0x01);
    static const int TYPE_OBJ_ACK = (TYPE_VER | 0x02);
    static const int TYPE_ACK = (TYPE_VER | 0x03);
    static const int TYPE_NACK = (TYPE_VER | 0x04);
    
    static const int MIN_HEADER_LENGTH = 8; // sync(1), type (1), size(2), object ID(4)
    static const int MAX_HEADER_LENGTH = 10; // sync(1), type (1), size(2), object ID (4), instance ID(2, not used in single objects)
    
    static const int CHECKSUM_LENGTH = 1;
    
    static const int MAX_PAYLOAD_LENGTH = 256;
    
    static const int MAX_PACKET_LENGTH = (MAX_HEADER_LENGTH + MAX_PAYLOAD_LENGTH + CHECKSUM_LENGTH);
    
    static const opuint16 ALL_INSTANCES = 0xFFFF;
    static const opuint16 OBJID_NOTFOUND = 0x0000;
    
    static const int TX_BUFFER_SIZE = 2*1024;
    static const opuint8 crc_table[256];
    
    // Types
    enum RxStateType {
        STATE_SYNC,
        STATE_TYPE,
        STATE_SIZE,
        STATE_OBJID,
        STATE_INSTID,
        STATE_DATA,
        STATE_CS
    };
    
public:
    typedef struct {
        opuint32 txBytes;
        opuint32 rxBytes;
        opuint32 txObjectBytes;
        opuint32 rxObjectBytes;
        opuint32 rxObjects;
        opuint32 txObjects;
        opuint32 txErrors;
        opuint32 rxErrors;
    } ComStats;
    
public:
    UAVTalk(CIODevice* ioDev, UAVObjectManager* objMngr);
    ~UAVTalk();
    
    bool sendObject(UAVObject* obj, bool acked, bool allInstances);
    bool sendObjectRequest(UAVObject* obj, bool allInstances);
    void cancelTransaction(UAVObject* obj);
    ComStats getStats();
    void resetStats();
    
// signals
    CL_Signal_v2<UAVObject*, bool /*sucess*/>& signal_transactionComplete();
    
private:
    CIODevice        * m_ioDev;
    UAVObjectManager * m_objMngr;
    
    CMutex             m_mutex;
    opmap<opuint32, Transaction*> m_transMap;
    opuint8            m_rxBuffer[MAX_PACKET_LENGTH];
    opuint8            m_txBuffer[MAX_PACKET_LENGTH];
    // Variables used by the receive state machine
    opuint8            m_rxTmpBuffer[4];
    opuint8            m_rxType;
    opuint32           m_rxObjId;
    opuint16           m_rxInstId;
    opuint16           m_rxLength;
    opuint16           m_rxPacketLength;
    opuint8            m_rxInstanceLength;
    
    opuint8            m_rxCSPacket, m_rxCS;
    opint32            m_rxCount;
    opint32            m_packetSize;
    RxStateType        m_rxState;
    ComStats           m_stats;
    
    bool               m_useUDPMirror;
//    QUdpSocket * udpSocketTx;
//    QUdpSocket * udpSocketRx;
//    QByteArray rxDataArray;
    
    CL_Signal_v2<UAVObject*, bool /*sucess*/> m_signalTransactionComplete;

    CL_Slot m_slotReadyRead;
    CL_Slot m_slotDeviceClosed;

    bool objectTransaction(UAVObject* obj, opuint8 type, bool allInstances);
    bool processInputByte(opuint8 rxbyte);
    bool receiveObject(opuint8 type, opuint32 objId, opuint16 instId, opuint8* data, opint32 length);
    UAVObject* updateObject(opuint32 objId, opuint16 instId, opuint8* data);
    void updateAck(UAVObject* obj);
    void updateNack(UAVObject* obj);
    bool transmitNack(opuint32 objId);
    bool transmitObject(UAVObject* obj, opuint8 type, bool allInstances);
    bool transmitSingleObject(UAVObject* obj, opuint8 type, bool allInstances);
    opuint8 updateCRC(opuint8 crc, const opuint8 data);
    opuint8 updateCRC(opuint8 crc, const opuint8* data, opint32 length);

// slots
    void processStream(CIODevice* device);
    void deviceClosed(CIODevice* device);
};


#endif /* defined(__gcs_osx__uavtalk__) */
