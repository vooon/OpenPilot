/**
 ******************************************************************************
 *
 * @file       emptygadgetfactory.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup EmptyGadgetPlugin Empty Gadget Plugin
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
#include "dviewgadgetfactory.h"
#include "dviewwidget.h"
#include "dviewgadget.h"
#include "dviewgadgetconfiguration.h"
//#include "dviewgadgetoptionspage.h"
#include <coreplugin/iuavgadget.h>

DViewGadgetFactory::DViewGadgetFactory(QObject *parent) :
        IUAVGadgetFactory(QString("DViewGadget"),
                          tr("Data View Gadget"),
                          parent)
{
}

DViewGadgetFactory::~DViewGadgetFactory()
{

}

IUAVGadget* DViewGadgetFactory::createGadget(QWidget *parent) {
    DViewWidget* gadgetWidget = new DViewWidget(parent);
    return new DViewGadget(QString("DViewGadget"), gadgetWidget, parent);
}

IUAVGadgetConfiguration *DViewGadgetFactory::createConfiguration(QSettings* qSettings)
{
    return new DViewGadgetConfiguration(QString("DViewGadget"), qSettings);
}

//IOptionsPage *DViewGadgetFactory::createOptionsPage(IUAVGadgetConfiguration *config)
//{
//    return new DViewGadgetOptionsPage(qobject_cast<DViewGadgetConfiguration*>(config));
//}
