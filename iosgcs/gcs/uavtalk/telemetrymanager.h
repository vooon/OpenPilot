#ifndef TELEMETRYMANAGER_H
#define TELEMETRYMANAGER_H

class CIODevice;
class UAVObjectManager;
class UAVTalk;
class Telemetry;
class TelemetryMonitor;

class TelemetryManager
{
public:
    TelemetryManager(UAVObjectManager* manager);
    ~TelemetryManager();

    void start(CIODevice* device);
    void stop();
    bool isConnected() const;
    CIODevice* ioDevice() const;
    TelemetryMonitor* monitor() const;

    void onUpdate();

private:
    UAVObjectManager * m_manager;
    CIODevice        * m_ioDevice;
    UAVTalk          * m_uavTalk;
    Telemetry        * m_telemetry;
    TelemetryMonitor * m_monitor;
    bool               m_bConnected;
};

#endif // TELEMETRYMANAGER_H
