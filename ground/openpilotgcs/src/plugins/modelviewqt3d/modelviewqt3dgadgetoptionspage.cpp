/**
 ******************************************************************************
 *
 * @file       modelviewqt3dgadgetoptionspage.cpp
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

#include "modelviewqt3dgadgetoptionspage.h"
#include "modelviewqt3dgadgetconfiguration.h"
#include "ui_modelviewqt3doptionspage.h"

ModelViewQt3DGadgetOptionsPage::ModelViewQt3DGadgetOptionsPage(
        ModelViewQt3DGadgetConfiguration *config, QObject *parent) :
    IOptionsPage(parent),
    m_config(config)
{
}

QWidget *ModelViewQt3DGadgetOptionsPage::createPage(QWidget *parent)
{
    m_page = new Ui::ModelViewQt3DOptionsPage();
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);

    QString filter = tr("3D Models (%1)").arg(QGLAbstractScene::supportedFormats().join(" "));

    m_page->modelPathChooser->setExpectedKind(Utils::PathChooser::File);
    m_page->modelPathChooser->setPromptDialogFilter(filter);
    m_page->modelPathChooser->setPromptDialogTitle(tr("Choose 3D model"));
    m_page->backgroundPathChooser->setExpectedKind(Utils::PathChooser::File);
    m_page->backgroundPathChooser->setPromptDialogFilter(tr("Images (*.png *.jpg *.bmp *.xpm)"));
    m_page->backgroundPathChooser->setPromptDialogTitle(tr("Choose background image"));

    m_page->modelPathChooser->setPath(m_config->acFilename());
    m_page->backgroundPathChooser->setPath(m_config->bgFilename());

    return w;
}

void ModelViewQt3DGadgetOptionsPage::apply()
{
    m_config->setAcFilename(m_page->modelPathChooser->path());
    m_config->setBgFilename(m_page->backgroundPathChooser->path());
}

void ModelViewQt3DGadgetOptionsPage::finish()
{
    delete m_page;
}
