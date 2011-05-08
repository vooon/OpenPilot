/**
 ******************************************************************************
 * @file
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup uavsettingsmanagerplugin
 * @{
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

#ifndef UAVSETTINGSMANAGERGADGETOPTIONSPAGE_H
#define UAVSETTINGSMANAGERGADGETOPTIONSPAGE_H

#include "uavsettingsmanager_global.h"
#include "coreplugin/dialogs/ioptionspage.h"
#include <QString>
#include <QFont>
#include <QStringList>
#include <QDebug>

namespace Core
{
class IUAVGadgetConfiguration;
}

class UAVSettingsManagerGadgetConfiguration;

namespace Ui
{
class UAVSettingsManagerGadgetOptionsPage;
}

using namespace Core;

class UAVSETTINGSMANAGER_EXPORT UAVSettingsManagerGadgetOptionsPage : public IOptionsPage
{
    Q_OBJECT
public:
    explicit UAVSettingsManagerGadgetOptionsPage(UAVSettingsManagerGadgetConfiguration *config, QObject *parent = 0);

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();

private:
    Ui::UAVSettingsManagerGadgetOptionsPage *options_page;
    UAVSettingsManagerGadgetConfiguration *m_config;
    QFont font;

private slots:

};

#endif // UAVSETTINGSMANAGERGADGETOPTIONSPAGE_H
/**
 * @}
 * @}
 */
