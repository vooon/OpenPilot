/**
 ******************************************************************************
 *
 * @file       modelviewqt3dgadgetconfiguration.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ModelViewPlugin ModelView Plugin
 * @{
 * @brief A gadget that displays a 3D representation of the UAV
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

#ifndef MODELVIEWQT3DGADGETCONFIGURATION_H
#define MODELVIEWQT3DGADGETCONFIGURATION_H

#include <coreplugin/iuavgadgetconfiguration.h>

using namespace Core;

class ModelViewQt3DGadgetConfiguration : public IUAVGadgetConfiguration
{
    Q_OBJECT

public:
    explicit ModelViewQt3DGadgetConfiguration(QString classId
                                              , QSettings* qSettings = 0
                                              , QObject *parent = 0);

    void saveConfig(QSettings* settings) const;
    IUAVGadgetConfiguration *clone();

    QString acFilename() { return m_acFilename; }
    QString bgFilename() { return m_bgFilename; }
    quint16 refreshRate() { return m_refreshRate; }
    bool perspective() { return m_perspective; }
    bool typeOfZoom() { return m_typeOfZoom; }

    void setAcFilename(QString acFile) { m_acFilename = acFile; }
    void setBgFilename(QString bgFile) { m_bgFilename = bgFile; }
    void setRefreshRate(quint16 ref) { m_refreshRate = ref; }
    void setProjection(bool proj) { m_perspective = proj; }
    void setTypeOfZoom(bool zoom) { m_typeOfZoom = zoom; }

signals:

public slots:

private:
    QString m_acFilename;
    QString m_bgFilename;
    quint16 m_refreshRate;
    bool m_perspective;     // 1 - perspective, 0 - orthographic
    bool m_typeOfZoom;      // 1 - FOV zoom, 0 - camera move
};

#endif // MODELVIEWQT3DGADGETCONFIGURATION_H
