/**
 ******************************************************************************
 *
 * @file       flightgearbridge.h
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

#include "fgsimulator.h"
#include "extensionsystem/pluginmanager.h"
#include "coreplugin/icore.h"
#include "coreplugin/threadmanager.h"

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif




//FGSimulator::FGSimulator(QString hostAddr, int outPort, int inPort, bool manual, QString binPath, QString dataPath) :
//		Simulator(hostAddr, outPort, inPort,  manual, binPath, dataPath),
//		fgProcess(NULL)
//{
//	// Note: Only tested on windows 7
//#if defined(Q_WS_WIN)
//	cmdShell = QString("c:/windows/system32/cmd.exe");
//#else
//	cmdShell = QString("bash");
//#endif
//}

FGSimulator::FGSimulator(const SimulatorSettings& params) :
		Simulator(params)
{
    udpCounterFGrecv = 0;
    udpCounterGCSsend = 0;
}

FGSimulator::~FGSimulator()
{
	disconnect(simProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processReadyRead()));
}

void FGSimulator::setupUdpPorts(const QString& host, int inPort, int outPort)
{
    if(inSocket->bind(QHostAddress(host), inPort))
        emit processOutput("Successfully bound to address " + host + " on port " + QString::number(inPort) + "\n");
    else
        emit processOutput("Cannot bind to address " + host + " on port " + QString::number(inPort) + "\n");

    once = false;
}

bool FGSimulator::setupProcess()
{

    QMutexLocker locker(&lock);
    udpCounterGCSsend = 0;

    QString args("--fg-root=\"" + settings.dataPath + "\" " +
                 "--timeofday=noon " +
                 "--httpd=5400 " +
                 "--enable-hud " +
                 "--in-air " +
                 "--altitude=3000 " +
                 "--vc=100 " +
                 "--log-level=alert " +
                 "--generic=socket,out,20," + settings.hostAddress + "," +
                 QString::number(settings.inPort) + ",udp,opfgprotocol" +
                 "--generic=socket,in,40," + settings.remoteHostAddress + "," +
                 QString::number(settings.outPort) + ",udp,opfgprotocol");

    emit processOutput("Start Flightgear from the command line with the following arguments: \n\n" + args + "\n\n" +
                       "You can optionally run Flightgear from a networked computer.\n" +
                       "Make sure the computer running Flightgear can can ping your local interface adapter. ie." + settings.hostAddress + "\n"
                       "Remote computer must have the correct OpenPilot protocol installed.");

    return true;
    /*
	QMutexLocker locker(&lock);

	// Copy FlightGear generic protocol configuration file to the FG protocol directory
	// NOTE: Not working on Windows 7, if FG is installed in the "Program Files",
	// likelly due to permissions. The file should be manually copied to data/Protocol/opfgprotocol.xml
//	QFile xmlFile(":/flightgear/genericprotocol/opfgprotocol.xml");
//	xmlFile.open(QIODevice::ReadOnly | QIODevice::Text);
//	QString xml = xmlFile.readAll();
//	xmlFile.close();
//	QFile xmlFileOut(pathData + "/Protocol/opfgprotocol.xml");
//	xmlFileOut.open(QIODevice::WriteOnly | QIODevice::Text);
//	xmlFileOut.write(xml.toAscii());
//	xmlFileOut.close();

	Qt::HANDLE mainThread = QThread::currentThreadId();
	qDebug() << "setupProcess Thread: "<< mainThread;

	simProcess = new QProcess();
	simProcess->setReadChannelMode(QProcess::MergedChannels);
	connect(simProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processReadyRead()));
	// Note: Only tested on windows 7
#if defined(Q_WS_WIN)
	QString cmdShell("c:/windows/system32/cmd.exe");
#else
	QString cmdShell("bash");
#endif

	// Start shell (Note: Could not start FG directly on Windows, only through terminal!)
	simProcess->start(cmdShell);
	if (simProcess->waitForStarted() == false)
	{
		emit processOutput("Error:" + simProcess->errorString());
		return false;
	}

	// Setup arguments
	// Note: The input generic protocol is set to update at a much higher rate than the actual updates are sent by the GCS.
	// If this is not done then a lag will be introduced by FlightGear, likelly because the receive socket buffer builds up during startup.
        QString args("--fg-root=\"" + settings.dataPath + "\" " +
                     "--timeofday=noon " +
                     "--httpd=5400 " +
                     "--enable-hud " +
                     "--in-air " +
                     "--altitude=3000 " +
                     "--vc=100 " +
                     "--log-level=alert " +
                     "--generic=socket,out,20," + settings.hostAddress + "," + QString::number(settings.inPort) + ",udp,opfgprotocol");
	if(!settings.manual)
	{
            args.append(" --generic=socket,in,400," + settings.remoteHostAddress + "," + QString::number(settings.outPort) + ",udp,opfgprotocol");
	}

        // Start FlightGear - only if checkbox is selected in HITL options page
        if (settings.startSim)
        {
            QString cmd("\"" + settings.binPath + "\" " + args + "\n");
            simProcess->write(cmd.toAscii());
        }
        else
        {
            emit processOutput("Start Flightgear from the command line with the following arguments: \n\n" + args + "\n\n" +
                               "You can optionally run Flightgear from a networked computer.\n" +
                               "Make sure the computer running Flightgear can can ping your local interface adapter. ie." + settings.hostAddress + "\n"
                               "Remote computer must have the correct OpenPilot protocol installed.");
        }

        udpCounterGCSsend = 0;

	return true;
        */
}

