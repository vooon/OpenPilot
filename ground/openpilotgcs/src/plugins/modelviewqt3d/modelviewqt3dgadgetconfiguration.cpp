/**
 ******************************************************************************
 *
 * @file       modelviewqt3dgadgetconfiguration.cpp
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

#include "modelviewqt3dgadgetconfiguration.h"
#include "utils/pathutils.h"

ModelViewQt3DGadgetConfiguration::ModelViewQt3DGadgetConfiguration(QString classId
                                                                   , QSettings* qSettings
                                                                   , QObject *parent)
    : IUAVGadgetConfiguration(classId, parent)
    , m_acFilename(":/modelview/models/warning_sign.obj")
    , m_bgFilename(":/modelview/models/black.jpg")
    , m_refreshRate(100)
    , m_perspective(1)
    , m_typeOfZoom(1)
{
    //if a saved configuration exists load it
    if(qSettings != 0) {
        QString modelFile = qSettings->value("acFilename").toString();
        QString bgFile = qSettings->value("bgFilename").toString();
        m_acFilename = Utils::PathUtils().InsertDataPath(modelFile);
        m_bgFilename = Utils::PathUtils().InsertDataPath(bgFile);
        m_refreshRate = qSettings->value("refreshRate").toInt();
        m_perspective = qSettings->value("perspective").toBool();
        m_typeOfZoom = qSettings->value("typeOfZoom").toBool();
    }
}

IUAVGadgetConfiguration *ModelViewQt3DGadgetConfiguration::clone()
{
    ModelViewQt3DGadgetConfiguration *mv = new ModelViewQt3DGadgetConfiguration(this->classId());
    mv->m_acFilename = m_acFilename;
    mv->m_bgFilename = m_bgFilename;
    mv->m_refreshRate = m_refreshRate;
    mv->m_perspective = m_perspective;
    mv->m_typeOfZoom = m_typeOfZoom;
    return mv;
}

/**
 * Saves a configuration.
 *
 */
void ModelViewQt3DGadgetConfiguration::saveConfig(QSettings* qSettings) const {
    qSettings->setValue("acFilename", Utils::PathUtils().RemoveDataPath(m_acFilename));
    qSettings->setValue("bgFilename", Utils::PathUtils().RemoveDataPath(m_bgFilename));
    qSettings->setValue("refreshRate", m_refreshRate);
    qSettings->setValue("perspective", m_perspective);
    qSettings->setValue("typeOfZoom", m_typeOfZoom);
}
