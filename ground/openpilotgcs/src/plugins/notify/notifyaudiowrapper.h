/**
 ******************************************************************************
 *
 * @file       notifyaudiowrapper.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @brief      Notify Plugin audio playback wrapper header
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

#ifndef NOTIFYAUDIOWRAPPER_H
#define NOTIFYAUDIOWRAPPER_H

#include <QObject>
#include <QString>
#include <phonon/MediaObject>
#include <phonon/Path>
#include <phonon/AudioOutput>
#include <phonon/Global>

class NotifyAudioWrapper : public QObject
{
    Q_OBJECT
public:
    explicit NotifyAudioWrapper(QObject *parent = 0);
    ~NotifyAudioWrapper()
    {
        if(_mediaObject != NULL)
        {
            delete _mediaObject;
        }
    }

    bool readyToPlay()
    {
        return (_mediaObject != NULL) &&
                ((_mediaObject->state() == Phonon::PausedState) ||
                (_mediaObject->state() == Phonon::StoppedState) ||
                 _firstPlay);
    }

    void initialise();

signals:
    void stateChanged ( Phonon::State newstate, Phonon::State oldstate );

public slots:
    void play(QString soundList);

private slots:
    void handleStateChanged(Phonon::State newstate, Phonon::State oldstate);

private:
    Phonon::MediaObject* _mediaObject;
    bool _firstPlay;
};

#endif // NOTIFYAUDIOWRAPPER_H
