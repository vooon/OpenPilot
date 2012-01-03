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
#include <QBitArray>

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

    QString modelFilename()             { return m_modelFilename; }
    QString worldFilename()             { return m_worldFilename; }
    quint8 aaSamples()                  { return m_aaSamples; }
    quint8 fpsLimiter()                 { return m_fpsLimiter; }
    QBitArray ppOptions()               { return m_ppOptions; }

    void setModelFile(QString mFile)    { m_modelFilename = mFile; }
    void setWorldFile(QString wFile)    { m_worldFilename = wFile; }
    void setAASamples(quint8 aa)        { m_aaSamples = aa; }
    void setFPSLimiter(quint8 fps)      { m_fpsLimiter = fps; }
    void setPostProcess(QBitArray opt)  { m_ppOptions = opt; }

private:
    QString m_modelFilename;
    QString m_worldFilename;
    quint8 m_aaSamples;
    quint8 m_fpsLimiter;
    QBitArray m_ppOptions;
};

#endif // MODELVIEWQT3DGADGETCONFIGURATION_H
