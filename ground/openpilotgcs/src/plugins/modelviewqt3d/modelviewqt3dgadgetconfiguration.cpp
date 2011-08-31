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
    , m_modelFilename("")
    , m_worldFilename("")
    , m_aaSamples(0)
    , m_fpsLimiter(10)
    , m_ppOptions(8)
{
    //if a saved configuration exists load it
    if(qSettings != 0) {
        QString modelFile = qSettings->value("modelFilename").toString();
        QString worldFile = qSettings->value("worldFilename").toString();
        m_modelFilename = Utils::PathUtils().InsertDataPath(modelFile);
        m_worldFilename = Utils::PathUtils().InsertDataPath(worldFile);
        m_aaSamples = qSettings->value("aaSamples").toInt();
        m_fpsLimiter = qSettings->value("fpsLimiter").toInt();
        m_ppOptions = qSettings->value("ppOptions").toBitArray();
    }
}

IUAVGadgetConfiguration *ModelViewQt3DGadgetConfiguration::clone()
{
    ModelViewQt3DGadgetConfiguration *mv = new ModelViewQt3DGadgetConfiguration(this->classId());
    mv->m_modelFilename = m_modelFilename;
    mv->m_worldFilename = m_worldFilename;
    mv->m_fpsLimiter = m_fpsLimiter;
    mv->m_aaSamples = m_aaSamples;
    mv->m_ppOptions = m_ppOptions;
    return mv;
}

void ModelViewQt3DGadgetConfiguration::saveConfig(QSettings* qSettings) const
{
    qSettings->setValue("modelFilename", Utils::PathUtils().RemoveDataPath(m_modelFilename));
    qSettings->setValue("worldFilename", Utils::PathUtils().RemoveDataPath(m_worldFilename));
    qSettings->setValue("aaSamples", m_aaSamples);
    qSettings->setValue("fpsLimiter", m_fpsLimiter);
    qSettings->setValue("ppOptions", m_ppOptions);
}
