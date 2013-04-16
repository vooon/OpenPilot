//
//  cuavobject.cpp
//  gcs_osx
//
//  Created by Vova Starikh on 2/9/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#include "uavobject.h"
#include "uavobjectfield.h"
#include "io/CFile"
#include <assert.h>

// Macros
#define SET_BITS(var, shift, value, mask) var = (var & ~(mask << shift)) |	(value << shift);

UAVObject::UAVObject(opuint32 objID, bool isSingleInst, const CString& name)
{
    m_objID        = objID;
    m_instID       = 0;
    m_isSingleInst = isSingleInst;
    m_name         = name;
}

void UAVObject::initialize(opuint32 instID)
{
    CMutexSection locker(&m_mutex);
    m_instID = instID;
}

opuint32 UAVObject::getObjID() const
{
    return m_objID;
}

opuint32 UAVObject::getInstID() const
{
    return m_instID;
}

bool UAVObject::isSingleInstance() const
{
    return m_isSingleInst;
}

CString UAVObject::getName() const
{
    return m_name;
}

CString UAVObject::getCategory() const
{
    return m_category;
}

CString UAVObject::getDescription() const
{
    return m_description;
}

opuint32 UAVObject::getNumBytes() const
{
    return m_numBytes;
}

opint32 UAVObject::pack(opuint8* dataOut) const
{
    CMutexSection locker(&m_mutex);
    opint32 offset = 0;
    oplist<FIELD>::const_iterator it;
    for (it = m_fields.begin();it!=m_fields.end();it++)
    {
        UAVObjectField* objField = (*it).field;
        if (!objField)
            continue;
        objField->pack(&dataOut[offset]);
        offset += objField->getNumBytes();
    }
    return m_numBytes;
}

opint32 UAVObject::unpack(const opuint8* dataIn)
{
    CMutexSection locker(&m_mutex);
    opint32 offset = 0;
    oplist<FIELD>::const_iterator it;
    for (it = m_fields.begin();it!=m_fields.end();it++)
    {
        UAVObjectField* objField = (*it).field;
        if (!objField)
            continue;
        objField->unpack(&dataIn[offset]);
        offset += objField->getNumBytes();
    }
    m_signal_objectUnpacked.invoke(this);
    m_signal_objectUpdated.invoke(this);
    return m_numBytes;
    
}

bool UAVObject::save()
{
    CMutexSection locker(&m_mutex);
    
    // Open file
    CFile file(m_name + ".uavobj");
    if (!file.open(CFile::WriteOnly))
    {
        return false;
    }
    
    // Write object
    if (!save(file))
    {
        return false;
    }
    // Close file
    file.close();
    return true;
    
}

bool UAVObject::save(CFile& file)
{
    CMutexSection locker(&m_mutex);
    opuint8 buffer[m_numBytes];
    
    // Write the object ID
    if (file.write(&m_objID, 4) == -1) {
        return false;
    }
    
    // Write the instance ID
    if (!m_isSingleInst)
    {
        if (file.write(&m_instID, 2) == -1)
        {
            return false;
        }
    }
    
    // Write the data
    pack(buffer);
    if ( file.write((const char*)buffer, m_numBytes) == -1 )
    {
        return false;
    }
    
    // Done
    return true;
    
}

bool UAVObject::load()
{
    CMutexSection locker(&m_mutex);
    
    // Open file
    CFile file(m_name + ".uavobj");
    if (!file.open(CFile::ReadOnly))
    {
        return false;
    }
    
    // Load object
    if (!load(file))
    {
        return false;
    }
    // Close file
    file.close();
    return true;
    
}

bool UAVObject::load(CFile& file)
{
    CMutexSection locker(&m_mutex);
    opuint8 buffer[m_numBytes];
    
    // Read the object ID
    opuint32 objId = 0;
    if (file.read(&objId, 4) != 4)
    {
        return false;
    }
    // Check that the IDs match
    if (objId != m_objID)
    {
        return false;
    }

    // Read the instance ID
    opuint16 instId;
    if (file.read(&instId, 2) != 2)
    {
        return false;
    }

    // Check that the IDs match
    if (instId != m_instID)
    {
        return false;
    }
    
    // Read and unpack the data
    if (file.read((char*)buffer, m_numBytes) != int (m_numBytes))
    {
        return false;
    }
    unpack(buffer);
    // Done
    return true;
}

void UAVObject::lock()
{
    m_mutex.lock();
}

void UAVObject::lock(int timeoutMs)
{
    assert(0 == "need implement this function");
}

