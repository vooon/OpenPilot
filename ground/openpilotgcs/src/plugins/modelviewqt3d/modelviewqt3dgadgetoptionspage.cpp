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

    m_page->worldPathChooser->setExpectedKind(Utils::PathChooser::File);
    m_page->worldPathChooser->setPromptDialogFilter(filter);
    m_page->worldPathChooser->setPromptDialogTitle(tr("Choose world model"));

    m_page->modelPathChooser->setPath(m_config->modelFilename());
    m_page->worldPathChooser->setPath(m_config->worldFilename());

    m_page->aaSamples->setValue(m_config->aaSamples());
    m_page->fpsLimiter->setValue(m_config->fpsLimiter());

    QBitArray opt = m_config->ppOptions();
    m_page->bit0->setChecked(opt.testBit(0));
    m_page->bit1->setChecked(opt.testBit(1));
    m_page->bit2->setChecked(opt.testBit(2));
    m_page->bit3->setChecked(opt.testBit(3));
//    m_page->bit4->setChecked(opt.testBit(4));
//    m_page->bit5->setChecked(opt.testBit(5));
//    m_page->bit6->setChecked(opt.testBit(6));
    m_page->bit7->setChecked(opt.testBit(7));

    return w;
}

void ModelViewQt3DGadgetOptionsPage::apply()
{
    m_config->setModelFile(m_page->modelPathChooser->path());
    m_config->setWorldFile(m_page->worldPathChooser->path());
    m_config->setAASamples(m_page->aaSamples->value());
    m_config->setFPSLimiter(m_page->fpsLimiter->value());

    QBitArray opt(8);
    opt.setBit(0, m_page->bit0->isChecked());
    opt.setBit(1, m_page->bit1->isChecked());
    opt.setBit(2, m_page->bit2->isChecked());
    opt.setBit(3, m_page->bit3->isChecked());
    opt.setBit(4, false);
    opt.setBit(5, false);
    opt.setBit(6, false);
    opt.setBit(7, m_page->bit7->isChecked());
    m_config->setPostProcess(opt);
}

void ModelViewQt3DGadgetOptionsPage::finish()
{
    delete m_page;
}
