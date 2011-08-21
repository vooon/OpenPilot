/**
 ******************************************************************************
 *
 * @file       modelviewqt3dgadgetfactory.cpp
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
#include "modelviewqt3dgadgetfactory.h"
#include "modelviewqt3dgadgetwidget.h"
#include "modelviewqt3dgadget.h"
#include "modelviewqt3dgadgetconfiguration.h"
#include "modelviewqt3dgadgetoptionspage.h"
#include <coreplugin/uavgadgetoptionspagedecorator.h>
#include <coreplugin/iuavgadget.h>

ModelViewQt3DGadgetFactory::ModelViewQt3DGadgetFactory(QObject *parent) :
    IUAVGadgetFactory(QString("ModelViewQt3DGadget"), tr("ModelView Qt3D"), parent)
{
}

ModelViewQt3DGadgetFactory::~ModelViewQt3DGadgetFactory()
{
}

Core::IUAVGadget* ModelViewQt3DGadgetFactory::createGadget(QWidget *parent)
{
    ModelViewQt3DGadgetWidget* gadgetWidget = new ModelViewQt3DGadgetWidget(parent);
    return new ModelViewQt3DGadget(QString("ModelViewQt3DGadget"), gadgetWidget, parent);
}

IUAVGadgetConfiguration *ModelViewQt3DGadgetFactory::createConfiguration(QSettings* qSettings)
{
    return new ModelViewQt3DGadgetConfiguration(QString("ModelViewQt3DGadget"), qSettings);
}

IOptionsPage *ModelViewQt3DGadgetFactory::createOptionsPage(IUAVGadgetConfiguration *config)
{
    return new ModelViewQt3DGadgetOptionsPage(qobject_cast<ModelViewQt3DGadgetConfiguration*>(config));
}

