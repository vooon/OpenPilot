#include "telemetry.h"
#include "uavtalk.h"
#include "uavobjectmanager.h"
#include "uavmetaobject.h"
#include "io/CIODevice"
#include "system/CTime"
#include "Logger"

#include "oplinksettings.h"
#include "objectpersistence.h"

#include <assert.h>


ObjectTransactionInfo::ObjectTransactionInfo()
{
    obj = 0;
    allInstances = false;
    objRequest = false;
    retriesRemaining = 0;
    acked = false;
    telem = 0;

    m_startTime = CTime();
    m_waitTime  = 0;
    m_bStarted  = false;
}

ObjectTransactionInfo::~ObjectTransactionInfo()
{
    telem = 0;
}

void ObjectTransactionInfo::start(int waitTime)
{
    m_bStarted  = true;
    m_startTime = CTime();
    m_waitTime  = waitTime;
}

void ObjectTransactionInfo::stop()
{
    m_bStarted = false;
}

void ObjectTransactionInfo::onUpdate()
{
    if (!m_bStarted)
        return;
    CTime current;
    if (current-m_startTime >= m_waitTime) {
        m_startTime = current;
        timeout();
    }
}

void ObjectTransactionInfo::timeout()
{
    if (telem != 0)
        telem->transactionTimeout(this);
}




Telemetry::Telemetry(UAVTalk* talk, UAVObjectManager* manager)
{
    m_talk    = talk;
    m_manager = manager;

    opvector< opvector<UAVObject*> > objects = m_manager->getObjects();
    for (int objidx = 0; objidx < int (objects.size()); ++objidx) {
        registerObject(objects[objidx][0]); // we only need to register one instance per object type
    }
    // Listen to new object creations
    m_slotNewObject = m_manager->signal_newObject().connect(this, &Telemetry::newObject);
    m_slotNewInstance = m_manager->signal_newInstance().connect(this, &Telemetry::newInstance);
    // Listen to transaction completions
    m_slotTransactionCompleted = talk->signal_transactionComplete().connect(this, &Telemetry::transactionCompleted);
    // Get GCS stats object
    m_gcsStatsObj = GCSTelemetryStats::GetInstance(m_manager);
    // Setup and start the periodic timer
    m_timeToNextUpdateMs = 0;

    m_timerStarted = true;
    m_startTime    = CTime();
    m_waitTime     = 1000;
    // Setup and start the stats timer
    m_txErrors = 0;
    m_txRetries = 0;
}

Telemetry::~Telemetry()
{
    for (opmap<opuint32, ObjectTransactionInfo*>::iterator it = m_transMap.begin();it!=m_transMap.end();it++)
    {
        SAFE_DELETE((*it).second);
    }
}

Telemetry::TelemetryStats Telemetry::getStats() const
{
    CMutexSection locker(&m_mutex);

    // Get UAVTalk stats
    UAVTalk::ComStats utalkStats = m_talk->getStats();

    // Update stats
    TelemetryStats stats;
    stats.txBytes = utalkStats.txBytes;
    stats.rxBytes = utalkStats.rxBytes;
    stats.txObjectBytes = utalkStats.txObjectBytes;
    stats.rxObjectBytes = utalkStats.rxObjectBytes;
    stats.rxObjects = utalkStats.rxObjects;
    stats.txObjects = utalkStats.txObjects;
    stats.txErrors = utalkStats.txErrors + m_txErrors;
    stats.rxErrors = utalkStats.rxErrors;
    stats.txRetries = m_txRetries;

    // Done
    return stats;
}

void Telemetry::resetStats()
{
    CMutexSection locker(&m_mutex);
    m_talk->resetStats();
    m_txErrors = 0;
    m_txRetries = 0;
}

/////////////////////////////////////////////////////////////////////
//                               notifies                          //
/////////////////////////////////////////////////////////////////////

void Telemetry::onUpdate(float /*timeElapsed*/)
{
    if (m_timerStarted)
    {
        CTime current = CTime();
        if (current-m_startTime >= m_waitTime)
        {
            m_startTime = current;
            processPeriodicUpdates();
        }
    }

    m_bTransMapChanged = false;
    for (opmap<opuint32, ObjectTransactionInfo*>::iterator it = m_transMap.begin();it!=m_transMap.end();it++) {
        if (m_bTransMapChanged)
            break;
        if ((*it).second) {
            (*it).second->onUpdate();
        }
        if (m_bTransMapChanged)
            break;
    }
}

