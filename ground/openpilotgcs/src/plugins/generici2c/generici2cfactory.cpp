/**
 ******************************************************************************
 *
 * @file       generici2cfactory.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup GenericI2CPlugin
 * @{
 * @brief A place holder gadget plugin
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
#include "generici2cfactory.h"
#include "generici2cwidget.h"
#include "generici2c.h"
#include <coreplugin/iuavgadget.h>

GenericI2CFactory::GenericI2CFactory(QObject *parent) :
    IUAVGadgetFactory(QString("GenericI2C"),
                      tr("Generic I2C compiler"),
                      parent)
{
}

GenericI2CFactory::~GenericI2CFactory()
{

}

IUAVGadget* GenericI2CFactory::createGadget(QWidget *parent) {
    GenericI2CWidget* gadgetWidget = new GenericI2CWidget(parent);
    return new GenericI2C(QString("GenericI2C"), gadgetWidget, parent);
}
