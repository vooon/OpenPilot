/**
 ******************************************************************************
 *
 * @file       uavsettingsmanagergadget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @brief      Gadget for UAV Settings Manager Plugin
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

#include "uavsettingsmanagergadget.h"
#include "uavsettingsmanagergadgetwidget.h"
#include "uavsettingsmanagergadgetconfiguration.h"

UAVSettingsManagerGadget::UAVSettingsManagerGadget(QString classId, UAVSettingsManagerGadgetWidget *widget, QWidget *parent) :
        IUAVGadget(classId, parent),
        m_widget(widget)
{
}

UAVSettingsManagerGadget::~UAVSettingsManagerGadget()
{
    delete m_widget;
}

/*
  This is called when a configuration is loaded, and updates the plugin's settings.
  Careful: the plugin is already drawn before the loadConfiguration method is called the
  first time, so you have to be careful not to assume all the plugin values are initialized
  the first time you use them
 */
void UAVSettingsManagerGadget::loadConfiguration(IUAVGadgetConfiguration* config)
{
    m_widget->loadConfiguration(qobject_cast<UAVSettingsManagerGadgetConfiguration*>(config));
}
/**
 * @}
 * @}
 */
