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
    qDebug() << this << "ModelViewQt3DGadgetWidget";

    ExtensionSystem::PluginManager* pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager* objManager = pm->getObject<UAVObjectManager>();
    attActual = AttitudeActual::GetInstance(objManager);

    m_model = NULL;
    modelviewLayout = new QHBoxLayout(this);
    modelviewLayout->setMargin(2);

    timer = startTimer(500);
}

ModelViewQt3DGadgetWidget::~ModelViewQt3DGadgetWidget()
{
    qDebug() << this << "~ModelViewQt3DGadgetWidget";
}

void ModelViewQt3DGadgetWidget::setLoadedConfig(QString mfn, QString wfn
                                                , int aas, int fps
                                                , QBitArray opt)
{
    if (mfn == "")
        return;
    qDebug() << this << "setLoadedConfig";

    if (m_model)
        delete m_model;
    setQGLFormat(aas, fps);
    m_model = new ModelViewQt3D(QGLFormat::defaultFormat());
    m_model->setFocusPolicy(Qt::StrongFocus);
    m_model->setMinimumSize(128, 128);
    m_model->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    modelviewLayout->addWidget(m_model);

    m_model->setModelFile(mfn);
    m_model->setWorldFile(wfn);
    m_model->setCameraEye(QVector3D(0, -1, -2.5));
    m_model->setPostprocess(opt);

    killTimer(timer);
    qBound(1, fps, 60);
    timer = startTimer(1000/fps);

    connect(m_model, SIGNAL(focusIsChanged(bool)), this, SLOT(onModelViewFocused(bool)));
}

// private slots

void ModelViewQt3DGadgetWidget::onModelViewFocused(bool focus)
{
    if (focus) {
        //
    } else {
        //
    }
}

// private

void ModelViewQt3DGadgetWidget::timerEvent(QTimerEvent *)
{
    updateAttitude();
}

void ModelViewQt3DGadgetWidget::setQGLFormat(int samples, int fpsLimit)
{
    qDebug() << this << "setGLFormat";
    QGLFormat f;

/*    f.setAlpha(TRUE);
    f.setAccum(TRUE);
    f.setAccumBufferSize(64);
    f.setDoubleBuffer(TRUE);
    f.setDepth(TRUE);
    f.setDepthBufferSize(24);
    f.setDirectRendering(TRUE);
    f.setRgba(TRUE);
    f.setRedBufferSize(8);
    f.setGreenBufferSize(8);
    f.setBlueBufferSize(8);
    f.setAlphaBufferSize(8);
    f.setOverlay(FALSE);
    f.setPlane(0);
    f.setStencil(TRUE);
    f.setStencilBufferSize(8);
    f.setStereo(FALSE);*/

    qBound(0, samples, 16);
    if (samples > 0) {
        f.setSampleBuffers(TRUE);
        f.setSamples(samples);
    } else {
        f.setSampleBuffers(FALSE);
        f.setSamples(0);
    }

//    qBound(0, fpsLimit, 6);
//    f.setSwapInterval(fpsLimit);
    Q_UNUSED(fpsLimit);
    f.setSwapInterval(1);

    QGLFormat::setDefaultFormat(f);
}

void ModelViewQt3DGadgetWidget::updateAttitude()
{
    AttitudeActual::DataFields data = attActual->getData();
    double w = data.q1;
    if (w == 0.0)
        w = 1.0;
    QQuaternion att = QQuaternion(w, data.q3, -data.q4, -data.q2);
    QQuaternion rot = QQuaternion().fromAxisAndAngle(0, 1, 0, 180);
    att *= rot;

    if (m_model->getAttitude() == att)
        return;

    m_model->setAttitude(att);
    m_model->updateAttitude();
}
