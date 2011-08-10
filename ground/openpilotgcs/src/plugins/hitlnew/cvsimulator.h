/**
 ******************************************************************************
 *
 * @file       cvsimulator.h
 * @author     hhrhhr
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup HITLPlugin HITL Plugin
 * @{
 * @brief The Hardware In The Loop plugin (CV)
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

#ifndef CVSIMULATOR_H
#define CVSIMULATOR_H

//#define USE_DEBUG_CV_TIMER

#include <QObject>
#include "simulator.h"
#include <QMatrix4x4>
#include <QVector3D>
#include <QTime>
#include <qmath.h>

class CVSimulator: public Simulator
{
    Q_OBJECT
public:
    CVSimulator(const SimulatorSettings& params);
    ~CVSimulator();

    void setupTcpPorts(const QString& host, int inPort, int outPort);
    bool setupProcess();

private slots:
    void transmitUpdate();

private:
    void processUpdate(const QByteArray& data);

    float throttlehold;
    float iddleup;
    float simTimeOld;
    QVector3D posOld;
    QVector3D speedOld;

    bool firstStart;
    bool firstUpdate;
    void sendCommand2CV(QString command);

    void cvMatrix2quaternion(const QMatrix4x4& M, QQuaternion& Q);
    void cvMatrix2rpy(const QMatrix4x4& M, QVector3D& rpy);
    void quaternion2rpy(const QQuaternion& Q, QVector3D& rpy);

    QTime rxTimer;

#ifdef USE_DEBUG_CV_TIMER
    int inputcount;
    int inputtimevalue;
    QTime inputtimer;
    int outputcount;
    int outputtimevalue;
    QTime outputtimer;

#endif
};

class CVSimulatorCreator : public SimulatorCreator
{
public:
    CVSimulatorCreator(const QString& classId, const QString& description)
    : SimulatorCreator (classId,description)
    {}

    Simulator* createSimulator(const SimulatorSettings& params)
    {
        return new CVSimulator(params);
    }
};
#endif // CVSIMULATOR_H
