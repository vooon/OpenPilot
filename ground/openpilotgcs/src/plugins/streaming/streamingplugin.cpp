/**
 ******************************************************************************
 *
 * @file       streaming.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @brief      Import/Export Plugin
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup   Streaming
 * @{
 *
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

#include "streamingplugin.h"
#include <QDebug>
#include <QtPlugin>
#include <QThread>
#include <QStringList>
#include <QDir>
#include <QFileDialog>
#include <QList>
#include <QErrorMessage>
#include <QWriteLocker>

#include <extensionsystem/pluginmanager.h>
#include <QKeySequence>
#include "uavobjectmanager.h"
#include "../ipconnection/ipconnectionplugin.h"


StreamingConnection::StreamingConnection()
{

}

StreamingConnection::~StreamingConnection()
{
}

void StreamingConnection::onEnumerationChanged()
{
        emit availableDevChanged(this);
}


QList <Core::IConnection::device> StreamingConnection::availableDevices()
{
//    QList <device> list;
//    device d;
//    d.displayName="Stream replay...";
//    d.name="Stream replay...";
//    list <<d;

//    return list;
}

QIODevice* StreamingConnection::openDevice(const QString &deviceName)
{
    Q_UNUSED(deviceName)

    if (stream.isOpen()){
        stream.close();
    }
/*    QString fileName = QFileDialog::getOpenFileName(NULL, tr("Open file"), QString(""), tr("OpenPilot Stream (*.opl)"));
    if (!fileName.isNull()) {
        startReplay(fileName);
    }
*/
    return &stream;
}
/*
void StreamingConnection::startReplay(QString file)
{
    stream.setFileName(file);
    if(stream.open(QIODevice::ReadOnly)) {
        qDebug() << "Replaying " << file;
        // state = REPLAY;
        stream.startReplay();
    }
}
*/
void StreamingConnection::closeDevice(const QString &deviceName)
{
    Q_UNUSED(deviceName);
    //we have to delete the serial connection we created
    if (stream.isOpen()){
        stream.close();
        m_deviceOpened = false;
    }
}


QString StreamingConnection::connectionName()
{
    return QString("Stream replay");
}

QString StreamingConnection::shortName()
{
    return QString("Stream");
}






/**
  * Sets the ipAddress to use for streaming and takes the parent plugin
  * to connect to stop streaming signal
  * @param[in] file File name to write to
  * @param[in] parent plugin
  */
bool StreamingThread::openIP(QString ipAddress, StreamingPlugin * parent)
{
    stream.setIPAddress(ipAddress);
    stream.open(QIODevice::WriteOnly);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    uavTalk = new UAVTalk(&stream, objManager);
    connect(parent,SIGNAL(stopStreamingSignal()),this,SLOT(stopStreaming()));

    return true;
};

/**
  * Streams an object update to the file.  Data format is the
  * timestamp as a 32 bit uint counting ms from start of
  * file writing (flight time will be embedded in stream),
  * then object packet size, then the packed UAVObject.
  */
void StreamingThread::objectUpdated(UAVObject * obj)
{
    QWriteLocker locker(&lock);
    if(!uavTalk->sendObject(obj,false,false) )
        qDebug() << "Error streaming " << obj->getName();
};

/**
  * Connect signals from all the objects updates to the write routine then
  * run event loop
  */
void StreamingThread::run()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    QList< QList<UAVObject*> > list;
    list = objManager->getObjects();
    QList< QList<UAVObject*> >::const_iterator i;
    QList<UAVObject*>::const_iterator j;
    int objects = 0;

    for (i = list.constBegin(); i != list.constEnd(); ++i)
    {
        for (j = (*i).constBegin(); j != (*i).constEnd(); ++j)
        {
            connect(*j, SIGNAL(objectUpdated(UAVObject*)), (StreamingThread*) this, SLOT(objectUpdated(UAVObject*)));
            objects++;
            //qDebug() << "Detected " << j[0];
        }
    }

    GCSTelemetryStats* gcsStatsObj = GCSTelemetryStats::GetInstance(objManager);
    GCSTelemetryStats::DataFields gcsStats = gcsStatsObj->getData();
    if ( gcsStats.Status == GCSTelemetryStats::STATUS_CONNECTED )
    {
        qDebug() << "Streaming: connected already, ask for all settings";
        retrieveSettings();
    } else {
        qDebug() << "Streaming: not connected, do no ask for settings";
    }


    exec();
}


/**
  * Pass this command to the correct thread then close the file
  */
void StreamingThread::stopStreaming()
{
    QWriteLocker locker(&lock);

    // Disconnect all objects we registered with:
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    QList< QList<UAVObject*> > list;
    list = objManager->getObjects();
    QList< QList<UAVObject*> >::const_iterator i;
    QList<UAVObject*>::const_iterator j;

    for (i = list.constBegin(); i != list.constEnd(); ++i)
    {
        for (j = (*i).constBegin(); j != (*i).constEnd(); ++j)
        {
            disconnect(*j, SIGNAL(objectUpdated(UAVObject*)), (StreamingThread*) this, SLOT(objectUpdated(UAVObject*)));
        }
    }

    stream.close();
    qDebug() << "File closed";
    quit();
}

/**
 * Initialize queue with settings objects to be retrieved.
 */
void StreamingThread::retrieveSettings()
{
    // Clear object queue
    queue.clear();
    // Get all objects, add metaobjects, settings and data objects with OnChange update mode to the queue
    // Get UAVObjectManager instance
    ExtensionSystem::PluginManager* pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objMngr = pm->getObject<UAVObjectManager>();
    QList< QList<UAVDataObject*> > objs = objMngr->getDataObjects();
    for (int n = 0; n < objs.length(); ++n)
    {
        UAVDataObject* obj = objs[n][0];
        if ( obj->isSettings() )
                {
                    queue.enqueue(obj);
                }
        }
    // Start retrieving
    qDebug() << tr("Streaming: retrieve settings objects from the autopilot (%1 objects)")
                  .arg( queue.length());
    retrieveNextObject();
}


