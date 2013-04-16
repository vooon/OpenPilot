//
//  uavobjectmanager.cpp
//  gcs_osx
//
//  Created by Vova Starikh on 2/11/13.
//  Copyright (c) 2013 Open Pilot. All rights reserved.
//

#include "uavobjectmanager.h"
#include "uavdataobject.h"
#include "uavmetaobject.h"
#include "Logger"

UAVObjectManager::UAVObjectManager()
{
    
}

UAVObjectManager::~UAVObjectManager()
{
    
}

bool UAVObjectManager::registerObject(UAVDataObject* obj)
{
    CMutexSection locker(&m_mutex);
    // Check if this object type is already in the list
    for (int objidx = 0; objidx < int (m_objects.size()); ++objidx)
    {
        // Check if the object ID is in the list
        if (m_objects[objidx].size() > 0 && m_objects[objidx][0]->getObjID() == obj->getObjID())
        {
            // Check if this is a single instance object, if yes we can not add a new instance
            if (obj->isSingleInstance())
            {
                return false;
            }
            // The object type has alredy been added, so now we need to initialize the new instance with the appropriate id
            // There is a single metaobject for all object instances of this type, so no need to create a new one
            // Get object type metaobject from existing instance
            UAVDataObject* refObj = dynamic_cast<UAVDataObject*>(m_objects[objidx][0]);
            if (refObj == NULL)
            {
                return false;
            }
            UAVMetaObject* mobj = refObj->getMetaObject();
            // If the instance ID is specified and not at the default value (0) then we need to make sure
            // that there are no gaps in the instance list. If gaps are found then then additional instances
            // will be created.
            if ( (obj->getInstID() > 0) && (obj->getInstID() < MAX_INSTANCES) )
            {
                for (int instidx = 0; instidx < int (m_objects[objidx].size()); ++instidx)
                {
                    if ( m_objects[objidx][instidx]->getInstID() == obj->getInstID() )
                    {
                        // Instance conflict, do not add
                        return false;
                    }
                }
                // Check if there are any gaps between the requested instance ID and the ones in the list,
                // if any then create the missing instances.
                for (opuint32 instidx = int (m_objects[objidx].size()); instidx < obj->getInstID(); ++instidx)
                {
                    UAVDataObject* cobj = obj->clone(instidx);
                    cobj->initialize(mobj);
                    m_objects[objidx].push_back(cobj);
                    getObject(cobj->getObjID())->emitNewInstance(cobj);
                    m_signal_newInstance.invoke(cobj);
                }
                // Finally, initialize the actual object instance
                obj->initialize(mobj);
            }
            else if (obj->getInstID() == 0)
            {
                // Assign the next available ID and initialize the object instance
                obj->initialize(int (m_objects[objidx].size()), mobj);
            }
            else
            {
                return false;
            }
            // Add the actual object instance in the list
            m_objects[objidx].push_back(obj);
            getObject(obj->getObjID())->emitNewInstance(obj);
            m_signal_newInstance.invoke(obj);
            return true;
        }
    }
    // If this point is reached then this is the first time this object type (ID) is added in the list
    // create a new list of the instances, add in the object collection and create the object's metaobject
    // Create metaobject
    CString mname = obj->getName();
    mname += CString("Meta");
    UAVMetaObject* mobj = new UAVMetaObject(obj->getObjID()+1, mname, obj);
    // Initialize object
    obj->initialize(0, mobj);
    // Add to list
    addObject(obj);
    addObject(mobj);
    return true;
}

opvector< opvector<UAVObject*> > UAVObjectManager::getObjects()
{
    CMutexSection locker(&m_mutex);
    return m_objects;
}

opvector< opvector<UAVDataObject*> > UAVObjectManager::getDataObjects()
{
    CMutexSection locker(&m_mutex);
    opvector< opvector<UAVDataObject*> > dObjects;
    
    // Go through objects and copy to new list when types match
    for (int objidx = 0; objidx < int (m_objects.size()); ++objidx)
    {
        if (m_objects[objidx].size() > 0)
        {
            // Check type
            UAVDataObject* obj = dynamic_cast<UAVDataObject*>(m_objects[objidx][0]);
            if (obj != NULL)
            {
                // Create instance list
                opvector<UAVDataObject*> list;
                // Go through instances and cast them to UAVDataObject, then add to list
                for (int instidx = 0; instidx < int (m_objects[objidx].size()); ++instidx)
                {
                    obj = dynamic_cast<UAVDataObject*>(m_objects[objidx][instidx]);
                    if (obj != NULL)
                    {
                        list.push_back(obj);
                    }
                }
                // Append to object list
                dObjects.push_back(list);
            }
        }
    }
    // Done
    return dObjects;
}

