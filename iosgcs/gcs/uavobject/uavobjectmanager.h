/**
 ******************************************************************************
 *
 * @file       uavobjectmanager.h
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

#ifndef __gcs_osx__uavobjectmanager__
#define __gcs_osx__uavobjectmanager__

#include "OPTypes"
#include "OPVector"
#include "signals/Signals"
#include "thread/CMutex"

class CString;

class UAVObject;
class UAVDataObject;
class UAVMetaObject;

class UAVObjectManager
{
public:
    UAVObjectManager();
    ~UAVObjectManager();

    bool registerObject(UAVDataObject* obj);
    opvector< opvector<UAVObject*> > getObjects();
    opvector< opvector<UAVDataObject*> > getDataObjects();
    opvector< opvector<UAVMetaObject*> > getMetaObjects();
    UAVObject* getObject(const CString& name, opuint32 instId = 0);
    UAVObject* getObject(opuint32 objId, opuint32 instId = 0);
    opvector<UAVObject*> getObjectInstances(const CString& name);
    opvector<UAVObject*> getObjectInstances(opuint32 objId);
    opint32 getNumInstances(const CString& name);
    opint32 getNumInstances(opuint32 objId);

// signals
    CL_Signal_v1<UAVObject*>& signal_newObject();
    CL_Signal_v1<UAVObject*>& signal_newInstance();

private:
    static const opuint32 MAX_INSTANCES = 1000;
    
    opvector< opvector<UAVObject*> > m_objects;
    CMutex m_mutex;
    
    CL_Signal_v1<UAVObject*> m_signal_newObject;
    CL_Signal_v1<UAVObject*> m_signal_newInstance;

    
    void addObject(UAVObject* obj);
    UAVObject* getObject(const CString* name, opuint32 objId, opuint32 instId);
    opvector<UAVObject*> getObjectInstances(const CString* name, opuint32 objId);
    opint32 getNumInstances(const CString* name, opuint32 objId);
};

#endif /* defined(__gcs_osx__uavobjectmanager__) */