void Telemetry::transactionTimeout(ObjectTransactionInfo *info)
{
    info->stop();
    // Check if more retries are pending
    if (info->retriesRemaining > 0)
    {
        --info->retriesRemaining;
        processObjectTransaction(info);
        ++m_txRetries;
    }
    else
    {
        // Stop the timer.
        info->stop();
        // Terminate transaction
        m_talk->cancelTransaction(info->obj);
        // Send signal
        info->obj->emitTransactionCompleted(false);
        // Remove this transaction as it's complete.
        m_bTransMapChanged = true;
        m_transMap.erase(info->obj->getObjID());
        delete info;
        // Process new object updates from queue
        processObjectQueue();
        ++m_txErrors;
    }
}

//===================================================================
//  p r i v a t e   f u n c t i o n s
//===================================================================

void Telemetry::registerObject(UAVObject* obj)
{
    // Setup object for periodic updates
    addObject(obj);

    // Setup object for telemetry updates
    updateObject(obj, EV_NONE);
}

void Telemetry::addObject(UAVObject* obj)
{
    if (!obj)
        return;
    // Check if object type is already in the list
    for (int n = 0; n < int (m_objList.size()); ++n)
    {
        if (m_objList[n].obj->getObjID() == obj->getObjID() )
        {
            // Object type (not instance!) is already in the list, do nothing
            return;
        }
    }

    // If this point is reached, then the object type is new, let's add it
    ObjectTimeInfo timeInfo;
    timeInfo.obj = obj;
    timeInfo.timeToNextUpdateMs = 0;
    timeInfo.updatePeriodMs = 0;
    m_objList.push_back(timeInfo);
}

void Telemetry::updateObject(UAVObject* obj, opuint32 eventType)
{
    // Get metadata
    UAVObject::Metadata metadata = obj->getMetadata();
    UAVObject::UpdateMode updateMode = UAVObject::GetGcsTelemetryUpdateMode(metadata);

    // Setup object depending on update mode
    opint32 eventMask;
    if (updateMode == UAVObject::UPDATEMODE_PERIODIC)
    {
        // Set update period
        setUpdatePeriod(obj, metadata.gcsTelemetryUpdatePeriod);
        // Connect signals for all instances
        eventMask = EV_UPDATED_MANUAL | EV_UPDATE_REQ | EV_UPDATED_PERIODIC;
        if (dynamic_cast<UAVMetaObject*>(obj) != NULL)
        {
            eventMask |= EV_UNPACKED; // we also need to act on remote updates (unpack events)
        }
        connectToObjectInstances(obj, eventMask);
    }
    else if ( updateMode == UAVObject::UPDATEMODE_ONCHANGE )
    {
        // Set update period
        setUpdatePeriod(obj, 0);
        // Connect signals for all instances
        eventMask = EV_UPDATED | EV_UPDATED_MANUAL | EV_UPDATE_REQ;
        if (dynamic_cast<UAVMetaObject*>(obj) != NULL)
        {
            eventMask |= EV_UNPACKED; // we also need to act on remote updates (unpack events)
        }
        connectToObjectInstances(obj, eventMask);
    }
    else if ( updateMode == UAVObject::UPDATEMODE_THROTTLED )
    {
        // If we received a periodic update, we can change back to update on change
        if ((eventType == EV_UPDATED_PERIODIC) || (eventType == EV_NONE))
        {
            // Set update period
            if (eventType == EV_NONE) {
                 setUpdatePeriod(obj, metadata.gcsTelemetryUpdatePeriod);
            }
            // Connect signals for all instances
            eventMask = EV_UPDATED | EV_UPDATED_MANUAL | EV_UPDATE_REQ | EV_UPDATED_PERIODIC;
        }
        else
        {
            // Otherwise, we just received an object update, so switch to periodic for the timeout period to prevent more updates
            // Connect signals for all instances
            eventMask = EV_UPDATED | EV_UPDATED_MANUAL | EV_UPDATE_REQ;
        }
        if(dynamic_cast<UAVMetaObject*>(obj) != NULL) {
           eventMask |= EV_UNPACKED; // we also need to act on remote updates (unpack events)
        }
        connectToObjectInstances(obj, eventMask);
    }
    else if (updateMode == UAVObject::UPDATEMODE_MANUAL)
    {
        // Set update period
        setUpdatePeriod(obj, 0);
        // Connect signals for all instances
        eventMask = EV_UPDATED_MANUAL | EV_UPDATE_REQ;
        if( dynamic_cast<UAVMetaObject*>(obj) != NULL )
        {
            eventMask |= EV_UNPACKED; // we also need to act on remote updates (unpack events)
        }
        connectToObjectInstances(obj, eventMask);
    }
}

