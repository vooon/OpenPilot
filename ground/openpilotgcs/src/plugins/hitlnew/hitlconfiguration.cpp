/**
 ******************************************************************************
 *
 * @file       hitlconfiguration.cpp
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

#include "hitlconfiguration.h"

HITLConfiguration::HITLConfiguration(QString classId, QSettings* qSettings, QObject *parent) :
        IUAVGadgetConfiguration(classId, parent)
{
        settings.simulatorId = "";
        settings.hostAddress = "127.0.0.1";
        settings.remoteHostAddress = "127.0.0.1";
        settings.outPort = 0;
        settings.inPort = 0;
    settings.isUdp = TRUE;

    settings.manual = FALSE;
    settings.isActDes = TRUE;
    settings.isAttAct = TRUE;
    settings.isAttRaw = TRUE;
    settings.isBaroAlt= FALSE;
    settings.isGPSPos = FALSE;
    settings.isPosAct = FALSE;
    settings.isVelAct = FALSE;
    settings.isHomeLoc = FALSE;
    settings.actDesRate = 50;
    settings.attActRate = 50;
    settings.attRawRate = 50;
    settings.baroAltRate = 10;
    settings.GPSPosRate = 10;
    settings.posActRate = 10;
    settings.velActRate = 10;
    settings.homeLocRate = 1;

    settings.latitude = 0.f;
    settings.longitude = 0.f;

    settings.startSim = false;
    settings.binPath = "";
    settings.dataPath = "";

        //if a saved configuration exists load it
        if(qSettings != 0) {
                settings.simulatorId = qSettings->value("simulatorId").toString();

                settings.hostAddress = qSettings->value("hostAddress").toString();
                settings.remoteHostAddress = qSettings->value("remoteHostAddress").toString();
                settings.outPort = qSettings->value("outPort").toInt();
                settings.inPort = qSettings->value("inPort").toInt();
        settings.isUdp = qSettings->value("protocolUdp").toBool();

        settings.manual = qSettings->value("manual").toBool();
        settings.isActDes = qSettings->value("isActDes").toBool();
        settings.isAttAct = qSettings->value("isAttAct").toBool();
        settings.isAttRaw = qSettings->value("isAttRaw").toBool();
        settings.isBaroAlt = qSettings->value("isBaroAlt").toBool();
        settings.isGPSPos = qSettings->value("isGPSPos").toBool();
        settings.isPosAct = qSettings->value("isPosAct").toBool();
        settings.isVelAct = qSettings->value("isVelAct").toBool();
        settings.isHomeLoc = qSettings->value("isHomeLoc").toBool();

        settings.actDesRate = qSettings->value("actDesRate").toInt();
        settings.attActRate = qSettings->value("attActRate").toInt();
        settings.attRawRate = qSettings->value("attRawRate").toInt();
        settings.isBaroAlt = qSettings->value("isBaroAlt").toInt();
        settings.GPSPosRate = qSettings->value("GPSPosRate").toInt();
        settings.posActRate = qSettings->value("posActRate").toInt();
        settings.velActRate = qSettings->value("velActRate").toInt();
        settings.homeLocRate = qSettings->value("homeLocRate").toInt();

        settings.latitude = qSettings->value("latitude").toFloat();
        settings.longitude = qSettings->value("longitude").toFloat();

        settings.startSim = qSettings->value("startSim").toBool();
        settings.binPath = qSettings->value("binPath").toString();
        settings.dataPath = qSettings->value("dataPath").toString();
        }
}

IUAVGadgetConfiguration *HITLConfiguration::clone()
{
	HITLConfiguration *m = new HITLConfiguration(this->classId());
	m->settings = settings;
    return m;
}

 /**
  * Saves a configuration.
  *
  */
void HITLConfiguration::saveConfig(QSettings* qSettings) const
{
    qSettings->setValue("simulatorId", settings.simulatorId);

    qSettings->setValue("hostAddress", settings.hostAddress);
    qSettings->setValue("remoteHostAddress", settings.remoteHostAddress);
    qSettings->setValue("outPort", settings.outPort);
    qSettings->setValue("inPort", settings.inPort);
    qSettings->setValue("protocolUdp", settings.isUdp);

    qSettings->setValue("manual", settings.manual);
    qSettings->setValue("isActDes", settings.isActDes);
    qSettings->setValue("isAttAct", settings.isAttAct);
    qSettings->setValue("isAttRaw", settings.isAttRaw);
    qSettings->setValue("isBaroAlt", settings.isBaroAlt);
    qSettings->setValue("isGPSPos", settings.isGPSPos);
    qSettings->setValue("isPosAct", settings.isPosAct);
    qSettings->setValue("isVelAct", settings.isVelAct);
    qSettings->setValue("isHomeLoc", settings.isHomeLoc);

    qSettings->setValue("actDesRate", settings.actDesRate);
    qSettings->setValue("attActRate", settings.attActRate);
    qSettings->setValue("attRawRate", settings.attRawRate);
    qSettings->setValue("baroAltRate", settings.baroAltRate);
    qSettings->setValue("GPSPosRate", settings.GPSPosRate);
    qSettings->setValue("posActRate", settings.posActRate);
    qSettings->setValue("velActRate", settings.velActRate);
    qSettings->setValue("homeLocRate", settings.homeLocRate);

    qSettings->setValue("latitude", settings.latitude);
    qSettings->setValue("longitude", settings.longitude);

    qSettings->setValue("startSim", settings.startSim);
    qSettings->setValue("binPath", settings.binPath);
    qSettings->setValue("dataPath", settings.dataPath);
}

