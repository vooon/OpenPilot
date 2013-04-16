/**
 ******************************************************************************
 *
 * @file       uavmetaobject.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVObjectsPlugin UAVObjects Plugin
 * @{
 * @brief      The UAVUObjects GCS plugin
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

#include "uavmetaobject.h"
#include "uavobjectfield.h"

UAVMetaObject::UAVMetaObject(opuint32 objID, const CString& name, UAVObject* parent) :
    UAVObject(objID, true, name)
{
    m_parent = parent;
    // Setup default metadata of metaobject (can not be changed)
    UAVObject::MetadataInitialize(m_ownMetadata);
    // Setup fields
    CStringList modesBitField;
    modesBitField << CString("FlightReadOnly")
                  << CString("GCSReadOnly")
                  << CString("FlightTelemetryAcked")
                  << CString("GCSTelemetryAcked")
                  << CString("FlightUpdatePeriodic")
                  << CString("FlightUpdateOnChange")
                  << CString("GCSUpdatePeriodic")
                  << CString("GCSUpdateOnChange");
    
    oplist<UAVObjectField*> fields;
    fields.push_back( new UAVObjectField(CString("Modes"), CString("boolean"), UAVObjectField::BITFIELD, modesBitField, CStringList()) );
    fields.push_back( new UAVObjectField(CString("Flight Telemetry Update Period"), CString("ms"), UAVObjectField::UINT16, 1, CStringList()) );
    fields.push_back( new UAVObjectField(CString("GCS Telemetry Update Period"), CString("ms"), UAVObjectField::UINT16, 1, CStringList()) );
    fields.push_back( new UAVObjectField(CString("Logging Update Period"), CString("ms"), UAVObjectField::UINT16, 1, CStringList()) );
    // Initialize parent
    UAVObject::initialize(0);
    UAVObject::initializeFields(fields, (opuint8*)&m_parentMetadata, sizeof(Metadata));
    // Setup metadata of parent
    m_parentMetadata = parent->getDefaultMetadata();
}

UAVObject* UAVMetaObject::getParentObject()
{
    return m_parent;
}

void UAVMetaObject::setMetadata(const Metadata& /*mdata*/)
{
//    Q_UNUSED(mdata);
//    return; // can not update metaobject's metadata
}

UAVMetaObject::Metadata UAVMetaObject::getMetadata()
{
    return m_ownMetadata;
}

UAVMetaObject::Metadata UAVMetaObject::getDefaultMetadata()
{
    return m_ownMetadata;
}

void UAVMetaObject::setData(const Metadata& mdata)
{
    CMutexSection locker(&m_mutex);
    m_parentMetadata = mdata;
    m_signal_objectUpdatedAuto.invoke(this); // trigger object updated event
    m_signal_objectUpdated.invoke(this);
}

UAVMetaObject::Metadata UAVMetaObject::getData()
{
    CMutexSection locker(&m_mutex);
    return m_parentMetadata;
}
