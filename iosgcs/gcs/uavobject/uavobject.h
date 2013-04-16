//
//  cuavobject.h
//  gcs_osx
//
//  Created by Vova Starikh on 2/9/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#ifndef __gcs_osx__cuavobject__
#define __gcs_osx__cuavobject__

#include "OPTypes"
#include "text/CString"
#include "thread/CMutex"
#include "OPList"
#include "signals/Signals"

class CFile;

class UAVObjectField;

// Constants
#define UAVOBJ_ACCESS_SHIFT 0
#define UAVOBJ_GCS_ACCESS_SHIFT 1
#define UAVOBJ_TELEMETRY_ACKED_SHIFT 2
#define UAVOBJ_GCS_TELEMETRY_ACKED_SHIFT 3
#define UAVOBJ_TELEMETRY_UPDATE_MODE_SHIFT 4
#define UAVOBJ_GCS_TELEMETRY_UPDATE_MODE_SHIFT 6
#define UAVOBJ_UPDATE_MODE_MASK 0x3

class UAVObject
{
public:
    typedef struct tagField {
        UAVObjectField * field;
        CL_Slot          slotFieldUpdated;
    } FIELD, *PFIELD;

    /**
     * Object update mode
     */
    enum UpdateMode {
        UPDATEMODE_MANUAL = 0,  /** Manually update object, by calling the updated() function */
        UPDATEMODE_PERIODIC = 1, /** Automatically update object at periodic intervals */
        UPDATEMODE_ONCHANGE = 2, /** Only update object when its data changes */
        UPDATEMODE_THROTTLED = 3 /** Object is updated on change, but not more often than the interval time */
    };
    
    /**
     * Access mode
     */
    enum AccessMode {
        ACCESS_READWRITE = 0,
        ACCESS_READONLY = 1
    };
    
    /**
     * Object metadata, each object has a meta object that holds its metadata. The metadata define
     * properties for each object and can be used by multiple modules (e.g. telemetry and logger)
     *
     * The object metadata flags are packed into a single 16 bit integer.
     * The bits in the flag field are defined as:
     *
     *   Bit(s)  Name                       Meaning
     *   ------  ----                       -------
     *      0    access                     Defines the access level for the local transactions (readonly=0 and readwrite=1)
     *      1    gcsAccess                  Defines the access level for the local GCS transactions (readonly=0 and readwrite=1), not used in the flight s/w
     *      2    telemetryAcked             Defines if an ack is required for the transactions of this object (1:acked, 0:not acked)
     *      3    gcsTelemetryAcked          Defines if an ack is required for the transactions of this object (1:acked, 0:not acked)
     *    4-5    telemetryUpdateMode        Update mode used by the telemetry module (UAVObjUpdateMode)
     *    6-7    gcsTelemetryUpdateMode     Update mode used by the GCS (UAVObjUpdateMode)
     */
    typedef struct {
        opuint8  flags; /** Defines flags for update and logging modes and whether an update should be ACK'd (bits defined above) */
        opuint16 flightTelemetryUpdatePeriod; /** Update period used by the telemetry module (only if telemetry mode is PERIODIC) */
        opuint16 gcsTelemetryUpdatePeriod; /** Update period used by the GCS (only if telemetry mode is PERIODIC) */
        opuint16 loggingUpdatePeriod; /** Update period used by the logging module (only if logging mode is PERIODIC) */
    } __attribute__((packed)) Metadata;
    
public:
    UAVObject(opuint32 objID, bool isSingleInst, const CString& name);
    
    void initialize(opuint32 instID);
    opuint32 getObjID() const;
    opuint32 getInstID() const;
    bool isSingleInstance() const;
    CString getName() const;
    CString getCategory() const;
    CString getDescription() const;
    opuint32 getNumBytes() const;
    opint32 pack(opuint8* dataOut) const;
    opint32 unpack(const opuint8* dataIn);
    bool save();
    bool save(CFile& file);
    bool load();
    bool load(CFile& file);
    void lock();
    void lock(int timeoutMs);
    void unlock();
    CMutex* getMutex();
    opint32 getNumFields();
    oplist<FIELD> getFields();
    UAVObjectField* getField(const CString& name);
    CString toString();
    CString toStringBrief();
    CString toStringData();
    void emitTransactionCompleted(bool success);
    void emitNewInstance(UAVObject *);

    virtual void setMetadata(const Metadata& mdata) = 0;
    virtual Metadata getMetadata() = 0;
    virtual Metadata getDefaultMetadata() = 0;

    // Metadata accessors
    static void MetadataInitialize(Metadata& meta);
    static AccessMode GetFlightAccess(const Metadata& meta);
    static void SetFlightAccess(Metadata& meta, AccessMode mode);
    static AccessMode GetGcsAccess(const Metadata& meta);
    static void SetGcsAccess(Metadata& meta, AccessMode mode);
    static opuint8 GetFlightTelemetryAcked(const Metadata& meta);
    static void SetFlightTelemetryAcked(Metadata& meta, opuint8 val);
    static opuint8 GetGcsTelemetryAcked(const Metadata& meta);
    static void SetGcsTelemetryAcked(Metadata& meta, opuint8 val);
    static UpdateMode GetFlightTelemetryUpdateMode(const Metadata& meta);
    static void SetFlightTelemetryUpdateMode(Metadata& meta, UpdateMode val);
    static UpdateMode GetGcsTelemetryUpdateMode(const Metadata& meta);
    static void SetGcsTelemetryUpdateMode(Metadata& meta, UpdateMode val);

// signals
    CL_Signal_v1<UAVObject*>& signal_objectUpdated();
    CL_Signal_v1<UAVObject*>& signal_objectUnpacked();
    CL_Signal_v2<UAVObject*, bool /*success*/>& signal_transactionCompleted();
    CL_Signal_v1<UAVObject*>& signal_newInstance();
    CL_Signal_v1<UAVObject*>& signal_objectUpdatedAuto();
    CL_Signal_v1<UAVObject*>& signal_objectUpdatedManual();
    CL_Signal_v1<UAVObject*>& signal_objectUpdatedPeriodic();
    CL_Signal_v1<UAVObject*>& signal_updateRequested();
    
protected:
    opuint32         m_objID;
    opuint32         m_instID;
    bool             m_isSingleInst;
    CString          m_name;
    CString          m_description;
    CString          m_category;
    opuint32         m_numBytes;
    mutable CMutex   m_mutex;
    opuint8        * m_data;
    oplist<FIELD>    m_fields;
    
    CL_Signal_v1<UAVObject*> m_signal_objectUpdated;
    CL_Signal_v1<UAVObject*> m_signal_objectUnpacked;
    CL_Signal_v2<UAVObject*, bool /*success*/> m_signal_transactionCompleted;
    CL_Signal_v1<UAVObject*> m_signal_newInstance;
    CL_Signal_v1<UAVObject*> m_signal_objectUpdatedAuto;
    CL_Signal_v1<UAVObject*> m_signal_objectUpdatedManual;
    CL_Signal_v1<UAVObject*> m_signal_objectUpdatedPeriodic;
    CL_Signal_v1<UAVObject*> m_signal_updateRequested;
    
    void initializeFields(oplist<UAVObjectField*>& fields, opuint8* data, opuint32 numBytes);
    void setDescription(const CString& description);
    void setCategory(const CString& category);
    
    // slots
public:
    void requestUpdate();
    void updated();
    void fieldUpdated(UAVObjectField* field);       // private slot
};


#endif /* defined(__gcs_osx__cuavobject__) */
