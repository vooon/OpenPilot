#include "telemetrymonitor.h"
#include "uavobjectmanager.h"
#include "uavmetaobject.h"
#include "uavdataobject.h"
#include "telemetry.h"
#include "Logger"

TelemetryMonitor::TelemetryMonitor(UAVObjectManager* manager, Telemetry* telemetry)
{
    m_manager    = manager;
    m_telemetry  = telemetry;
    m_objPending = 0;

    m_gcsStatsObj    = GCSTelemetryStats::GetInstance(manager);
    m_flightStatsObj = FlightTelemetryStats::GetInstance(manager);

    if (m_flightStatsObj) {
        m_slotFlightStatsUpdated = m_flightStatsObj->signal_objectUpdated().connect(this, &TelemetryMonitor::flightStatsUpdated);
    }

    m_updatesStarted = false;
    m_startTime      = CTime();
    m_updatesInerval = STATS_CONNECT_PERIOD_MS;
}

TelemetryMonitor::~TelemetryMonitor()
{
}

void TelemetryMonitor::onUpdate()
{
    if (m_updatesStarted)
    {
        CTime current;
        if (current-m_startTime >= m_updatesInerval)
        {
            processStatsUpdates();
            m_startTime = CTime();
        }
    }
}

/////////////////////////////////////////////////////////////////////
//                              signals                            //
/////////////////////////////////////////////////////////////////////

CL_Signal_v2<double, double>& TelemetryMonitor::signal_telemetryUpdated()
{
    return m_signalTelemetryUpdated;
}

//===================================================================
//  p r i v a t e   f u n c t i o n s
//===================================================================

/**
 * Called periodically to update the statistics and connection status.
 */
void TelemetryMonitor::processStatsUpdates()
{
    CMutexSection locker(&m_mutex);

    // Get telemetry stats
    GCSTelemetryStats::DataFields gcsStats = m_gcsStatsObj->getData();
    FlightTelemetryStats::DataFields flightStats = m_flightStatsObj->getData();
    Telemetry::TelemetryStats telStats = m_telemetry->getStats();
    m_telemetry->resetStats();

    // Update stats object
    gcsStats.RxDataRate = (float)telStats.rxBytes / ((float)m_updatesInerval/1000.0);
    gcsStats.TxDataRate = (float)telStats.txBytes / ((float)m_updatesInerval/1000.0);
    gcsStats.RxFailures += telStats.rxErrors;
    gcsStats.TxFailures += telStats.txErrors;
    gcsStats.TxRetries += telStats.txRetries;

    // Check for a connection timeout
    bool connectionTimeout;
    if (telStats.rxObjects > 0)
    {
        m_connectionTimer.start();
    }
    if (m_connectionTimer.elapsed() > CONNECTION_TIMEOUT_MS)
    {
        connectionTimeout = true;
    }
    else
    {
        connectionTimeout = false;
    }

    // Update connection state
    int oldStatus = gcsStats.Status;
    if (gcsStats.Status == GCSTelemetryStats::STATUS_DISCONNECTED)
    {
        // Request connection
        gcsStats.Status = GCSTelemetryStats::STATUS_HANDSHAKEREQ;
    }
    else if (gcsStats.Status == GCSTelemetryStats::STATUS_HANDSHAKEREQ)
    {
        // Check for connection acknowledge
        if (flightStats.Status == FlightTelemetryStats::STATUS_HANDSHAKEACK)
        {
            gcsStats.Status = GCSTelemetryStats::STATUS_CONNECTED;
        }
    }
    else if (gcsStats.Status == GCSTelemetryStats::STATUS_CONNECTED)
    {
        // Check if the connection is still active and the the autopilot is still connected
        if (flightStats.Status == FlightTelemetryStats::STATUS_DISCONNECTED || connectionTimeout)
        {
            gcsStats.Status = GCSTelemetryStats::STATUS_DISCONNECTED;
        }
    }

//    Logger() << "txDataRate = " << (double)gcsStats.TxDataRate << ", rxDataRate = " << (double)gcsStats.RxDataRate;
    m_signalTelemetryUpdated.invoke((double)gcsStats.TxDataRate, (double)gcsStats.RxDataRate);
//    emit telemetryUpdated((double)gcsStats.TxDataRate, (double)gcsStats.RxDataRate);

    // Set data
    m_gcsStatsObj->setData(gcsStats);

    // Force telemetry update if not yet connected
    if ( gcsStats.Status != GCSTelemetryStats::STATUS_CONNECTED ||
         flightStats.Status != FlightTelemetryStats::STATUS_CONNECTED )
    {
        m_gcsStatsObj->updated();
    }

    // Act on new connections or disconnections
    if (gcsStats.Status == GCSTelemetryStats::STATUS_CONNECTED && gcsStats.Status != oldStatus)
    {
        m_updatesInerval = STATS_UPDATE_PERIOD_MS;
        Logger() << "Connection with the autopilot established";
        startRetrievingObjects();
    }
    if (gcsStats.Status == GCSTelemetryStats::STATUS_DISCONNECTED && gcsStats.Status != oldStatus)
    {
        m_updatesInerval = STATS_CONNECT_PERIOD_MS;
        Logger() << "Connection with the autopilot lost";
        Logger() << "Trying to connect to the autopilot";
        m_signalDisconnected.invoke();
    }
}

