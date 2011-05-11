/**
 ******************************************************************************
 *
 * @file       uavsettingsmanagergadgetoptionspage.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @brief      Option page for UAV Settings Manager Plugin
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

#include "uavsettingsmanagergadgetoptionspage.h"
#include "uavsettingsmanagergadgetconfiguration.h"
#include "ui_uavsettingsmanagergadgetoptionspage.h"

#include <QFileDialog>
#include <QtDebug>

UAVSettingsManagerGadgetOptionsPage::UAVSettingsManagerGadgetOptionsPage(UAVSettingsManagerGadgetConfiguration *config, QObject *parent) :
        IOptionsPage(parent),
        m_config(config)
{
}

//creates options page widget (uses the UI file)
QWidget *UAVSettingsManagerGadgetOptionsPage::createPage(QWidget *parent)
{

    options_page = new Ui::UAVSettingsManagerGadgetOptionsPage();
    //main widget
    QWidget *optionsPageWidget = new QWidget(parent);
    //main layout
    options_page->setupUi(optionsPageWidget);

    return optionsPageWidget;
}

/**
 * Called when the user presses apply or OK.
 *
 * Saves the current values
 *
 */
void UAVSettingsManagerGadgetOptionsPage::apply()
{
}


void UAVSettingsManagerGadgetOptionsPage::finish()
{

}
/**
 * @}
 * @}
 */
