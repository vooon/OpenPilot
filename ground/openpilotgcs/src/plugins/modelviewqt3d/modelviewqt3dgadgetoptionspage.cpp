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
        ModelViewQt3DGadgetConfiguration *config
        , QObject *parent)
    : IOptionsPage(parent)
    , m_config(config)
{
    //
}

QWidget *ModelViewQt3DGadgetOptionsPage::createPage(QWidget *parent)
{
    m_page = new Ui::ModelViewQt3DOptionsPage();
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);

    // all supported formats
    QString filter = tr("3D Models (%1)")
                     .arg(QGLAbstractScene::supportedFormats()
                          .join(" "));

    m_page->modelPathChooser->setExpectedKind(Utils::PathChooser::File);
    m_page->modelPathChooser->setPromptDialogFilter(filter);
    m_page->modelPathChooser->setPromptDialogTitle(tr("Choose 3D model"));

    m_page->backgroundPathChooser->setExpectedKind(Utils::PathChooser::File);
    m_page->backgroundPathChooser->setPromptDialogFilter(tr("Images (*.png *.jpg *.bmp *.xpm)"));
    m_page->backgroundPathChooser->setPromptDialogTitle(tr("Choose background image"));

    m_page->modelPathChooser->setPath(m_config->acFilename());
    m_page->backgroundPathChooser->setPath(m_config->bgFilename());

    m_page->refreshRate->setValue(m_config->refreshRate());

    m_page->perspective->setChecked(m_config->perspective());
    m_page->orthographic->setChecked(!m_config->perspective());
    m_page->zoomGroup->setEnabled(m_config->perspective());

    m_page->fovZoom->setChecked(m_config->typeOfZoom());
    m_page->cameraMove->setChecked(!m_config->typeOfZoom());

    QBitArray opt = m_config->ppOptions();
    m_page->op_optimize->setChecked(opt.testBit(0));
    m_page->op_optimize2->setChecked(opt.testBit(1));
    m_page->op_nothing->setChecked(opt.testBit(2));
    m_page->op_calculatenormals->setChecked(opt.testBit(3));
    m_page->op_forcefaceted->setChecked(opt.testBit(4));
    m_page->op_fixnormals->setChecked(opt.testBit(5));
    m_page->op_fix1->setChecked(opt.testBit(6));
    m_page->op_fix2->setChecked(opt.testBit(7));

    return w;
}

void ModelViewQt3DGadgetOptionsPage::apply()
{
    m_config->setAcFilename(m_page->modelPathChooser->path());
    m_config->setBgFilename(m_page->backgroundPathChooser->path());
    m_config->setRefreshRate(m_page->refreshRate->value());
    m_config->setProjection(m_page->perspective->isChecked());
    m_config->setTypeOfZoom(m_page->fovZoom->isChecked());

    QBitArray opt(8);
    opt.setBit(0, m_page->op_optimize->isChecked());
    opt.setBit(1, m_page->op_optimize2->isChecked());
    opt.setBit(2, m_page->op_nothing->isChecked());
    opt.setBit(3, m_page->op_calculatenormals->isChecked());
    opt.setBit(4, m_page->op_forcefaceted->isChecked());
    opt.setBit(5, m_page->op_fixnormals->isChecked());
    opt.setBit(6, m_page->op_fix1->isChecked());
    opt.setBit(7, m_page->op_fix2->isChecked());
    m_config->setPostProcess(opt);
}

void ModelViewQt3DGadgetOptionsPage::finish()
{
    delete m_page;
}
