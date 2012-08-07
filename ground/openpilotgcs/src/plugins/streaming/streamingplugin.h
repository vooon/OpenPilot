/**
 ******************************************************************************
 * @file       streamingplugin.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup streamingplugin
 * @{
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

#ifndef STREAMINGPLUGIN_H_
#define STREAMINGPLUGIN_H_

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/iconnection.h>
#include <extensionsystem/iplugin.h>
#include "uavobjectmanager.h"
#include "gcstelemetrystats.h"
#include <uavtalk/uavtalk.h>
#include <stream.h>

#include <QThread>
#include <QQueue>
#include <QReadWriteLock>

class StreamingPlugin;
class StreamingGadgetFactory;

/**
*   Define a connection via the IConnection interface
*   Plugin will add a instance of this class to the pool,
*   so the connection manager can use it.
*/
class StreamingConnection
    : public Core::IConnection
{
    Q_OBJECT
public:
    StreamingConnection();
    virtual ~StreamingConnection();

    virtual QList <Core::IConnection::device> availableDevices();
    virtual QIODevice *openDevice(const QString &deviceName);
    virtual void closeDevice(const QString &deviceName);

    virtual QString connectionName();
    virtual QString shortName();

    bool deviceOpened() {return m_deviceOpened;}
    Stream* getStream() { return &stream;}


private:
    Stream stream;


protected slots:
    void onEnumerationChanged();
/*    void startReplay(QString file);
*/
protected:
    bool m_deviceOpened;
};



class StreamingThread : public QThread
{
Q_OBJECT
public:
    bool openIP(QString ipAddress, StreamingPlugin * parent);

private slots:
    void objectUpdated(UAVObject * obj);
    void transactionCompleted(UAVObject* obj, bool success);

public slots:
    void stopStreaming();

protected:
    void run();
    QReadWriteLock lock;
    Stream stream;
    UAVTalk * uavTalk;

private:
    QQueue<UAVDataObject*> queue;

    void retrieveSettings();
    void retrieveNextObject();

};

class StreamingPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    StreamingPlugin();
    ~StreamingPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList & arguments, QString * errorString);
    void shutdown();

    StreamingConnection* getStreamConnection() { return streamConnection; };
    Stream* getStream() { return streamConnection->getStream();}
    void setStreamMenuTitle(QString str);


signals:
    void stopStreamingSignal(void);
    void stopReplaySignal(void);
    void stateChanged(QString);


protected:
    enum {IDLE, STREAMING, REPLAY} state;
    StreamingThread * streamingThread;

    // These are used for replay, streaming in its own thread
    StreamingConnection* streamConnection;

private slots:
    void toggleStreaming();
    void startStreaming(QString file);
    void stopStreaming();
    void streamingStopped();
    void replayStarted();
    void replayStopped();

private:
    StreamingGadgetFactory *mf;
    Core::Command* cmd;

};
#endif /* STREAMINGPLUGIN_H_ */
/**
 * @}
 * @}
 */