/**
 * Retrieve the next object in the queue
 */
void StreamingThread::retrieveNextObject()
{
    // If queue is empty return
    if ( queue.isEmpty() )
    {
        qDebug() << "Streaming: Object retrieval completed";
        return;
    }
    // Get next object from the queue
    UAVObject* obj = queue.dequeue();
    // Connect to object
    connect(obj, SIGNAL(transactionCompleted(UAVObject*,bool)), this, SLOT(transactionCompleted(UAVObject*,bool)));
    // Request update
    obj->requestUpdate();
}

/**
 * Called by the retrieved object when a transaction is completed.
 */
void StreamingThread::transactionCompleted(UAVObject* obj, bool success)
{
    Q_UNUSED(success);
    // Disconnect from sending object
    obj->disconnect(this);
    // Process next object if telemetry is still available
    // Get stats objects
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
    GCSTelemetryStats* gcsStatsObj = GCSTelemetryStats::GetInstance(objManager);
    GCSTelemetryStats::DataFields gcsStats = gcsStatsObj->getData();
    if ( gcsStats.Status == GCSTelemetryStats::STATUS_CONNECTED )
    {
        retrieveNextObject();
    }
    else
    {
        qDebug() << "Streaming: Object retrieval has been cancelled";
        queue.clear();
    }
}



/****************************************************************
    Streaming plugin
 ********************************/


StreamingPlugin::StreamingPlugin() : state(IDLE)
{
    streamConnection = new StreamingConnection();
}

StreamingPlugin::~StreamingPlugin()
{
    if (streamingThread)
        delete streamingThread;

    // Don't delete it, the plugin manager will do it:
    //delete streamConnection;
}

/**
  * Add Streaming plugin to Menu
  */
bool StreamingPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    streamingThread = NULL;

    //BUT THIS IS THE OP WAY TO MAKE MENUS THAT DO ACTIONS
    // Command to start streaming UAVO
    Core::ActionManager* am = Core::ICore::instance()->actionManager();
    Core::ActionContainer* ac = am->actionContainer(Core::Constants::M_TOOLS);
    cmd = am->registerAction(new QAction(this),
                                            "ConfigPlugin.Streaming",
                                            QList<int>() <<
                                            Core::Constants::C_GLOBAL_ID);

//    cmd->setDefaultKeySequence(QKeySequence("Ctrl+S"));
//    cmd->action()->setText(QString("Stream to %1: %2").arg(IPconnectionConnection::getHostID()).arg(IPconnectionConnection::getHostID()));
    cmd->action()->setText(tr("Stream to network"));

    ac->menu()->addSeparator();
    ac->appendGroup("Streaming");
    ac->addAction(cmd, "Streaming");

    connect(cmd->action(), SIGNAL(triggered(bool)), this, SLOT(toggleStreaming()));

/*
    mf = new StreamingGadgetFactory(this);
    addAutoReleasedObject(mf);

    // Map signal from end of replay to replay stopped
    connect(getStream(),SIGNAL(replayFinished()), this, SLOT(replayStopped()));
    connect(getStream(),SIGNAL(replayStarted()), this, SLOT(replayStarted()));
*/
    return true;
}

/**
  * The action that is triggered by the menu item which opens the
  * file and begins streaming if successful
  */
void StreamingPlugin::toggleStreaming()
{
    if(state == IDLE)
    {

/*        startStreaming(IP_ADDRESS_GOES_HERE);*/
        cmd->action()->setText(tr("Stop streaming"));

    }
    else if(state == STREAMING)
    {
        stopStreaming();
        cmd->action()->setText(tr("Start streaming..."));
    }
}


/**
  * Starts the streaming thread to a certain IP address
  */
void StreamingPlugin::startStreaming(QString ipAddress)
{
    qDebug() << "Streaming to " << ipAddress;
    // We have to delete the previous streaming thread if is was still there!
    if (streamingThread)
        delete streamingThread;
    streamingThread = new StreamingThread();
    if(streamingThread->openIP(ipAddress,this))
    {
        connect(streamingThread,SIGNAL(finished()),this,SLOT(streamingStopped()));
        state = STREAMING;
        streamingThread->start();
        emit stateChanged("STREAMING");
    } else {
        QErrorMessage err;
        err.showMessage("Unable to open file for streaming");
        err.exec();
    }
}

/**
  * Send the stop streaming signal to the StreamingThread
  */
void StreamingPlugin::stopStreaming()
{
    emit stopStreamingSignal();
    disconnect( this,SIGNAL(stopStreamingSignal()),0,0);

}


/**
  * Receive the streaming stopped signal from the StreamingThread
  * and change status to not streaming
  */
void StreamingPlugin::streamingStopped()
{
    if(state == STREAMING)
        state = IDLE;

    emit stateChanged("IDLE");

    free(streamingThread);
    streamingThread = NULL;
}

/**
  * Received the replay stopped signal from the Stream
  */
void StreamingPlugin::replayStopped()
{
    state = IDLE;
    emit stateChanged("IDLE");
}

/**
  * Received the replay started signal from the Stream
  */
void StreamingPlugin::replayStarted()
{
    state = REPLAY;
    emit stateChanged("REPLAY");
}




void StreamingPlugin::extensionsInitialized()
{
    addAutoReleasedObject(streamConnection);
}

void StreamingPlugin::shutdown()
{
    // Do nothing
}
Q_EXPORT_PLUGIN(StreamingPlugin)

/**
 * @}
 * @}
 */
