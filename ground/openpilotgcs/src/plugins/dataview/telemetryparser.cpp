/**
 ******************************************************************************
 *
 * @file       telemetryparser.cpp
 * @author     Edouard Lafargue & the OpenPilot team Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup GPSGadgetPlugin GPS Gadget Plugin
 * @{
 * @brief A gadget that displays GPS status and enables basic configuration
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


#include "telemetryparser.h"
#include <math.h>
#include <QDebug>
#include <QStringList>


/**
 * Initialize the parser
 */
TelemetryParser::TelemetryParser(QObject *parent) : GPSParser(parent)
{
    //qDebug() << "Using TelemetryParser Start";

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();

    UAVDataObject *systemstatsObj = dynamic_cast<UAVDataObject*>(objManager->getObject("SystemStats"));
    if (systemstatsObj != NULL) {
        connect(systemstatsObj, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(updateSystemStats(UAVObject*)));
    } else {
        qDebug() << "Error: Object is unknown (SystemStats).";
    }

    airspeedObj = dynamic_cast<UAVDataObject*>(objManager->getObject("VelocityActual"));
    if (airspeedObj != NULL ) {
        connect(airspeedObj, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(updateAirspeed(UAVObject*)));
    } else {
         qDebug() << "Error: Object is unknown (VelocityActual).";
    }

    altitudeObj = dynamic_cast<UAVDataObject*>(objManager->getObject("BaroAltitude"));
    if (altitudeObj != NULL ) {
        connect(altitudeObj, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(updateAltitude(UAVObject*)));
    } else {
         qDebug() << "Error: Object is unknown (PositionActual).";
    }

    attitudeObj = dynamic_cast<UAVDataObject*>(objManager->getObject("AttitudeActual"));
    if (attitudeObj != NULL ) {
       connect(attitudeObj, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(updateAttitude(UAVObject*)));
    } else {
        qDebug() << "Error: Object is unknown (AttitudeActual).";
    }

    headingObj = dynamic_cast<UAVDataObject*>(objManager->getObject("PositionActual"));
    if (headingObj != NULL ) {
       connect(headingObj, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(updateHeading(UAVObject*)));
    } else {
        qDebug() << "Error: Object is unknown (PositionActual).";
    }

    gpsObj = dynamic_cast<UAVDataObject*>(objManager->getObject("GPSPosition"));
    if (gpsObj != NULL) {
        connect(gpsObj, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(updateGPS(UAVObject*)));
    } else {
        qDebug() << "Error: Object is unknown (GPSPosition).";
    }

    gpsObj = dynamic_cast<UAVDataObject*>(objManager->getObject("GPSTime"));
    if (gpsObj != NULL) {
        connect(gpsObj, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(updateTime(UAVObject*)));
    } else {
        qDebug() << "Error: Object is unknown (GPSTime).";
    }

    gcsTelemetryObj = dynamic_cast<UAVDataObject*>(objManager->getObject("GCSTelemetryStats"));
    if (gcsTelemetryObj != NULL ) {
        connect(gcsTelemetryObj, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(updateLinkStatus(UAVObject*)));
    } else {
        qDebug() << "Error: Object is unknown (GCSTelemetryStats).";
    }

    gcsBatteryObj = dynamic_cast<UAVDataObject*>(objManager->getObject("FlightBatteryState"));
    if (gcsBatteryObj != NULL ) {
        connect(gcsBatteryObj, SIGNAL(objectUpdated(UAVObject*)), this, SLOT(updateBattery(UAVObject*)));
    } else {
        qDebug() << "Error: Object is unknown (FlightBatteryState).";
    }
}

TelemetryParser::~TelemetryParser()
{

}

void TelemetryParser::updateSystemStats( UAVObject* object1) {
    double ftime = object1->getField(QString("FlightTime"))->getDouble();
    emit systemstats(ftime);
    //qDebug() << "updateFlightTime";
}

/*!
  \brief Called by the @ref PositionActual updates to show altitude
  */
void TelemetryParser::updateAltitude(UAVObject *object) {
    double alt = object->getField(QString("Altitude"))->getDouble();
    double temp = object->getField(QString("Temperature"))->getDouble();
    double press = object->getField(QString("Pressure"))->getDouble();
    emit altitude(alt,temp,press);
    //qDebug() << "updateAltitude";
}


/*!
  \brief Updates the GPS stats
  */
void TelemetryParser::updateGPS(UAVObject *object1) {
    double sats = object1->getField(QString("Satellites"))->getDouble();
    emit satsused(sats);

    double lat = object1->getField(QString("Latitude"))->getDouble();
    double lon = object1->getField(QString("Longitude"))->getDouble();
    double alt = object1->getField(QString("Altitude"))->getDouble();
    lat *= 1E-7;
    lon *= 1E-7;
    emit position(lat,lon,alt);

    double hdg = object1->getField(QString("Heading"))->getDouble();
    double spd = object1->getField(QString("Groundspeed"))->getDouble();
    emit speedheading(spd,hdg);

    QString fix = object1->getField(QString("Status"))->getValue().toString();
    emit fixtype(fix);

    double hdop = object1->getField(QString("HDOP"))->getDouble();
    double vdop = object1->getField(QString("VDOP"))->getDouble();
    double pdop = object1->getField(QString("PDOP"))->getDouble();
    emit dop(hdop,vdop,pdop);
    //qDebug() << "updateGPS";
}