void FGSimulator::processReadyRead()
{
	QByteArray bytes = simProcess->readAllStandardOutput();
	QString str(bytes);
	if ( !str.contains("Error reading data") ) // ignore error
	{
		emit processOutput(str);
	}
}

void FGSimulator::transmitUpdate()
{
    ActuatorDesired::DataFields actData;
    FlightStatus::DataFields flightStatusData = flightStatus->getData();
    ManualControlCommand::DataFields manCtrlData = manCtrlCommand->getData();
    float ailerons = -1;
    float elevator = -1;
    float rudder = -1;
    float throttle = -1;

    if(flightStatusData.FlightMode == FlightStatus::FLIGHTMODE_MANUAL)
    {
        // Read joystick input
        if(flightStatusData.Armed == FlightStatus::ARMED_ARMED)
        {
            // Note: Pitch sign is reversed in FG ?
            ailerons = manCtrlData.Roll;
            elevator = manCtrlData.Pitch;
            rudder = manCtrlData.Yaw;
            throttle = manCtrlData.Throttle;
        }
    }
    else
    {
         // Read ActuatorDesired from autopilot
        actData = actDesired->getData();

        ailerons = actData.Roll;
        elevator = actData.Pitch;
        rudder = actData.Yaw;
        throttle = actData.Throttle;
    }

    int allowableDifference = 10;

    //qDebug() << "UDP sent:" << udpCounterGCSsend << " - UDP Received:" << udpCounterFGrecv;

    if(udpCounterFGrecv == udpCounterGCSsend)
        udpCounterGCSsend = 0;
    
    if((udpCounterGCSsend < allowableDifference) || (udpCounterFGrecv==0) ) //FG udp queue is not delayed
    {       
        udpCounterGCSsend++;

	// Send update to FlightGear
	QString cmd;
        cmd = QString("%1,%2,%3,%4,%5\n")
              .arg(ailerons) //ailerons
              .arg(elevator) //elevator
              .arg(rudder) //rudder
              .arg(throttle) //throttle
              .arg(udpCounterGCSsend); //UDP packet counter delay

	QByteArray data = cmd.toAscii();

        //qDebug() << "Sending packet to FG";

        if(outSocket->writeDatagram(data, QHostAddress(settings.remoteHostAddress), settings.outPort) == -1)
        {
            emit processOutput("Error sending UDP packet to FG: " + outSocket->errorString() + "\n");
        }
    }
    else
    {
        // don't send new packet. Flightgear cannot process UDP fast enough.
        // V1.9.1 reads udp packets at set frequency and will get delayed if packets are sent too fast
        // V2.0 DOES NOT currently work with --generic-protocol
        // V2.4 DOES work with --generic-protocol and reads UDP packets. Yay!
    }

    /*
    if(!settings.manual)
    {
        actData.Roll = ailerons;
        actData.Pitch = elevator;
        actData.Yaw = rudder;
        actData.Throttle = throttle;
        //actData.NumLongUpdates = (float)udpCounterFGrecv;
        //actData.UpdateTime = (float)udpCounterGCSsend;
        actDesired->setData(actData);
    }*/
}


