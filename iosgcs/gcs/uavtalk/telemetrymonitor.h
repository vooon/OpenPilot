#ifndef TELEMETRYMONITOR_H
#define TELEMETRYMONITOR_H

#include "thread/CMutex"
#include "OPList"
#include "signals/Signals"
#include "system/CTime"
#include "gcstelemetrystats.h"
#include "flighttelemetrystats.h"

class UAVObject;
class UAVObjectManager;
class Telemetry;

class TelemetryMonitor
{
private:
    static const int STATS_UPDATE_PERIOD_MS = 4000;
    static const int STATS_CONNECT_PERIOD_MS = 2000;
    static const int CONNECTION_TIMEOUT_MS = 8000;

public:
    TelemetryMonitor(UAVObjectManager* manager, Telemetry* telemetry);
    ~TelemetryMonitor();

    void onUpdate();

// signals
    CL_Signal_v2<double, double>& signal_telemetryUpdated();
    
private:
    UAVObjectManager     * m_manager;
    Telemetry            * m_telemetry;
    UAVObject            * m_objPending;
    GCSTelemetryStats    * m_gcsStatsObj;
    FlightTelemetryStats * m_flightStatsObj;
    oplist<UAVObject*>     m_queue;
    CTime                  m_connectionTimer;
    CMutex                 m_mutex;

    bool  m_updatesStarted;
    CTime m_startTime;
    int   m_updatesInerval;

    CL_Slot m_slotFlightStatsUpdated;
    CL_Slot m_slotTransactionCompleted;

    CL_Signal_v0 m_signalConnected;
    CL_Signal_v0 m_signalDisconnected;
    CL_Signal_v2<double, double> m_signalTelemetryUpdated;

    void processStatsUpdates();
    void startRetrievingObjects();
    void stopRetrievingObjects();
    void retrieveNextObject();

// notify
    void flightStatsUpdated(UAVObject* obj);
    void transactionCompleted(UAVObject*, bool success);
};

#endif // TELEMETRYMONITOR_H