void Telemetry::setUpdatePeriod(UAVObject* obj, opint32 periodMs)
{
    // Find object type (not instance!) and update its period
    for (opvector<ObjectTimeInfo>::iterator it = m_objList.begin();it<m_objList.end();it++)
    {
        if ((*it).obj->getObjID() == obj->getObjID() )
        {
            (*it).updatePeriodMs = periodMs;
            (*it).timeToNextUpdateMs = opuint32((float)periodMs * (float)rand() / (float)RAND_MAX); // avoid bunching of updates
        }
    }
}

void Telemetry::connectToObjectInstances(UAVObject* obj, opuint32 eventMask)
{
    opvector<UAVObject*> objs = m_manager->getObjectInstances(obj->getObjID());
    opmap<UAVObject*, OBJECTCONNECT>::iterator it_conn;
    for (opvector<UAVObject*>::iterator it= objs.begin();it<objs.end();it++)
    {
        // Disconnect all
        if (m_connectedObjs.count((*it)) != 0)
        {
            it_conn = m_connectedObjs.find(*it);
            (*it_conn).second.slotObjectUnpacked.destroy();
            (*it_conn).second.slotUpdatedAuto.destroy();
            (*it_conn).second.slotUpdatedManual.destroy();
            (*it_conn).second.slotUpdatedPeriodic.destroy();
            (*it_conn).second.slotUpdateRequest.destroy();
        }
        else
        {
            OBJECTCONNECT objConnect;
            objConnect.obj = *it;
            m_connectedObjs[*it] = objConnect;
        }
        it_conn = m_connectedObjs.find(*it);
        if (it_conn == m_connectedObjs.end())
        {
            assert(0 == "it_conn == 0");
            continue;
        }
        // Connect only the selected events
        if ( (eventMask&EV_UNPACKED) != 0)
        {
            (*it_conn).second.slotObjectUnpacked = (*it)->signal_objectUnpacked().connect(this, &Telemetry::objectUnpacked);
        }
        if ( (eventMask&EV_UPDATED) != 0)
        {
            (*it_conn).second.slotUpdatedAuto = (*it)->signal_objectUpdatedAuto().connect(this, &Telemetry::objectUpdatedAuto);
        }
        if ( (eventMask&EV_UPDATED_MANUAL) != 0)
        {
            (*it_conn).second.slotUpdatedManual = (*it)->signal_objectUpdatedManual().connect(this, &Telemetry::objectUpdatedManual);
        }
        if ( (eventMask&EV_UPDATED_PERIODIC) != 0)
        {
            (*it_conn).second.slotUpdatedPeriodic = (*it)->signal_objectUpdatedPeriodic().connect(this, &Telemetry::objectUpdatedPeriodic);
        }
        if ( (eventMask&EV_UPDATE_REQ) != 0)
        {
            (*it_conn).second.slotUpdateRequest = (*it)->signal_updateRequested().connect(this, &Telemetry::updateRequested);
        }
    }
}