void TelemetryMonitor::startRetrievingObjects()
{
    // Clear object queue
    m_queue.clear();
    // Get all objects, add metaobjects, settings and data objects with OnChange update mode to the queue
    opvector<opvector<UAVObject*> > objs = m_manager->getObjects();
    for (int n = 0; n < int (objs.size()); ++n)
    {
        UAVObject* obj = objs[n][0];
        UAVMetaObject* mobj = dynamic_cast<UAVMetaObject*>(obj);
        UAVDataObject* dobj = dynamic_cast<UAVDataObject*>(obj);
        UAVObject::Metadata mdata = obj->getMetadata();
        if (mobj != NULL)
        {
            m_queue.push_back(obj);
        }
        else if (dobj != NULL)
        {
            if (dobj->isSettings())
            {
                m_queue.push_back(obj);
            }
            else
            {
                if (UAVObject::GetFlightTelemetryUpdateMode(mdata) == UAVObject::UPDATEMODE_ONCHANGE)
                {
                    m_queue.push_back(obj);
                }
            }
        }
    }
    // Start retrieving
    Logger() << CString("Starting to retrieve meta and settings objects from the autopilot (%1 objects)").arg(int (m_queue.size()));
    retrieveNextObject();
}

void TelemetryMonitor::stopRetrievingObjects()
{
    Logger() << "Object retrieval has been cancelled";
    m_queue.clear();
}

void TelemetryMonitor::retrieveNextObject()
{
    // If queue is empty return
    if (m_queue.empty())
    {
        Logger() << "Object retrieval completed";
        m_signalConnected.invoke();
        return;
    }
    // Get next object from the queue
    UAVObject* obj = *m_queue.begin();
    m_queue.pop_front();
    //qxtLog->trace( tr("Retrieving object: %1").arg(obj->getName()) );
    // Connect to object
    m_slotTransactionCompleted = obj->signal_transactionCompleted().connect(this, &TelemetryMonitor::transactionCompleted);
    // Request update
    obj->requestUpdate();
    m_objPending = obj;
}

//-------------------------------------------------------------------
//  n o t i f y
//-------------------------------------------------------------------

void TelemetryMonitor::flightStatsUpdated(UAVObject* /*obj*/)
{
//    Q_UNUSED(obj);
    CMutexSection locker(&m_mutex);

    // Force update if not yet connected
    GCSTelemetryStats::DataFields gcsStats = m_gcsStatsObj->getData();
    FlightTelemetryStats::DataFields flightStats = m_flightStatsObj->getData();
    if (gcsStats.Status != GCSTelemetryStats::STATUS_CONNECTED ||
         flightStats.Status != FlightTelemetryStats::STATUS_CONNECTED)
    {
        processStatsUpdates();
    }
}

void TelemetryMonitor::transactionCompleted(UAVObject*, bool /*success*/)
{
//    Q_UNUSED(success);
    CMutexSection locker(&m_mutex);
    // Disconnect from sending object
    m_slotTransactionCompleted.destroy();
    m_objPending = NULL;
    // Process next object if telemetry is still available
    GCSTelemetryStats::DataFields gcsStats = m_gcsStatsObj->getData();
    if (gcsStats.Status == GCSTelemetryStats::STATUS_CONNECTED)
    {
        retrieveNextObject();
    }
    else
    {
        stopRetrievingObjects();
    }
}