opvector< opvector<UAVMetaObject*> > UAVObjectManager::getMetaObjects()
{
    CMutexSection locker(&m_mutex);
    opvector< opvector<UAVMetaObject*> > mObjects;
    
    // Go through objects and copy to new list when types match
    for (int objidx = 0; objidx < int (m_objects.size()); ++objidx)
    {
        if (m_objects[objidx].size() > 0)
        {
            // Check type
            UAVMetaObject* obj = dynamic_cast<UAVMetaObject*>(m_objects[objidx][0]);
            if (obj != NULL)
            {
                // Create instance list
                opvector<UAVMetaObject*> list;
                // Go through instances and cast them to UAVMetaObject, then add to list
                for (int instidx = 0; instidx < int (m_objects[objidx].size()); ++instidx)
                {
                    obj = dynamic_cast<UAVMetaObject*>(m_objects[objidx][instidx]);
                    if (obj != NULL)
                    {
                        list.push_back(obj);
                    }
                }
                // Append to object list
                mObjects.push_back(list);
            }
        }
    }
    // Done
    return mObjects;
}

UAVObject* UAVObjectManager::getObject(const CString& name, opuint32 instId /*= 0*/)
{
    return getObject(&name, 0, instId);
}

UAVObject* UAVObjectManager::getObject(opuint32 objId, opuint32 instId /*= 0*/)
{
    return getObject(NULL, objId, instId);
}

opvector<UAVObject*> UAVObjectManager::getObjectInstances(const CString& name)
{
    return getObjectInstances(&name, 0);
}

opvector<UAVObject*> UAVObjectManager::getObjectInstances(opuint32 objId)
{
    return getObjectInstances(NULL, objId);
}

opint32 UAVObjectManager::getNumInstances(const CString& name)
{
    return getNumInstances(&name, 0);
}

opint32 UAVObjectManager::getNumInstances(opuint32 objId)
{
    return getNumInstances(NULL, objId);
}

/////////////////////////////////////////////////////////////////////
//                               signals                           //
/////////////////////////////////////////////////////////////////////

CL_Signal_v1<UAVObject*>& UAVObjectManager::signal_newObject()
{
    return m_signal_newObject;
}

CL_Signal_v1<UAVObject*>& UAVObjectManager::signal_newInstance()
{
    return m_signal_newInstance;
}

//===================================================================
//  p r i v a t e   f u n c t i o n s
//===================================================================

void UAVObjectManager::addObject(UAVObject* obj)
{
    // Add to list
    opvector<UAVObject*> list;
    list.push_back(obj);
    m_objects.push_back(list);
    m_signal_newObject.invoke(obj);
}

UAVObject* UAVObjectManager::getObject(const CString* name, opuint32 objId, opuint32 instId)
{
    CMutexSection locker(&m_mutex);
    // Check if this object type is already in the list
    for (int objidx = 0; objidx < int (m_objects.size()); ++objidx)
    {
        // Check if the object ID is in the list
        if (m_objects[objidx].size() > 0)
        {
            if ( (name != NULL && m_objects[objidx][0]->getName() == *name) || (name == NULL && m_objects[objidx][0]->getObjID() == objId) )
            {
                // Look for the requested instance ID
                for (int instidx = 0; instidx < int (m_objects[objidx].size()); ++instidx)
                {
                    if (m_objects[objidx][instidx]->getInstID() == instId)
                    {
                        return m_objects[objidx][instidx];
                    }
                }
            }
        }
    }
    Logger() << "UAVObjectManager::getObject: Object not found.  Probably a bug or mismatched GCS/flight versions.";
    // If this point is reached then the requested object could not be found
    return NULL;
}

opvector<UAVObject*> UAVObjectManager::getObjectInstances(const CString* name, opuint32 objId)
{
    CMutexSection locker(&m_mutex);
    // Check if this object type is already in the list
    for (int objidx = 0; objidx < int (m_objects.size()); ++objidx)
    {
        // Check if the object ID is in the list
        if (m_objects[objidx].size() > 0)
        {
            if ( (name != NULL && m_objects[objidx][0]->getName() == *name) || (name == NULL && m_objects[objidx][0]->getObjID() == objId) )
            {
                return m_objects[objidx];
            }
        }
    }
    // If this point is reached then the requested object could not be found
    return opvector<UAVObject*>();
}

opint32 UAVObjectManager::getNumInstances(const CString* name, opuint32 objId)
{
    CMutexSection locker(&m_mutex);
    // Check if this object type is already in the list
    for (int objidx = 0; objidx < int (m_objects.size()); ++objidx)
    {
        // Check if the object ID is in the list
        if (m_objects[objidx].size() > 0)
        {
            if ( (name != NULL && m_objects[objidx][0]->getName() == *name) || (name == NULL && m_objects[objidx][0]->getObjID() == objId) )
            {
                return opint32 (m_objects[objidx].size());
            }
        }
    }
    // If this point is reached then the requested object could not be found
    return -1;
}