void FGSimulator::processUpdate(const QByteArray& inp)
{
    //TODO: this does not use the FLIGHT_PARAM structure, it should!

	QString data(inp);
        //qDebug() << data;

	QStringList fields = data.split(",");

        //Check for correct number of fields
        if (fields.length() != 28) {
            emit processOutput("Number of fields incorrect. Should be 28. Received " + QString(fields.length()) + "\n");
            emit processOutput("Data string:\n" + data + "\n");
            return;
        }

	// Get pitch (deg)
        float pitch = fields[0].toFloat();
	// Get pitchRate (deg/s)
        float pitchRate = fields[1].toFloat();
	// Get roll (deg)
        float roll = fields[2].toFloat();
	// Get rollRate (deg/s)
        float rollRate = fields[3].toFloat();
	// Get yaw (deg)
        float yaw = fields[4].toFloat();
	// Get yawRate (deg/s)
        float yawRate = fields[5].toFloat();
	// Get latitude (deg)
        float latitude = fields[6].toFloat();
	// Get longitude (deg)
        float longitude = fields[7].toFloat();
	// Get heading (deg)
        float heading = fields[8].toFloat();
	// Get altitude (m)
        float altitude = fields[9].toFloat() * FT2M;
	// Get altitudeAGL (m)
        float altitudeAGL = fields[10].toFloat() * FT2M;
        // Get xVel,yVel,zVel
        float velX = fields[11].toFloat();
        float velY = fields[12].toFloat();
        float velZ = fields[13].toFloat();
	// Get groundspeed (m/s)
        float groundspeed = sqrt(pow(velX,2) + pow(velY,2));
	// Get airspeed (m/s)
        //float airspeed = fields[12].toFloat() * KT2MPS;
	// Get temperature (degC)
        float temperature = fields[14].toFloat();
	// Get pressure (kpa)
        float pressure = fields[15].toFloat() * INHG2KPA;
	// Get VelocityActual Down (cm/s)
        float positionActualDown = - fields[16].toFloat() * FT2M; //FPS2CMPS;
	// Get VelocityActual East (cm/s)
        float positionActualEast = fields[17].toFloat() * FT2M; //FPS2CMPS;
	// Get VelocityActual Down (cm/s)
        float positionActualNorth = fields[18].toFloat() * FT2M; //FPS2CMPS;
        // Get Joystick Axis 1
        float joystickAxis1 = fields[19].toFloat(); //Roll (fields[17].toFloat()/2 + 1.5) * 1000;
        // Get Joystick Axis 2
        float joystickAxis2 = fields[20].toFloat(); //Pitch (fields[18].toFloat()/2 + 1.5) * 1000;
        // Get Joystick Axis 3
        float joystickAxis3 = fields[21].toFloat(); //Throttle (fields[19].toFloat()/2 + 1.5) * 1000;
        // Get Joystick Axis 4
        float joystickAxis4 = fields[22].toFloat(); //Rudder (fields[20].toFloat()/2 + 1.5) * 1000;
        // Get Joystick Axis 5
        float joystickAxis5 = (fields[23].toFloat()/2 + 1.5) * 1000; //Convert to servo pulse counts
        // Get Joystick Axis 6
        float joystickAxis6 = (fields[24].toFloat()/2 + 1.5) * 1000;
        // Get Joystick Axis 7
        float joystickAxis7 = (fields[25].toFloat()/2 + 1.5) * 1000;
        // Get Joystick Axis 8
        float joystickAxis8 = (fields[26].toFloat()/2 + 1.5) * 1000;

        // Get UDP packets received by FG
        int n = fields[27].toInt();
        udpCounterFGrecv = n;

        //run once
        HomeLocation::DataFields homeData = posHome->getData();
        if(!once)
        {
            //Print first packet to GUI
            emit processOutput("First FG packet:\n" + data);

            memset(&homeData, 0, sizeof(HomeLocation::DataFields));
            // Update homelocation
            homeData.Latitude = latitude * 10e6;
            homeData.Longitude = longitude * 10e6;
            homeData.Altitude = 0;
            double LLA[3];
            LLA[0]=latitude;
            LLA[1]=longitude;
            LLA[2]=0;
            double ECEF[3];
            double RNE[9];
            Utils::CoordinateConversions().RneFromLLA(LLA,(double (*)[3])RNE);
            for (int t=0;t<9;t++) {
                    homeData.RNE[t]=RNE[t];
            }
            Utils::CoordinateConversions().LLA2ECEF(LLA,ECEF);
            homeData.ECEF[0]=ECEF[0]*100;
            homeData.ECEF[1]=ECEF[1]*100;
            homeData.ECEF[2]=ECEF[2]*100;
            homeData.Be[0]=0;
            homeData.Be[1]=0;
            homeData.Be[2]=0;
            posHome->setData(homeData);
            once=1;
        }
	
	// Update VelocityActual.{Nort,East,Down}

        VelocityActual::DataFields velocityActualData;
	memset(&velocityActualData, 0, sizeof(VelocityActual::DataFields));
        velocityActualData.North = velY;
        velocityActualData.East = velX; //need to check that this matches the sim coordinate system
        velocityActualData.Down = velZ;
	velActual->setData(velocityActualData);

	// Update PositionActual.{Nort,East,Down}
	PositionActual::DataFields positionActualData;
	memset(&positionActualData, 0, sizeof(PositionActual::DataFields));
        positionActualData.North = positionActualNorth * 100; //Currently hardcoded as there is no way of setting up a reference point to calculate distance
        positionActualData.East = positionActualEast * 100; //Currently hardcoded as there is no way of setting up a reference point to calculate distance
        positionActualData.Down = positionActualDown * 100; //(altitude * 100); //Multiply by 100 because positionActual expects input in Centimeters.
        posActual->setData(positionActualData);

	// Update AltitudeActual object
        BaroAltitude::DataFields altActualData;
        memset(&altActualData, 0, sizeof(BaroAltitude::DataFields));
        altActualData.Altitude = altitudeAGL;
	altActualData.Temperature = temperature;
	altActualData.Pressure = pressure;
	altActual->setData(altActualData);

	// Update attActual object
	AttitudeActual::DataFields attActualData;
        memset(&attActualData, 0, sizeof(AttitudeActual::DataFields));
	attActualData.Roll = roll;
	attActualData.Pitch = pitch;
	attActualData.Yaw = yaw;

        /*
        float rpy[] = {roll,pitch,yaw};
        float quat[4] = {0.0};
        Utils::CoordinateConversions().RPY2Quaternion(rpy,quat);

        attActualData.q1 = quat[0];
        attActualData.q2 = quat[1];
        attActualData.q3 = quat[2];
        attActualData.q4 = quat[3];
        */
	attActual->setData(attActualData);

	// Update gps objects
        GPSPosition::DataFields gpsData;
        memset(&gpsData, 0, sizeof(GPSPosition::DataFields));
        gpsData.Altitude = altitude;
        gpsData.Heading = heading;
	gpsData.Groundspeed = groundspeed;
        gpsData.Latitude = latitude*1e7;
        gpsData.Longitude = longitude*1e7;
	gpsData.Satellites = 10;
        gpsData.Status = GPSPosition::STATUS_FIX3D;
        gpsPos->setData(gpsData);

        /*float NED[3];
        double LLA[3] = {(double) gpsData.Latitude / 1e7, (double) gpsData.Longitude / 1e7, (double) (gpsData.GeoidSeparation + gpsData.Altitude)};
        // convert from cm back to meters
        double ECEF[3] = {(double) (homeData.ECEF[0] / 100), (double) (homeData.ECEF[1] / 100), (double) (homeData.ECEF[2] / 100)};
                Utils::CoordinateConversions().LLA2Base(LLA, ECEF, (float (*)[3]) homeData.RNE, NED);

        positionActualData.North = NED[0]*100; //Currently hardcoded as there is no way of setting up a reference point to calculate distance
        positionActualData.East = NED[1]*100; //Currently hardcoded as there is no way of setting up a reference point to calculate distance
        positionActualData.Down = NED[2]*100; //Multiply by 100 because positionActual expects input in Centimeters.
        posActual->setData(positionActualData);
        */

        // Update AttitudeRaw object (filtered gyros only for now)
        AttitudeRaw::DataFields rawData;
        memset(&rawData, 0, sizeof(AttitudeRaw::DataFields));
        rawData = attRaw->getData();
        rawData.gyros[0] = rollRate;
        rawData.gyros[1] = pitchRate;
        rawData.gyros[2] = yawRate;
        attRaw->setData(rawData);
        // attRaw->updated();

        // Update Joystick object
        ManualControlCommand::DataFields man;
        memset(&man, 0, sizeof(ManualControlCommand::DataFields));
        man = manCtrlCommand->getData();
        man.Roll = joystickAxis1;
        man.Pitch = joystickAxis2;
        man.Throttle = joystickAxis3;
        man.Yaw = joystickAxis4;
        man.Channel[4] = joystickAxis5;
        man.Channel[5] = joystickAxis6;
        man.Channel[6] = joystickAxis7;
        man.Channel[7] = joystickAxis8;
        man.Connected = true;
        manCtrlCommand->setData(man);

}

