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
#ifndef UAVSETTINGSMANAGERDIALOG_H
#define UAVSETTINGSMANAGERDIALOG_H

#include <QDialog>
#include "uavsettingsmanagergadgetconfiguration.h"

namespace Ui {
    class UAVSettingsManagerDialog;
}

class UAVSettingsManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UAVSettingsManagerDialog( UAVSettingsManagerGadgetConfiguration *config, QWidget *parent = 0);
    ~UAVSettingsManagerDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::UAVSettingsManagerDialog *ui;
};

#endif // UAVSETTINGSMANAGERDIALOG_H

/**
 * @}
 * @}
 */
