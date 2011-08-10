/**
 ******************************************************************************
 *
 * @file       simulator.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup HITLPlugin HITL Plugin
 * @{
 * @brief The Hardware In The Loop plugin
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

#include "simulator.h"
#include "qxtlogger.h"
#include "extensionsystem/pluginmanager.h"
#include "coreplugin/icore.h"
#include "coreplugin/threadmanager.h"

volatile bool Simulator::isStarted = false;

const float Simulator::GEE = 9.81;
const float Simulator::FT2M = 0.3048;
const float Simulator::KT2MPS = 0.514444444;
const float Simulator::INHG2KPA = 3.386;
const float Simulator::FPS2CMPS = 30.48;
const float Simulator::DEG2RAD = (M_PI/180.0);
const float Simulator::RAD2DEG = (180.0 / M_PI);


Simulator::Simulator(const SimulatorSettings& params) :
    simProcess(NULL),
    time(NULL),
    inSocket(NULL),
    outSocket(NULL),
    outSocketTcp(NULL),
    settings(params),
    simTimeout(2000),
    autopilotConnectionStatus(false),
    simConnectionStatus(false),
    txTimer(NULL),
    simTimer(NULL),
    name("")
{
    //    qDebug() << "Simulator constructed";
    moveToThread(Core::ICore::instance()->threadManager()->getRealTimeThread());
    connect(this, SIGNAL(myStart()), this, SLOT(onStart()),Qt::QueuedConnection);
    emit myStart();
}

Simulator::~Simulator()
{
    //    qDebug() << "Simulator DEconstructed";
    if (inSocket) {
        delete inSocket;
        inSocket = NULL;
    }
    if (outSocket) {
        delete outSocket;
        outSocket = NULL;
    }
    if (outSocketTcp) {
        delete outSocketTcp;
        outSocketTcp = NULL;
    }
    if (txTimer) {
        delete txTimer;
        txTimer = NULL;
    }
    if (simTimer) {
        delete simTimer;
        simTimer = NULL;
    }

    // NOTE: Does not currently work, may need to send control+c to through the terminal
    if (simProcess != NULL){
        simProcess->disconnect();
        if(simProcess->state() == QProcess::Running)
            simProcess->kill();
        delete simProcess;
        simProcess = NULL;
    }

    // reset all objects
    setupDefaultObject(actDesired);
    setupDefaultObject(attRaw);
    setupDefaultObject(attActual);
    setupDefaultObject(baroAlt);
    setupDefaultObject(gpsPos);
    setupDefaultObject(posActual);
    setupDefaultObject(velActual);
    setupDefaultObject(posHome);
}

void Simulator::onDeleteSimulator(void)
{
    //    qDebug() << "Simulator::onDeleteSimulator";
    // [1]
    Simulator::setStarted(false);
    // [2]
    Simulator::Instances().removeOne(simulatorId);
    
    disconnect(this);
    delete this;
}

void Simulator::onStart()
{
    QMutexLocker locker(&lock);

    QThread* mainThread = QThread::currentThread();
    qDebug() << "Simulator Thread: "<< mainThread;

    // Get required UAVObjects
    ExtensionSystem::PluginManager* pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager* objManager = pm->getObject<UAVObjectManager>();
    actDesired = ActuatorDesired::GetInstance(objManager);
    attRaw = AttitudeRaw::GetInstance(objManager);
    attActual = AttitudeActual::GetInstance(objManager);
    baroAlt = BaroAltitude::GetInstance(objManager);
    gpsPos = GPSPosition::GetInstance(objManager);
    posActual = PositionActual::GetInstance(objManager);
    velActual = VelocityActual::GetInstance(objManager);
    posHome = HomeLocation::GetInstance(objManager);
    manCtrlCommand = ManualControlCommand::GetInstance(objManager);
    flightStatus = FlightStatus::GetInstance(objManager);
    telStats = GCSTelemetryStats::GetInstance(objManager);

    // Listen to autopilot connection events
    TelemetryManager* telMngr = pm->getObject<TelemetryManager>();
    connect(telMngr, SIGNAL(connected()), this, SLOT(onAutopilotConnect()));
    connect(telMngr, SIGNAL(disconnected()), this, SLOT(onAutopilotDisconnect()));

    // If already connect setup autopilot
    GCSTelemetryStats::DataFields stats = telStats->getData();
    if ( stats.Status == GCSTelemetryStats::STATUS_CONNECTED )
        onAutopilotConnect();

    if (settings.isUdp) {
        inSocket = new QUdpSocket();
        outSocket = new QUdpSocket();
        setupUdpPorts(settings.hostAddress,settings.inPort,settings.outPort);
        connect(inSocket, SIGNAL(readyRead()), this, SLOT(receiveUpdateUdp()), Qt::DirectConnection);
    } else {
        outSocketTcp = new QTcpSocket();
        setupTcpPorts(settings.hostAddress, settings.inPort, settings.outPort);
        connect(outSocketTcp, SIGNAL(readyRead()), this, SLOT(receiveUpdateTcp()), Qt::DirectConnection);
        connect(outSocketTcp, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(displayError(QAbstractSocket::SocketError)));
    }

    emit processOutput("\nLocal interface: " + settings.hostAddress + "\n" + \
                       "Remote interface: " + settings.remoteHostAddress + "\n" + \
                       "inputPort: " + QString::number(settings.inPort) + "\n" + \
                       "outputPort: " + QString::number(settings.outPort) + "\n");

    // Setup transmit timer
    txTimer = new QTimer();
    connect(txTimer, SIGNAL(timeout()), this, SLOT(transmitUpdate()),Qt::DirectConnection);
    txTimer->setInterval(qRound(1000 / settings.actDesRate));
    txTimer->start();

    // Setup simulator connection timer
    simTimer = new QTimer();
    connect(simTimer, SIGNAL(timeout()), this, SLOT(onSimulatorConnectionTimeout()),Qt::DirectConnection);
    simTimer->setInterval(simTimeout);
    simTimer->start();

    // setup time
    time = new QTime();
    time->start();
    current.T=0;
    current.i=0;
}

void Simulator::receiveUpdateUdp()
{
    // Update connection timer and status
    //    simTimer->setInterval(simTimeout);
    simTimer->stop();
    simTimer->start();
    if (!simConnectionStatus){
        simConnectionStatus = true;
        emit simulatorConnected();
    }
    // Process data
    while(inSocket->hasPendingDatagrams()) {
        // Receive datagram
        QByteArray datagram;
        datagram.resize(inSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        inSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        // Process incomming data
        processUpdate(datagram);
    }
}

void Simulator::receiveUpdateTcp()
{
    // Update connection timer and status
    //    simTimer->setInterval(simTimeout);
    simTimer->stop();
    simTimer->start();
    if (!simConnectionStatus) {
        simConnectionStatus = true;
        emit simulatorConnected();
    }
    // Process data
    QByteArray telemetry = outSocketTcp->readAll();
    processUpdate(telemetry);
}

void Simulator::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
        case QAbstractSocket::RemoteHostClosedError:
            qDebug() << "SocketError: Remote host closed.";
            break;
        case QAbstractSocket::HostNotFoundError:
            qDebug() << "SocketError: The host was not found.";
            break;
        case QAbstractSocket::ConnectionRefusedError:
            qDebug() << "SocketError: The connection was refused by the peer.";
            break;
        default:
            qDebug() << "SocketError: The following error occurred: " << outSocketTcp->errorString();
    }
}

void Simulator::setupObjects()
{
    if (settings.isActDes) {
        setupInputObject(actDesired, qRound(1000/settings.actDesRate)); // Hz -> ms
    } else {
        setupDefaultObject(actDesired);
    }
    
    //    bool mc = settings.manual;
    //    if (mc)
    //        return;
    
    // use or not raw data from simulator
    if (settings.isAttRaw) {
        setupOutputObject(attRaw, qRound(1000/settings.attRawRate));
    } else {
        setupDefaultObject(attRaw);
    }
    
    // use or not attitudeActual from simulator
    // or take raw data and calculate attitudeActual by OP/CC
    if (settings.isAttAct) {
        setupOutputObject(attActual, qRound(1000/settings.attActRate));
    } else if (settings.isAttRaw && !settings.isAttAct) {
        setupInputObject(attActual,  qRound(1000/settings.attActRate));
    } else {
        setupDefaultObject(attActual);
    }
    
    if (settings.isBaroAlt) {
        setupOutputObject(baroAlt, qRound(1000/settings.baroAltRate));
    } else {
        setupDefaultObject(baroAlt);
    }
    
    if (settings.isGPSPos) {
        setupOutputObject(gpsPos, qRound(1000/settings.GPSPosRate));
    } else {
        setupDefaultObject(gpsPos);
    }
    
    if (settings.isPosAct) {
        setupOutputObject(posActual, qRound(1000/settings.posActRate));
    } else {
        setupDefaultObject(posActual);
    }
    
    if (settings.isVelAct) {
        setupOutputObject(velActual, qRound(1000/settings.velActRate));
    } else {
        setupDefaultObject(velActual);
    }
    
    if (settings.isHomeLoc) {
        setupOutputObject(posHome, qRound(1000/settings.posActRate));
    } else {
        setupDefaultObject(posHome);
    }
    
}

void Simulator::setupInputObject(UAVObject* obj, int updatePeriod)
{
    UAVObject::Metadata mdata;
    mdata = obj->getDefaultMetadata();
    mdata.flightAccess = UAVObject::ACCESS_READWRITE;
    mdata.gcsAccess = UAVObject::ACCESS_READONLY;
    mdata.gcsTelemetryUpdateMode = UAVObject::UPDATEMODE_NEVER;
    mdata.flightTelemetryAcked = false;
    mdata.flightTelemetryUpdateMode = UAVObject::UPDATEMODE_PERIODIC;
    mdata.flightTelemetryUpdatePeriod = updatePeriod;
    obj->setMetadata(mdata);
}

void Simulator::setupOutputObject(UAVObject* obj, int updatePeriod)
{
    UAVObject::Metadata mdata;
    mdata = obj->getDefaultMetadata();
    mdata.gcsAccess = UAVObject::ACCESS_READWRITE;
    mdata.flightAccess = UAVObject::ACCESS_READONLY;
    mdata.flightTelemetryUpdateMode = UAVObject::UPDATEMODE_NEVER;
    mdata.gcsTelemetryAcked = false;
    mdata.gcsTelemetryUpdateMode = UAVObject::UPDATEMODE_PERIODIC;
    mdata.gcsTelemetryUpdatePeriod = updatePeriod;
    obj->setMetadata(mdata);
}

// reset to default
void Simulator::setupDefaultObject(UAVObject *obj)
{
    UAVObject::Metadata mdata;
    mdata = obj->getDefaultMetadata();
    obj->setMetadata(mdata);
}

void Simulator::onAutopilotConnect()
{
    autopilotConnectionStatus = true;
    setupObjects();
    emit autopilotConnected();
}

void Simulator::onAutopilotDisconnect()
{
    autopilotConnectionStatus = false;
    emit autopilotDisconnected();
}

void Simulator::onSimulatorConnectionTimeout()
{
    if (simConnectionStatus) {
        simConnectionStatus = false;
        emit simulatorDisconnected();
    }
}

void Simulator::telStatsUpdated(UAVObject* obj)
{
    GCSTelemetryStats::DataFields stats = telStats->getData();
    if (!autopilotConnectionStatus && stats.Status == GCSTelemetryStats::STATUS_CONNECTED) {
        onAutopilotConnect();
    } else if (autopilotConnectionStatus && stats.Status != GCSTelemetryStats::STATUS_CONNECTED) {
        onAutopilotDisconnect();
    }
}