void Telemetry::processPeriodicUpdates()
{
    CMutexSection locker(&m_mutex);

    // Stop timer
    m_timerStarted = false;

    // Iterate through each object and update its timer, if zero then transmit object.
    // Also calculate smallest delay to next update (will be used for setting timeToNextUpdateMs)
    opint32 minDelay = MAX_UPDATE_PERIOD_MS;
    ObjectTimeInfo *objinfo;
    opint32 elapsedMs = 0;
    CTime time;
    opint32 offset;
    for (opvector<ObjectTimeInfo>::iterator it = m_objList.begin();it<m_objList.end();it++)
    {
        objinfo = &(*it);
        // If object is configured for periodic updates
        if (objinfo->updatePeriodMs > 0)
        {
            objinfo->timeToNextUpdateMs -= m_timeToNextUpdateMs;
            // Check if time for the next update
            if (objinfo->timeToNextUpdateMs <= 0)
            {
                // Reset timer
                offset = (-objinfo->timeToNextUpdateMs) % objinfo->updatePeriodMs;
                objinfo->timeToNextUpdateMs = objinfo->updatePeriodMs - offset;
                // Send object
                time.start();
                processObjectUpdates(objinfo->obj, EV_UPDATED_PERIODIC, true, false);
                elapsedMs = time.elapsed();
                // Update timeToNextUpdateMs with the elapsed delay of sending the object;
                m_timeToNextUpdateMs += elapsedMs;
            }
            // Update minimum delay
            if (objinfo->timeToNextUpdateMs < minDelay)
            {
                minDelay = objinfo->timeToNextUpdateMs;
            }
        }
    }

    // Check if delay for the next update is too short
    if (minDelay < MIN_UPDATE_PERIOD_MS)
    {
        minDelay = MIN_UPDATE_PERIOD_MS;
    }

    // Done
    m_timeToNextUpdateMs = minDelay;

    // Restart timer
    m_timerStarted = true;
    m_startTime    = CTime();
    m_waitTime     = m_timeToNextUpdateMs;
}

void Telemetry::processObjectUpdates(UAVObject* obj, EventMask event, bool allInstances, bool priority)
{
    // Push event into queue
    ObjectQueueInfo objInfo;
    objInfo.obj = obj;
    objInfo.event = event;
    objInfo.allInstances = allInstances;
    if (priority)
    {
        if (int(m_objPriorityQueue.size()) < MAX_QUEUE_SIZE)
        {
            m_objPriorityQueue.push_back(objInfo);
        }
        else
        {
            ++m_txErrors;
            obj->emitTransactionCompleted(false);
            Logger() << "Telemetry: priority event queue is full, event lost (" << obj->getName() << ")";
        }
    }
    else
    {
        if (int (m_objQueue.size()) < MAX_QUEUE_SIZE)
        {
            m_objQueue.push_back(objInfo);
        }
        else
        {
            ++m_txErrors;
            obj->emitTransactionCompleted(false);
        }
    }

    // Process the transaction
    processObjectQueue();
}

/**
 * Process events from the object queue
 */
void Telemetry::processObjectQueue()
{
    // Get object information from queue (first the priority and then the regular queue)
    ObjectQueueInfo objInfo;
    if (!m_objPriorityQueue.empty()) {
        objInfo = *m_objPriorityQueue.begin();
        m_objPriorityQueue.pop_front();
    } else if (!m_objQueue.empty()) {
        objInfo = *m_objQueue.begin();
        m_objQueue.pop_front();
    } else {
        return;
    }

    // Check if a connection has been established, only process GCSTelemetryStats updates
    // (used to establish the connection)
    GCSTelemetryStats::DataFields gcsStats = m_gcsStatsObj->getData();
    if ( gcsStats.Status != GCSTelemetryStats::STATUS_CONNECTED )
    {
        m_objQueue.clear();
        if (objInfo.obj->getObjID() != GCSTelemetryStats::OBJID && objInfo.obj->getObjID() != OPLinkSettings::OBJID  && objInfo.obj->getObjID() != ObjectPersistence::OBJID )
        {
            objInfo.obj->emitTransactionCompleted(false);
            return;
        }
    }

    // Setup transaction (skip if unpack event)
    UAVObject::Metadata metadata = objInfo.obj->getMetadata();
    UAVObject::UpdateMode updateMode = UAVObject::GetGcsTelemetryUpdateMode(metadata);
    if ( (objInfo.event != EV_UNPACKED) && ( (objInfo.event != EV_UPDATED_PERIODIC) || (updateMode != UAVObject::UPDATEMODE_THROTTLED) ))
    {
        if (m_transMap.count(objInfo.obj->getObjID()) ) {
            Logger() << "!!!!!! Making request for an object: " << objInfo.obj->getName() << " for which a request is already in progress!!!!!!";
        }
        UAVObject::Metadata metadata = objInfo.obj->getMetadata();
        ObjectTransactionInfo *transInfo = new ObjectTransactionInfo();
        transInfo->obj = objInfo.obj;
        transInfo->allInstances = objInfo.allInstances;
        transInfo->retriesRemaining = MAX_RETRIES;
        transInfo->acked = UAVObject::GetGcsTelemetryAcked(metadata);
        if (objInfo.event == EV_UPDATED || objInfo.event == EV_UPDATED_MANUAL || objInfo.event == EV_UPDATED_PERIODIC)
        {
            transInfo->objRequest = false;
        }
        else if ( objInfo.event == EV_UPDATE_REQ )
        {
            transInfo->objRequest = true;
        }
        transInfo->telem = this;
        // Insert the transaction into the transaction map.
        m_bTransMapChanged = true;
        m_transMap.insert(std::pair<opuint32, ObjectTransactionInfo*>(objInfo.obj->getObjID(), transInfo));
        processObjectTransaction(transInfo);
    }

    // If this is a metaobject then make necessary telemetry updates
    UAVMetaObject* metaobj = dynamic_cast<UAVMetaObject*>(objInfo.obj);
    if (metaobj != NULL)
    {
        updateObject( metaobj->getParentObject(), EV_NONE );
    }
    else if (updateMode != UAVObject::UPDATEMODE_THROTTLED)
    {
        updateObject(objInfo.obj, objInfo.event);
    }

    // The fact we received an unpacked event does not mean that
    // we do not have additional objects still in the queue,
    // so we have to reschedule queue processing to make sure they are not
    // stuck:
    if (objInfo.event == EV_UNPACKED)
        processObjectQueue();
}

