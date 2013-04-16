//
//  uavdataobject.cpp
//  gcs_osx
//
//  Created by Vova Starikh on 2/10/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#include "uavdataobject.h"
#include "uavmetaobject.h"

UAVDataObject::UAVDataObject(opuint32 objID, bool isSingleInst, bool isSet, const CString& name) :
    UAVObject(objID, isSingleInst, name)
{
    m_mobj  = NULL;
    m_isSet = isSet;
}

void UAVDataObject::initialize(opuint32 instID, UAVMetaObject* mobj)
{
    CMutexSection locker(&m_mutex);
    m_mobj = mobj;
    UAVObject::initialize(instID);
}

void UAVDataObject::initialize(UAVMetaObject* mobj)
{
    CMutexSection locker(&m_mutex);
    m_mobj = mobj;
}

bool UAVDataObject::isSettings()
{
    return m_isSet;
}

void UAVDataObject::setMetadata(const Metadata& mdata)
{
    if (m_mobj != NULL)
    {
        m_mobj->setData(mdata);
    }
}

UAVDataObject::Metadata UAVDataObject::getMetadata()
{
    if (m_mobj != NULL)
    {
        return m_mobj->getData();
    }
    else
    {
        return getDefaultMetadata();
    }
}

UAVMetaObject* UAVDataObject::getMetaObject()
{
    return m_mobj;
}
