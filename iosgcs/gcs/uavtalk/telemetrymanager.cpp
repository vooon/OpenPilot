#include "telemetrymanager.h"
#include "uavtalk.h"
#include "telemetry.h"
#include "telemetrymonitor.h"

TelemetryManager::TelemetryManager(UAVObjectManager* manager)
{
    m_manager    = manager;
    m_bConnected = false;
    m_ioDevice   = 0;
    m_uavTalk    = 0;
    m_telemetry  = 0;
}

TelemetryManager::~TelemetryManager()
{
    stop();
}

void TelemetryManager::start(CIODevice* device)
{
    if (!device)
        return;
    m_ioDevice = device;
    m_uavTalk = new UAVTalk(m_ioDevice, m_manager);
    m_telemetry = new Telemetry(m_uavTalk, m_manager);
    m_monitor = new TelemetryMonitor(m_manager, m_telemetry);
}

void TelemetryManager::stop()
{
    SAFE_DELETE(m_monitor);
    SAFE_DELETE(m_telemetry);
    SAFE_DELETE(m_uavTalk);
    m_ioDevice = 0;
}

bool TelemetryManager::isConnected() const
{
    return m_bConnected;
}

CIODevice* TelemetryManager::ioDevice() const
{
    return m_ioDevice;
}

TelemetryMonitor* TelemetryManager::monitor() const
{
    return m_monitor;
}

void TelemetryManager::onUpdate()
{
    if (m_monitor) {
        m_monitor->onUpdate();
    }
    if (m_telemetry)
    {
        m_telemetry->onUpdate(0.0);
    }
}
