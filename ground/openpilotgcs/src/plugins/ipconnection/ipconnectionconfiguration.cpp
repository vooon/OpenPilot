/**
 ******************************************************************************
 *
 * @file       IPconnectionconfiguration.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup IPConnPlugin IP Telemetry Plugin
 * @{
 * @brief IP Connection Plugin impliment telemetry over TCP/IP and UDP/IP
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

#include "ipconnectionconfiguration.h"
#include <coreplugin/icore.h>
#include <QDebug>

QString IPconnectionConfiguration::m_StreamingAddress="127.0.0.1";
int IPconnectionConfiguration::m_StreamingPort=9000;
int IPconnectionConfiguration::m_StreamTelemetry=false;

//QString m_StreamingAddress;
//int m_StreamingPort;
//int m_StreamTelemetry;


IPconnectionConfiguration::IPconnectionConfiguration(QString classId, QSettings* qSettings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent),
    m_ExternalHwAddress("127.0.0.1"),
    m_ExternalHwPort(9000),
    m_UseTCP(1)
{
    Q_UNUSED(qSettings);

    settings = Core::ICore::instance()->settings();
}

IPconnectionConfiguration::~IPconnectionConfiguration()
{
}

IUAVGadgetConfiguration *IPconnectionConfiguration::clone()
{
    IPconnectionConfiguration *m = new IPconnectionConfiguration(this->classId());
    m->m_ExternalHwAddress = m_ExternalHwAddress;
    m->m_ExternalHwPort = m_ExternalHwPort;
    m->m_UseTCP = m_UseTCP;
    return m;
}

/**
 * Saves a configuration.
 *
 */
void IPconnectionConfiguration::saveConfig(QSettings* qSettings) const {
   qSettings->setValue("externalHwPort", m_ExternalHwPort);
   qSettings->setValue("externalHwAddress", m_ExternalHwAddress);
   qSettings->setValue("useTCP", m_UseTCP);
}

void IPconnectionConfiguration::savesettings() const
{
    settings->beginGroup(QLatin1String("IPconnection"));

        settings->beginWriteArray("Current");
        settings->setArrayIndex(0);
        settings->setValue(QLatin1String("ExternalHwAddress"), m_ExternalHwAddress);
        settings->setValue(QLatin1String("ExternalHwPort"), m_ExternalHwPort);
        settings->setValue(QLatin1String("UseTCP"), m_UseTCP);
        settings->setValue(QLatin1String("StreamingAddress"), m_StreamingAddress);
        settings->setValue(QLatin1String("StreamingPort"), m_StreamingPort);
        settings->setValue(QLatin1String("StreamTelemetry"), m_StreamTelemetry);
        settings->endArray();
        settings->endGroup();
}


void IPconnectionConfiguration::restoresettings()
{
    settings->beginGroup(QLatin1String("IPconnection"));

        settings->beginReadArray("Current");
        settings->setArrayIndex(0);
        m_ExternalHwAddress = (settings->value(QLatin1String("ExternalHwAddress"), tr("")).toString());
        m_ExternalHwPort = (settings->value(QLatin1String("ExternalHwPort"), tr("")).toInt());
        m_UseTCP = (settings->value(QLatin1String("UseTCP"), tr("")).toInt());
        m_StreamingAddress = (settings->value(QLatin1String("StreamingAddress"), tr("")).toString());
        m_StreamingPort = (settings->value(QLatin1String("StreamingPort"), tr("")).toInt());
        m_StreamTelemetry = (settings->value(QLatin1String("StreamTelemetry"), tr("")).toInt());

//        qDebug() << " IP:" << IPconnectionConfiguration::m_StreamingAddress << " Port: " << IPconnectionConfiguration::m_StreamingPort << " Telem:" << IPconnectionConfiguration::m_StreamTelemetry;

        settings->endArray();
        settings->endGroup();
}

