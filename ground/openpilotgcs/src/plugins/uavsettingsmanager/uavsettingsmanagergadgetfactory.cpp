/**
 ******************************************************************************
 *
 * @file       uavsettingsmanagergadgetfactory.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @brief      Factory for UAV Settings Manager Plugin
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup   uavsettingsmanagerplugin
 * @{
 *
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
#include "uavsettingsmanagergadgetfactory.h"
#include "uavsettingsmanagergadgetwidget.h"
#include "uavsettingsmanagergadget.h"
#include "uavsettingsmanagergadgetconfiguration.h"
#include "uavsettingsmanagergadgetoptionspage.h"
#include <coreplugin/iuavgadget.h>

UAVSettingsManagerGadgetFactory::UAVSettingsManagerGadgetFactory(QObject *parent) :
        IUAVGadgetFactory(QString("UAVSettingsManagerGadget"),
                          tr("UAV Settings Manager"),
                          parent)
{
}

UAVSettingsManagerGadgetFactory::~UAVSettingsManagerGadgetFactory()
{
}

Core::IUAVGadget* UAVSettingsManagerGadgetFactory::createGadget(QWidget *parent)
{
    UAVSettingsManagerGadgetWidget* gadgetWidget = new UAVSettingsManagerGadgetWidget(parent);
    return new UAVSettingsManagerGadget(QString("UAVSettingsManagerGadget"), gadgetWidget, parent);
}

IUAVGadgetConfiguration *UAVSettingsManagerGadgetFactory::createConfiguration(QSettings* qSettings, UAVConfigInfo *configInfo)
{
    lastConfig = new UAVSettingsManagerGadgetConfiguration(QString("UAVSettingsManagerGadget"), qSettings, configInfo);
    return lastConfig;
}

IOptionsPage *UAVSettingsManagerGadgetFactory::createOptionsPage(IUAVGadgetConfiguration *config)
{
    return new UAVSettingsManagerGadgetOptionsPage(qobject_cast<UAVSettingsManagerGadgetConfiguration*>(config));
}

/**
 * @}
 * @}
 */
