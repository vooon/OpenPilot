/**
 ******************************************************************************
 *
 * @file       IPconnectionconfiguration.h
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

#ifndef IPconnectionCONFIGURATION_H
#define IPconnectionCONFIGURATION_H

#include <coreplugin/iuavgadgetconfiguration.h>
#include <QtCore/QString>
#include <QtCore/QSettings>

using namespace Core;

class IPconnectionConfiguration : public IUAVGadgetConfiguration
{
Q_OBJECT
Q_PROPERTY(QString ExternalHwAddress READ getExternalHwAddress WRITE setExternalHwAddress)
Q_PROPERTY(int ExternalHwPort READ getExternalHwPort WRITE setExternalHwPort)
Q_PROPERTY(int UseTCP READ getUseTCP WRITE setUseTCP)

public:
    explicit IPconnectionConfiguration(QString classId, QSettings* qSettings = 0, QObject *parent = 0);

    virtual ~IPconnectionConfiguration();
    void saveConfig(QSettings* settings) const;
    //void savesettings(QSettings* settings) const;
    //void restoresettings(QSettings* settings);
    void savesettings() const;
    void restoresettings();
    IUAVGadgetConfiguration *clone();

    QString getExternalHwAddress() const { return m_ExternalHwAddress; }
    int getExternalHwPort() const { return m_ExternalHwPort; }
    int getUseTCP() const { return m_UseTCP; }

     QString getStreamingAddress()  { return m_StreamingAddress; }
     int getStreamingPort()  { return m_StreamingPort; }
     int getStreamTelemetry()  { return m_StreamTelemetry; }

public slots:
    void setExternalHwAddress(QString IPAddress) { m_ExternalHwAddress = IPAddress; }
    void setExternalHwPort(int Port) { m_ExternalHwPort = Port; }
    void setUseTCP(int UseTCP) { m_UseTCP = UseTCP; }

    void setStreamingAddress(QString IPAddress) { m_StreamingAddress = IPAddress; }
    void setStreamingPort(int Port) { m_StreamingPort = Port; }
    void setStreamTelemetry(int streamTelemetry) { m_StreamTelemetry = streamTelemetry; }

private:
    QString m_ExternalHwAddress;
    int m_ExternalHwPort;
    int m_UseTCP;

    QString m_StreamingAddress;
    int m_StreamingPort;
    int m_StreamTelemetry;

    QSettings* settings;
};

#endif // IPconnectionCONFIGURATION_H
