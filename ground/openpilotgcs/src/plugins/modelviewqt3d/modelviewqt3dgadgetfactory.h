/**
 ******************************************************************************
 *
 * @file       modelviewqt3dgadgetfactory.h
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

#ifndef MODELVIEWQT3DGADGETFACTORY_H_
#define MODELVIEWQT3DGADGETFACTORY_H_

#include <coreplugin/iuavgadgetfactory.h>

namespace Core {
    class IUAVGadget;
    class IUAVGadgetFactory;
}

using namespace Core;

class ModelViewQt3DGadgetFactory : public Core::IUAVGadgetFactory
{
    Q_OBJECT
public:
    ModelViewQt3DGadgetFactory(QObject *parent = 0);
    ~ModelViewQt3DGadgetFactory();

    Core::IUAVGadget *createGadget(QWidget *parent);
    IUAVGadgetConfiguration *createConfiguration(QSettings* qSettings);
    IOptionsPage *createOptionsPage(IUAVGadgetConfiguration *config);
};

#endif // MODELVIEWQT3DGADGETFACTORY_H_
