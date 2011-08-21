/**
 ******************************************************************************
 *
 * @file       modelviewqt3dgadgetoptionspage.h
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

#ifndef MODELVIEWQT3DGADGETOPTIONSPAGE_H
#define MODELVIEWQT3DGADGETOPTIONSPAGE_H

#include "coreplugin/dialogs/ioptionspage.h"
#include <qglabstractscene.h>

class ModelViewQt3DGadgetConfiguration;
class QFileDialog;
namespace Core {
    class IUAVGadgetConfiguration;
}
namespace Ui {
    class ModelViewQt3DOptionsPage;
}

using namespace Core;

class ModelViewQt3DGadgetOptionsPage : public IOptionsPage
{
    Q_OBJECT

public:
    explicit ModelViewQt3DGadgetOptionsPage(ModelViewQt3DGadgetConfiguration *config, QObject *parent = 0);
    QString id() const { return ""; }
    QString trName() const { return ""; }
    QString category() const { return ""; }
    QString trCategory() const { return ""; }

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
private:

signals:

public slots:

private slots:

private:
    ModelViewQt3DGadgetConfiguration *m_config;
    Ui::ModelViewQt3DOptionsPage *m_page;
};

#endif // MODELVIEWQT3DGADGETOPTIONSPAGE_H
