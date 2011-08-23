/**
 ******************************************************************************
 *
 * @file       modelviewqt3dgadgetwidget.cpp
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
#include "modelviewqt3dgadgetwidget.h"
#include "extensionsystem/pluginmanager.h"

ModelViewQt3DGadgetWidget::ModelViewQt3DGadgetWidget(QWidget *parent)
    : QWidget(parent)
{
    qDebug() << "ModelViewQt3DGadgetWidget";

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

//  add modelview in layout
    {
        m_model = new ModelView(this);
        QHBoxLayout *layout = new QHBoxLayout;
        layout->setMargin(2);
        layout->addWidget(m_model);
        setLayout(layout);
    }

    ExtensionSystem::PluginManager* pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager* objManager = pm->getObject<UAVObjectManager>();
    attActual = AttitudeActual::GetInstance(objManager);

    refreshTimer = startTimer(500);
}

ModelViewQt3DGadgetWidget::~ModelViewQt3DGadgetWidget()
{
    qDebug() << "~ModelViewQt3DGadgetWidget";
}

void ModelViewQt3DGadgetWidget::setAcFilename(QString acf)
{
    if(!QFile::exists(acf))
        acf = ":/modelview/models/warning_sign.obj";
    m_model->setModelFile(acf);
}

void ModelViewQt3DGadgetWidget::setBgFilename(QString bgf)
{
    if (!QFile::exists(bgf))
        bgf = ":/modelview/models/black.jpg";
    m_model->setBackground(bgf);
}

void ModelViewQt3DGadgetWidget::setRefreshRate(quint16 rate)
{
    killTimer(refreshTimer);
    refreshTimer = startTimer(rate);
}

void ModelViewQt3DGadgetWidget::setProjection(bool perspective)
{
    m_model->setProjection(perspective);
}

void ModelViewQt3DGadgetWidget::setTypeOfZoom(bool zoom)
{
    m_model->setTypeOfZoom(zoom);
}

void ModelViewQt3DGadgetWidget::reloadModel()
{
    m_model->reloadModel();
}

void ModelViewQt3DGadgetWidget::timerEvent(QTimerEvent *)
{
    updateAttitude();
}

void ModelViewQt3DGadgetWidget::updateAttitude()
{
    AttitudeActual::DataFields data = attActual->getData();

    double w= data.q1;
    if (w == 0.0)
        w = 1.0;
    QQuaternion att = QQuaternion(w, -data.q3, -data.q4, data.q2);
    m_model->setAttitude(att);
    m_model->updateAttitude();
}
