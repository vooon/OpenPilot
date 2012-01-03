/**
 ******************************************************************************
 *
 * @file       modelviewqt3dgadgetwidget.h
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

#ifndef MODELVIEWQT3DGADGETWIDGET_H_
#define MODELVIEWQT3DGADGETWIDGET_H_

#include <QHBoxLayout>
#include "uavobjectmanager.h"
#include "attitudeactual.h"
#include "modelviewqt3d.h"

class ModelViewQt3DGadgetWidget : public QWidget
{
    Q_OBJECT

public:
    ModelViewQt3DGadgetWidget(QWidget *parent = 0);
    ~ModelViewQt3DGadgetWidget();
    void setLoadedConfig(QString mfn
                         , QString wfn
                         , int aas
                         , int fps
                         , QBitArray opt);
private slots:
    void onModelViewFocused(bool focus);

private:
    int timer;
    void timerEvent(QTimerEvent *);
    QHBoxLayout *modelviewLayout;
    ModelViewQt3D* m_model;
    void setQGLFormat(int samples = 0, int fpsLimit = 3);
    void updateAttitude();

    AttitudeActual* attActual;
};

#endif /* MODELVIEWQT3DGADGETWIDGET_H_ */
