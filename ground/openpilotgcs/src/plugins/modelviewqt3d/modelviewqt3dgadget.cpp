/**
 ******************************************************************************
 *
 * @file       modelviewqt3dgadget.cpp
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
#include "modelviewqt3dgadget.h"
#include "modelviewqt3dgadgetwidget.h"
#include "modelviewqt3dgadgetconfiguration.h"

ModelViewQt3DGadget::ModelViewQt3DGadget(QString classId
                                         , ModelViewQt3DGadgetWidget *widget
                                         , QWidget *parent)
    : IUAVGadget(classId, parent)
    , m_widget(widget)
{
    //
}

ModelViewQt3DGadget::~ModelViewQt3DGadget()
{
    delete m_widget;
}

void ModelViewQt3DGadget::loadConfiguration(IUAVGadgetConfiguration* config)
{
    ModelViewQt3DGadgetConfiguration *m = qobject_cast<ModelViewQt3DGadgetConfiguration*>(config);
    m_widget->setLoadedConfig(m->modelFilename()
                              , m->worldFilename()
                              , m->aaSamples()
                              , m->fpsLimiter()
                              , m->ppOptions()
                              );
}