void TelemetryParser::updateTime( UAVObject* object1) {
    double hour = object1->getField(QString("Hour"))->getDouble();
    double minute = object1->getField(QString("Minute"))->getDouble();
    double second = object1->getField(QString("Second"))->getDouble();
    double time = second + minute*100 + hour*10000;
    double year = object1->getField(QString("Year"))->getDouble();
    double month = object1->getField(QString("Month"))->getDouble();
    double day = object1->getField(QString("Day"))->getDouble();
    double date = day + month * 100 + year * 10000;
    emit datetime(date,time);
}

/*!
 \brief Updates the link stats
 */
void TelemetryParser::updateLinkStatus(UAVObject *object1) {
    // Double check that the field exists:
    QString st = QString("Status");
    QString tdr = QString("TxDataRate");
    QString rdr = QString("RxDataRate");
    UAVObjectField* field = object1->getField(st);
    UAVObjectField* field2 = object1->getField(tdr);
    UAVObjectField* field3 = object1->getField(rdr);
    if (field && field2 && field3) {
        QString s = field->getValue().toString();
        double v1 = field2->getDouble();
        double v2 = field3->getDouble();
        emit linkstatus(s,v1,v2);
    } else {
        qDebug() << "UpdateLinkStatus: Wrong field, maybe an issue with object disconnection ?";
    }
    //qDebug() << "updateLinkStatus";
}

/*!
  \brief Reads the updated attitude and computes the new display position

  Resolution is 1 degree roll & 1/7.5 degree pitch.
  */
void TelemetryParser::updateAttitude(UAVObject *object1) {
    UAVObjectField * rollField = object1->getField(QString("Roll"));
    UAVObjectField * yawField = object1->getField(QString("Yaw"));
    UAVObjectField * pitchField = object1->getField(QString("Pitch"));
    if(rollField && yawField && pitchField) {
        // These factors assume some things about the PFD SVG, namely:
        // - Roll, Pitch and Heading value in degrees
        // - Pitch lines are 300px high for a +20/-20 range, which means
        //   7.5 pixels per pitch degree.
        // TODO: loosen this constraint and only require a +/- 20 deg range,
        //       and compute the height from the SVG element.
        // Also: keep the integer value only, to avoid unnecessary redraws
        double roll=rollField->getDouble();
        double yaw=yawField->getDouble();
        double pitch=pitchField->getDouble();
        emit attitude(roll,yaw,pitch);
    } else {
        qDebug() << "Unable to get one of the fields for attitude update";
    }
    //qDebug() << "updateAttitude";
}

/*!
  \brief Updates the compass reading and speed dial.

  This also updates speed & altitude according to PositionActual data.

    Note: the speed dial shows the ground speed at the moment, because
        there is no airspeed by default. Should become configurable in a future
        gadget release (TODO)
  */
void TelemetryParser::updateHeading(UAVObject *object) {
    Q_UNUSED(object);
     //   qDebug() << "updateHeading";
}

/*!
  \brief Called by updates to @PositionActual to compute airspeed from velocity
  */
void TelemetryParser::updateAirspeed(UAVObject *object) {
    UAVObjectField* northField = object->getField("North");
    UAVObjectField* eastField = object->getField("East");
    if (northField && eastField) {
        double val = floor(sqrt(pow(northField->getDouble(),2) + pow(eastField->getDouble(),2))*10)/10;
//        double groundspeed = 3.6*val*speedScaleHeight/3000;
        double groundspeed = val/100;  // Note this is m/s , dial is kph
    emit airspeed(groundspeed);
    } else {
        qDebug() << "UpdateHeading: Wrong field, maybe an issue with object disconnection ?";
    }
    //qDebug() << "updateAirspeed";
}


/*!
  \brief Called by the UAVObject which got updated
  */
void TelemetryParser::updateBattery(UAVObject *object1) {
    // Double check that the field exists:
    QString voltage = QString("Voltage");
    QString current = QString("Current");
    QString energy = QString("ConsumedEnergy");
    UAVObjectField* field = object1->getField(voltage);
    UAVObjectField* field2 = object1->getField(current);
    UAVObjectField* field3 = object1->getField(energy);
    if (field && field2 && field3) {
        //QString s = QString();
        double v0 = field->getDouble();
        double v1 = field2->getDouble();
        double v2 = field3->getDouble();
        emit battery(v0,v1,v2);
    } else {
        qDebug() << "UpdateBattery: Wrong field, maybe an issue with object disconnection ?";
    }
    //qDebug() << "updateBattery";

}