void Telemetry::processObjectTransaction(ObjectTransactionInfo *transInfo)
{
    // Initiate transaction
    if (transInfo->objRequest)
    {
        m_talk->sendObjectRequest(transInfo->obj, transInfo->allInstances);
    }
    else
    {
        m_talk->sendObject(transInfo->obj, transInfo->acked, transInfo->allInstances);
    }
    // Start timer if a response is expected
    if (transInfo->objRequest || transInfo->acked)
    {
        transInfo->start(REQ_TIMEOUT_MS);
    }
    else
    {
        // Otherwise, remove this transaction as it's complete.
        m_bTransMapChanged = true;
        m_transMap.erase(transInfo->obj->getObjID());
        delete transInfo;
    }
}

//-------------------------------------------------------------------
//  s l o t s
//-------------------------------------------------------------------

void Telemetry::objectUnpacked(UAVObject* object)
{
    CMutexSection locker(&m_mutex);
    processObjectUpdates(object, EV_UNPACKED, false, true);
}

void Telemetry::objectUpdatedAuto(UAVObject* object)
{
    CMutexSection locker(&m_mutex);
    processObjectUpdates(object, EV_UPDATED, false, true);
}

void Telemetry::objectUpdatedManual(UAVObject* object)
{
    CMutexSection locker(&m_mutex);
    processObjectUpdates(object, EV_UPDATED_MANUAL, false, true);
}

void Telemetry::objectUpdatedPeriodic(UAVObject* object)
{
    CMutexSection locker(&m_mutex);
    processObjectUpdates(object, EV_UPDATED_PERIODIC, false, true);
}

void Telemetry::updateRequested(UAVObject* object)
{
    CMutexSection locker(&m_mutex);
    processObjectUpdates(object, EV_UPDATE_REQ, false, true);
}

void Telemetry::newObject(UAVObject* object)
{
    CMutexSection locker(&m_mutex);
    registerObject(object);
}

void Telemetry::newInstance(UAVObject* object)
{
    CMutexSection locker(&m_mutex);
    registerObject(object);
}

void Telemetry::transactionCompleted(UAVObject* object, bool sucess)
{
    // Lookup the transaction in the transaction map.
    opuint32 objId = object->getObjID();
    opmap<opuint32, ObjectTransactionInfo*>::iterator itr = m_transMap.find(objId);
    if (itr != m_transMap.end())
    {
        ObjectTransactionInfo *transInfo = (*itr).second;
        // Remove this transaction as it's complete.
        m_bTransMapChanged = true;
        m_transMap.erase(objId);
        delete transInfo;
        // Send signal
        object->emitTransactionCompleted(sucess);
        // Process new object updates from queue
        processObjectQueue();
    } else
    {
        Logger() << "Error: received a transaction completed when did not expect it.";
    }
}
