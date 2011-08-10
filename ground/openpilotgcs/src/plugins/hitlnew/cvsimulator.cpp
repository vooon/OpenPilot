/**
 ******************************************************************************
 *
 * @file       cvsimulator.h
 * @author     hhrhhr
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

#include "cvsimulator.h"
#include "extensionsystem/pluginmanager.h"
#include "coreplugin/icore.h"
#include "coreplugin/threadmanager.h"

CVSimulator::CVSimulator(const SimulatorSettings& params) : Simulator(params)
{
    qDebug() << "CVSimulator constructed";
}

CVSimulator::~CVSimulator()
{
    qDebug() << "CVSimulator DEconstructed";
    sendCommand2CV("releaseControl-2");
    emit processOutput("Release control to the Clearview.");
    outSocketTcp->close();
}

bool CVSimulator::setupProcess()
{
    throttlehold = 0.0;
    iddleup = 0.0;
    simTimeOld = 0;
    speedOld = QVector3D();

    firstStart = TRUE;
    firstUpdate = FALSE;

#ifdef USE_DEBUG_CV_TIMER
    // timer
    inputcount = 0;
    inputtimevalue = 0;
    inputtimer.start();
    outputcount = 0;
    outputtimevalue = 0;
    outputtimer.start();
#endif

    if (settings.isHomeLoc) {
        HomeLocation::DataFields homeData = posHome->getData();
        memset(&homeData, 0, sizeof(HomeLocation::DataFields));
        // Update homelocation
        homeData.Latitude = settings.latitude * 10e6;
        homeData.Longitude = settings.longitude * 10e6;
        homeData.Altitude = 0;
        double LLA[3];
        LLA[0] = settings.latitude;
        LLA[1] = settings.longitude;
        LLA[2] = 0;
        double RNE[9];
        Utils::CoordinateConversions().RneFromLLA(LLA,(double (*)[3])RNE);
        for (int t = 0; t < 9; t++) {
            homeData.RNE[t] = RNE[t];
        }
        double ECEF[3];
        Utils::CoordinateConversions().LLA2ECEF(LLA,ECEF);
        homeData.ECEF[0] = ECEF[0]*100;
        homeData.ECEF[1] = ECEF[1]*100;
        homeData.ECEF[2] = ECEF[2]*100;
        homeData.Be[0] = 0;
        homeData.Be[1] = 0;
        homeData.Be[2] = 0;
        posHome->setData(homeData);
    }
    return true;
}

void CVSimulator::setupTcpPorts(const QString& host, int inPort, int outPort)
{
    Q_UNUSED(inPort);
    outSocketTcp->abort();
    outSocketTcp->connectToHost(QHostAddress(host), outPort);
    qDebug("CVSimulator::setupTcpPorts() ended");
}

void CVSimulator::sendCommand2CV(const QString command)
{
    // getTelemetry-2, requestControl-2, setControls-2..., releaseControl-2
    QByteArray out = command.toAscii();
    out += "\n";
    outSocketTcp->write(out, out.length());
    outSocketTcp->flush();
}

void CVSimulator::transmitUpdate()
{
    // request control and return
    if (firstStart && settings.isActDes) {
        emit processOutput("Take control of the Clearview.");
        sendCommand2CV("requestControl-2");
        firstStart = FALSE;
        return;
    }
    if (!firstUpdate || !settings.isActDes) {
        sendCommand2CV("getTelemetry-2");
        return;
    }

#ifdef USE_DEBUG_CV_TIMER
    if ( outputcount >= 200) {
        outputtimevalue = 200000 / outputtimer.elapsed();
        qDebug() << "out timer:" << outputtimevalue;
        outputcount = 0;
        outputtimer.restart();
    }
    outputcount++;
#endif

    FlightStatus::DataFields flightStatusData = flightStatus->getData();
    if (flightStatusData.Armed == FlightStatus::ARMED_ARMED) {
        float ailerons = 0;
        float elevator = 0;
        float rudder = 0;
        float throttle = -1;
        ManualControlCommand::DataFields manCtrlData = manCtrlCommand->getData();
        if (flightStatusData.FlightMode == FlightStatus::FLIGHTMODE_MANUAL) {
            // Read joystick input
            ailerons = manCtrlData.Roll;
            elevator = manCtrlData.Pitch;
            rudder = manCtrlData.Yaw;
            throttle = manCtrlData.Throttle;
        } else {
            // Read ActuatorDesired from autopilot
            ActuatorDesired::DataFields actData;
            actData = actDesired->getData();
            ailerons = actData.Roll;
            elevator = actData.Pitch;
            rudder = actData.Yaw;
            throttle = qBound(0.f, actData.Throttle, 1.f);
            throttle = throttle * 2 - 1;

            // protection against NaN in some cases
            if ((ailerons != ailerons) || (elevator != elevator)
             || (rudder != rudder) || (throttle != throttle)) {
                qDebug("NaN detected!!!");
                if (ailerons != ailerons)   ailerons = 0;
                if (elevator != elevator)   elevator = 0;
                if (rudder != rudder)       rudder = 0;
                if (throttle != throttle)   throttle = -1;
            }
        }
        QString command = QString("setControls-2 %1 %2 %3 %4 %5 %6")
                .arg(-ailerons).arg(-elevator).arg(throttle).arg(-rudder)
                .arg(throttlehold).arg(iddleup);
        sendCommand2CV(command);
    } else {
        // if not armed just get telemetry
        sendCommand2CV("getTelemetry-2");
    }
}

void CVSimulator::processUpdate(const QByteArray& inp)
{
#ifdef USE_DEBUG_CV_TIMER
    if ( inputcount >= 200) {
        inputtimevalue = 200000 / inputtimer.elapsed();
        qDebug() << "in timer" << inputtimevalue;
//        emit processOutput(QString::number(inputtimevalue));
        inputcount = 0;
        inputtimer.restart();
    }
    inputcount++;
#endif

    QStringList in = QString(inp).split(" "); // must be 34 items
    // check
    if ( in.length() < 34 ) {
        if (in.length() == 1 && in.at(0) == "ok\n") {
            qDebug() << "CVSimulator::processUpdate: previous command OK";
        } else {
            qDebug() << "CVSimulator::processUpdate: small packet detected:\n" << inp;
        }
        return;
    }

    // init
    float simTimeElapsed;  // ms
                        // m[row][column]
    QMatrix4x4 m4D;     // m[0][3] posX, m[1][3] posY, m[2][3] posZ; m
    QVector3D rate;     // roll, yaw, pitch; rad/sec
    float steptime;     // simulator step time; ms (default = 1/120)
    QVector3D speed;    // speed vector in world coord, X, Y, Z; m/sec
    float flags[2];     // [0] crash (0-no/1-yes), [1] stop engine (0-worked/1-stop)
    float modelType;    // heli/plane (0/1)
    float control[6];   // ail, ele, thr, rud; -1 to +1
                        // throttle hold or gear (-1/1), idle up or flaps (-1/1)
    QString cr;         // "\n" at end

    // parse begin
    simTimeElapsed = in.at(0).toFloat();   //ms from simulator start

    m4D = QMatrix4x4(in.at( 1).toDouble(), in.at( 2).toDouble(), in.at( 3).toDouble(), in.at( 4).toDouble(),
                     in.at( 5).toDouble(), in.at( 6).toDouble(), in.at( 7).toDouble(), in.at( 8).toDouble(),
                     in.at( 9).toDouble(), in.at(10).toDouble(), in.at(11).toDouble(), in.at(12).toDouble(),
                     in.at(13).toDouble(), in.at(14).toDouble(), in.at(15).toDouble(), in.at(16).toDouble()
                     );

    rate = QVector3D(in.at(17).toDouble() * -RAD2DEG,     //roll
                     in.at(18).toDouble() * -RAD2DEG,     //yaw
                     in.at(19).toDouble() * -RAD2DEG      //pitch
                     );

    steptime = in.at(20).toFloat();

    speed = QVector3D(in.at(21).toDouble(),  // North
                      in.at(22).toDouble(),  // Down
                      in.at(23).toDouble()   // East
                      );

    flags[0] = in.at(24).toFloat();
    flags[1] = in.at(25).toFloat();
    modelType = in.at(26).toFloat();

    control[0] = in.at(27).toFloat();   // -1.0 ... +1.0
    control[1] = in.at(28).toFloat();
    control[2] = in.at(29).toFloat();
    control[3] = in.at(30).toFloat();
    control[4] = in.at(30).toFloat();   //
    control[5] = in.at(30).toFloat();

    cr = in.at(33);
    if (cr != "\n") {
        qDebug() << "CVSimulator::processUpdate: wrong last char:\n" << cr;
        return;
    }

    // find quat
    QQuaternion Q;
    cvMatrix2quaternion(m4D, Q);

    if (settings.isAttRaw) {
        // Update AttitudeRaw object
        AttitudeRaw::DataFields rawData;
        memset(&rawData, 0, sizeof(AttitudeRaw::DataFields));
        rawData = attRaw->getData();
        rawData.gyros[0] = rate.x();
        rawData.gyros[1] = rate.z();
        rawData.gyros[2] = rate.y();

        // find accel data (a=dv/dt)
        float dt = simTimeElapsed - simTimeOld;
        simTimeOld = simTimeElapsed;
        dt *= 0.001;

        QVector3D dv = speed - speedOld;
        speedOld = speed;

        QVector3D a = (dv / dt);
        a.setY(dv.y() - GEE);

        QQuaternion Qinv = Q.conjugate();
        QQuaternion A = QQuaternion(0, a);
        a = (Qinv * A * Q).vector();

        rawData.accels[0] = a.x();
        rawData.accels[1] = a.z();
        rawData.accels[2] = a.y();

        attRaw->setData(rawData);
    }

    if (settings.isAttAct) {
        // Update attActual object
        AttitudeActual::DataFields attActualData;
        memset(&attActualData, 0, sizeof(AttitudeActual::DataFields));

        attActualData.q1 = Q.scalar();
        attActualData.q2 = Q.x();
        attActualData.q3 = Q.y();
        attActualData.q4 = Q.z();
        // find rpy
        QVector3D rpy;
        cvMatrix2rpy(m4D, rpy);
        rpy *= -1;
        attActualData.Roll = rpy.x();
        attActualData.Pitch = rpy.y();
        attActualData.Yaw = rpy.z();
        // set data
        attActual->setData(attActualData);
    }

    if (settings.isPosAct) {
        // Update PositionActual {Nort, East, Down}
        PositionActual::DataFields positionActualData;
        memset(&positionActualData, 0, sizeof(PositionActual::DataFields));
        positionActualData.North = m4D(0,3) * 100;  // m -> sm
        positionActualData.East = m4D(1,3) * 100;
        positionActualData.Down = m4D(2,3) * 100;
        posActual->setData(positionActualData);
    }

    if (settings.isVelAct) {
        // Update VelocityActual {Nort, East, Down}
        VelocityActual::DataFields velocityActualData;
        memset(&velocityActualData, 0, sizeof(VelocityActual::DataFields));
        velocityActualData.North = speed.x() * 100;
        velocityActualData.East = speed.z() * 100;
        velocityActualData.Down = speed.y() * 100;
        velActual->setData(velocityActualData);
    }

    // save current data
    throttlehold = control[4];
    iddleup = control[5];

    if (!firstUpdate) {
        firstUpdate = TRUE;
    }
}

// all conversion code from http://www.euclideanspace.com
// TODO: check "copysign" function on other platforms (win ok, ubuntu ok...)
void CVSimulator::cvMatrix2rpy(const QMatrix4x4 &M, QVector3D &rpy)
{
    qreal roll;
    qreal pitch;
    qreal yaw;

    if (qFabs(M(1, 0)) > 0.998d) { // ~86.3°
        // gimbal lock
        roll  = 0.0d;
        pitch = copysign(M_PI_2, M(1, 0));
        yaw   = qAtan2(M(0, 2), M(2, 2));
    } else {
        roll  = qAtan2(-M(1, 2), M(1, 1));
        pitch = qAsin ( M(1, 0));
        yaw   = qAtan2(-M(2, 0), M(0, 0));
    }

    rpy.setX(roll  * RAD2DEG);
    rpy.setY(pitch * RAD2DEG);
    rpy.setZ(yaw   * RAD2DEG);
}

void CVSimulator::cvMatrix2quaternion(const QMatrix4x4 &M, QQuaternion &Q)
{
    qreal w, x, y, z;

    w = qSqrt(qMax(0.0d,   1.0d + M(0, 0)  + M(1, 1)  + M(2, 2))) / 2.0d;
    x = qSqrt(qMax(0.0d,  (1.0d + M(0, 0)) - M(1, 1)  - M(2, 2))) / 2.0d;
    y = qSqrt(qMax(0.0d, ((1.0d - M(0, 0)) + M(1, 1)) - M(2, 2))) / 2.0d;
    z = qSqrt(qMax(0.0d,  (1.0d - M(0, 0)  - M(1, 1)) + M(2, 2))) / 2.0d;

    x = copysign(x, (M(2, 1) - M(1, 2)));
    y = copysign(y, (M(0, 2) - M(2, 0)));
    z = copysign(z, (M(1, 0) - M(0, 1)));

    Q.setScalar(w);
    Q.setX(x);
    Q.setY(y);
    Q.setZ(z);
}

void CVSimulator::quaternion2rpy(const QQuaternion &Q, QVector3D &rpy)
{
    qreal roll, pitch, yaw;
    qreal d2 = 2.0d;

    qreal test = d2 * (Q.x() * Q.y() + Q.z() * Q.scalar());
    if (qFabs(test) > 0.998d) {
        // gimbal lock
        roll = 0.0d;
        pitch = copysign(M_PI_2, test);
        yaw = d2 * qAtan2(Q.x(), Q.scalar());
        yaw = copysign(yaw, test);
    } else {
        qreal qxx = Q.x() * Q.x();
        qreal qyy = Q.y() * Q.y();
        qreal qzz = Q.z() * Q.z();

        qreal r1 = d2 * (Q.x() * Q.scalar() - Q.y() * Q.z());
        qreal r2 = 1.0d - (d2 * (qxx + qzz));

        qreal y1 = d2 * (Q.y() * Q.scalar() - Q.x() * Q.z());
        qreal y2 = 1.0d - (d2 * (qyy + qzz));

        roll = qAtan2(r1, r2);
        pitch = qAsin(test);
        yaw = qAtan2(y1, y2);
    }

    rpy.setX(roll  * RAD2DEG);
    rpy.setY(pitch * RAD2DEG);
    rpy.setZ(yaw   * RAD2DEG);
}
