/**
 ******************************************************************************
 *
 * @file       uavsettingsmanagerplugin.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @brief      UAV Settings Manager Plugin
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @defgroup   uavsettingsmanagerplugin
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

#include "uavsettingsmanagerplugin.h"
#include "uavsettingsmanagergadgetfactory.h"
#include "uavsettingsmanagerdialog.h"
#include <QDebug>
#include <QtPlugin>
#include <QStringList>
#include <extensionsystem/pluginmanager.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <QKeySequence>


UAVSettingsManagerPlugin::UAVSettingsManagerPlugin()
{
    // Do nothing
}

UAVSettingsManagerPlugin::~UAVSettingsManagerPlugin()
{
    // Do nothing
}

bool UAVSettingsManagerPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);
    mf = new UAVSettingsManagerGadgetFactory(this);
    addAutoReleasedObject(mf);

    // Add Menu entry
    Core::ActionManager* am = Core::ICore::instance()->actionManager();
    Core::ActionContainer* ac = am->actionContainer(Core::Constants::M_FILE);

    Core::Command* cmd = am->registerAction(new QAction(this),
                                            "UAVSettingsManagerPlugin.UAVSettingsManager",
                                            QList<int>() <<
                                            Core::Constants::C_GLOBAL_ID);
    cmd->setDefaultKeySequence(QKeySequence("Ctrl+I"));
    cmd->action()->setText(tr("UAV Settings Manager..."));

//    ac->menu()->addSeparator();
//    ac->appendGroup("UAVSettingsManager");
//    ac->addAction(cmd, "UAVSettingsManager");
    ac->addAction(cmd, Core::Constants::G_FILE_SAVE);


    connect(cmd->action(), SIGNAL(triggered(bool)), this, SLOT(importExport()));

    return true;
}

void UAVSettingsManagerPlugin::importExport()
{
    UAVSettingsManagerDialog(mf->getLastConfig()).exec();
}

void UAVSettingsManagerPlugin::extensionsInitialized()
{
    // Do nothing
}

void UAVSettingsManagerPlugin::shutdown()
{
    // Do nothing
}
Q_EXPORT_PLUGIN(UAVSettingsManagerPlugin)

/**
 * @}
 * @}
 */
