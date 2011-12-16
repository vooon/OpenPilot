/**
 ******************************************************************************
 *
 * @file       notifyaudiowrapper.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @brief
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup   notify
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
#include <QStringList>

#include "notifyaudiowrapper.h"
#include "notifylogging.h"
#include "backendcapabilities.h"

NotifyAudioWrapper::NotifyAudioWrapper(QObject *parent) :
    QObject(parent),
    _mediaObject(0),
    _firstPlay(true)
{
    qRegisterMetaType<QString>("QString");
}
void NotifyAudioWrapper::initialise()
{
    if(_mediaObject != NULL)
    {
        delete _mediaObject;
        _mediaObject = NULL;
    }

    _mediaObject = Phonon::createPlayer(Phonon::NotificationCategory);
    _mediaObject->clearQueue();
    _firstPlay = true;
    QList<Phonon::AudioOutputDevice> audioOutputDevices =
              Phonon::BackendCapabilities::availableAudioOutputDevices();
    foreach(Phonon::AudioOutputDevice dev, audioOutputDevices) {
        qNotifyDebug() << "Notify: Audio Output device: " << dev.name() << " - " << dev.description();
    }
    connect(_mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
        this, SLOT(handleStateChanged(Phonon::State,Phonon::State)));
}

void NotifyAudioWrapper::play(QString soundList)
{
    _mediaObject->clear();

    foreach (QString item, soundList.split('|', QString::SkipEmptyParts))
    {
        _mediaObject->enqueue(item);
    }
    _mediaObject->play();
    _firstPlay = false; // On Linux, you sometimes have to nudge Phonon to play 1 time before
                              // the state is not "Loading" anymore.
}

void NotifyAudioWrapper::handleStateChanged(Phonon::State newstate, Phonon::State oldstate)
{
    Q_UNUSED(oldstate)

    if (newstate  == Phonon::ErrorState) {
        if (_mediaObject->errorType()==0) {
            qDebug() << "Phonon::ErrorState: ErrorType = " << _mediaObject->errorType();
            _mediaObject->clear();
        }
    }

    emit stateChanged(newstate, oldstate);
}