void UAVObject::unlock()
{
    m_mutex.unlock();
}

CMutex* UAVObject::getMutex()
{
    return &m_mutex;
}

opint32 UAVObject::getNumFields()
{
    CMutexSection locker(&m_mutex);
    return int (m_fields.size());
}

oplist<UAVObject::FIELD> UAVObject::getFields()
{
    CMutexSection locker(&m_mutex);
    return m_fields;
}

UAVObjectField* UAVObject::getField(const CString& name)
{
    CMutexSection locker(&m_mutex);
    // Look for field
    for (oplist<FIELD>::const_iterator it = m_fields.begin();it!=m_fields.end();it++)
    {
        UAVObjectField* field = (*it).field;
        if (!field)
            continue;
        if (name == field->getName())
        {
            return field;
        }
    }
    // If this point is reached then the field was not found
//    Logger()<<"UAVObject::getField Non existant field "<<name<<" requested.  This indicates a bug.  Make sure you also have null checking for non-debug code.";
    return NULL;
}

CString UAVObject::toString()
{
    CString sout;
    sout += toStringBrief() + CString(" ") + toStringData();
    return sout;
    
}

CString UAVObject::toStringBrief()
{
    CString sout = CString("%1 (ID: %2, InstID: %3, NumBytes: %4, SInst: %5)\n")
                .arg(getName())
                .arg(getObjID())
                .arg(getInstID())
                .arg(getNumBytes())
                .arg(isSingleInstance());
    return sout;
}

CString UAVObject::toStringData()
{
    CString sout;
    sout = CString("Data:\n");
    for (oplist<FIELD>::const_iterator it = m_fields.begin();it!=m_fields.end();it++)
    {
        UAVObjectField* field = (*it).field;
        if (!field)
            continue;
        sout += CString("\t%1").arg(field->toString());
    }
    return sout;
}

void UAVObject::emitTransactionCompleted(bool success)
{
    m_signal_transactionCompleted.invoke(this, success);
}

void UAVObject::emitNewInstance(UAVObject *obj)
{
    m_signal_newInstance.invoke(obj);
}

/////////////////////////////////////////////////////////////////////
//                        Metadata accessors                       //
/////////////////////////////////////////////////////////////////////

void UAVObject::MetadataInitialize(Metadata& meta)
{
	meta.flags =
    ACCESS_READWRITE << UAVOBJ_ACCESS_SHIFT |
    ACCESS_READWRITE << UAVOBJ_GCS_ACCESS_SHIFT |
    1 << UAVOBJ_TELEMETRY_ACKED_SHIFT |
    1 << UAVOBJ_GCS_TELEMETRY_ACKED_SHIFT |
    UPDATEMODE_ONCHANGE << UAVOBJ_TELEMETRY_UPDATE_MODE_SHIFT |
    UPDATEMODE_ONCHANGE << UAVOBJ_GCS_TELEMETRY_UPDATE_MODE_SHIFT;
	meta.flightTelemetryUpdatePeriod = 0;
	meta.gcsTelemetryUpdatePeriod = 0;
	meta.loggingUpdatePeriod = 0;
}

UAVObject::AccessMode UAVObject::GetFlightAccess(const Metadata& meta)
{
    return UAVObject::AccessMode((meta.flags >> UAVOBJ_ACCESS_SHIFT) & 1);
}

void UAVObject::SetFlightAccess(Metadata& meta, AccessMode mode)
{
    SET_BITS(meta.flags, UAVOBJ_ACCESS_SHIFT, mode, 1);
}

UAVObject::AccessMode UAVObject::GetGcsAccess(const Metadata& meta)
{
    return UAVObject::AccessMode((meta.flags >> UAVOBJ_GCS_ACCESS_SHIFT) & 1);
}

void UAVObject::SetGcsAccess(Metadata& meta, AccessMode mode)
{
    SET_BITS(meta.flags, UAVOBJ_GCS_ACCESS_SHIFT, mode, 1);
}

opuint8 UAVObject::GetFlightTelemetryAcked(const Metadata& meta)
{
    return (meta.flags >> UAVOBJ_TELEMETRY_ACKED_SHIFT) & 1;
}

void UAVObject::SetFlightTelemetryAcked(Metadata& meta, opuint8 val)
{
    SET_BITS(meta.flags, UAVOBJ_TELEMETRY_ACKED_SHIFT, val, 1);
}

opuint8 UAVObject::GetGcsTelemetryAcked(const Metadata& meta)
{
    return (meta.flags >> UAVOBJ_GCS_TELEMETRY_ACKED_SHIFT) & 1;
}

void UAVObject::SetGcsTelemetryAcked(Metadata& meta, opuint8 val)
{
    SET_BITS(meta.flags, UAVOBJ_GCS_TELEMETRY_ACKED_SHIFT, val, 1);
}

UAVObject::UpdateMode UAVObject::GetFlightTelemetryUpdateMode(const Metadata& meta)
{
    return UAVObject::UpdateMode((meta.flags >> UAVOBJ_TELEMETRY_UPDATE_MODE_SHIFT) & UAVOBJ_UPDATE_MODE_MASK);
}

void UAVObject::SetFlightTelemetryUpdateMode(Metadata& meta, UpdateMode val)
{
    SET_BITS(meta.flags, UAVOBJ_TELEMETRY_UPDATE_MODE_SHIFT, val, UAVOBJ_UPDATE_MODE_MASK);
}

UAVObject::UpdateMode UAVObject::GetGcsTelemetryUpdateMode(const Metadata& meta)
{
    return UAVObject::UpdateMode((meta.flags >> UAVOBJ_GCS_TELEMETRY_UPDATE_MODE_SHIFT) & UAVOBJ_UPDATE_MODE_MASK);
}

void UAVObject::SetGcsTelemetryUpdateMode(Metadata& meta, UpdateMode val)
{
    SET_BITS(meta.flags, UAVOBJ_GCS_TELEMETRY_UPDATE_MODE_SHIFT, val, UAVOBJ_UPDATE_MODE_MASK);
}

/////////////////////////////////////////////////////////////////////
//                              signals                            //
/////////////////////////////////////////////////////////////////////

CL_Signal_v1<UAVObject*>& UAVObject::signal_objectUpdated()
{
    return m_signal_objectUpdated;
}

CL_Signal_v1<UAVObject*>& UAVObject::signal_objectUnpacked()
{
    return m_signal_objectUnpacked;
}

CL_Signal_v2<UAVObject*, bool /*success*/>& UAVObject::signal_transactionCompleted()
{
    return m_signal_transactionCompleted;
}

CL_Signal_v1<UAVObject*>& UAVObject::signal_newInstance()
{
    return m_signal_newInstance;
}

CL_Signal_v1<UAVObject*>& UAVObject::signal_objectUpdatedAuto()
{
    return m_signal_objectUpdatedAuto;
}

CL_Signal_v1<UAVObject*>& UAVObject::signal_objectUpdatedManual()
{
    return m_signal_objectUpdatedManual;
}

CL_Signal_v1<UAVObject*>& UAVObject::signal_objectUpdatedPeriodic()
{
    return m_signal_objectUpdatedPeriodic;
}

CL_Signal_v1<UAVObject*>& UAVObject::signal_updateRequested()
{
    return m_signal_updateRequested;
}

//===================================================================
//  p r o t e c t e d   f u n c t i o n s
//===================================================================

void UAVObject::initializeFields(oplist<UAVObjectField*>& fields, opuint8* data, opuint32 numBytes)
{
    CMutexSection locker(&m_mutex);
    m_numBytes = numBytes;
    m_data = data;
//    m_fields = fields;
    // Initialize fields
    opuint32 offset = 0;
    for (oplist<UAVObjectField*>::iterator it = fields.begin();it!=fields.end();it++)
    {
        UAVObjectField* field = *it;
        if (!field)
            continue;
        FIELD stField;
        stField.field = field;
        field->initialize(data, offset, this);
        offset += field->getNumBytes();
        stField.slotFieldUpdated = field->signal_fieldUpdated().connect(this, &UAVObject::fieldUpdated);
        m_fields.push_back(stField);
    }
    
}

void UAVObject::setDescription(const CString& description)
{
    m_description= description;
}

void UAVObject::setCategory(const CString& category)
{
    m_category = category;
}

//-------------------------------------------------------------------
//  s l o t s
//-------------------------------------------------------------------

void UAVObject::requestUpdate()
{
    m_signal_updateRequested.invoke(this);
}

void UAVObject::updated()
{
    m_signal_objectUpdatedManual.invoke(this);
    m_signal_objectUpdated.invoke(this);
}

void UAVObject::fieldUpdated(UAVObjectField* /*field*/)
{
//    Q_UNUSED(field);
//    emit objectUpdatedAuto(this); // trigger object updated event
//    emit objectUpdated(this);
}
