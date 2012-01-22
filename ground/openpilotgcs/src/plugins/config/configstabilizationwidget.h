/**
 ******************************************************************************
 *
 * @file       configstabilizationwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Stabilization configuration panel
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
#ifndef CONFIGSTABILIZATIONWIDGET_H
#define CONFIGSTABILIZATIONWIDGET_H

#include "ui_stabilization.h"
#include "configtaskwidget.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include "stabilizationsettings.h"
#include <QtGui/QWidget>
#include <QTimer>

class ConfigStabilizationWidget: public ConfigTaskWidget
{
    Q_OBJECT

public:
    ConfigStabilizationWidget(QWidget *parent = 0);
    ~ConfigStabilizationWidget();

private:
    Ui_StabilizationWidget *m_stabilization;
    StabilizationSettings* stabSettings;
    QTimer updateTimer;
    void refreshUIValues(StabilizationSettings::DataFields &stabData);

private slots:
    virtual void refreshWidgetsValues();
    void updateObjectsFromWidgets();
    void reloadBoardValues();
    void realtimeUpdateToggle(bool);
    void resetToDefaults();
    void openHelp();

    void updateRateRollKP(double);
    void updateRateRollKI(double);
    void updateRateRollILimit(double);

    void updateRatePitchKP(double);
    void updateRatePitchKI(double);
    void updateRatePitchILimit(double);

    void updateRateYawKP(double);
    void updateRateYawKI(double);

    void updateRollKP(double);
    void updateRollKI(double);
    void updateRollILimit(double);

    void updatePitchKP(double);
    void updatePitchKI(double);
    void updatePitchILimit(double);

    void updateYawKP(double);
    void updateYawKI(double);

    void updatePitchMax(int);
    void updateRollMax(int);
    void updateYawMax(int);

    void updateManualPitch(int);
    void updateManualRoll(int);
    void updateManualYaw(int);

    void updateMaximumPitch(int);
    void updateMaximumRoll(int);
    void updateMaximumYaw(int);

    void updateBasicPitchProp(int);
    void updateBasicPitchInt(int);
    void updateBasicRollProp(int);
    void updateBasicRollInt(int);
    void updateBasicYawProp(int);
    void updateBasicYawInt(int);

    void updateBasicAttPitchProp(int);
    void updateBasicAttPitchInt(int);
    void updateBasicAttRollProp(int);
    void updateBasicAttRollInt(int);
    void updateBasicAttYawProp(int);
    void updateBasicAttYawInt(int);

    void updateBasicLimFsaP(int);
    void updateBasicLimFsaR(int);
    void updateBasicLimFsaY(int);

    void updateBasicLimFsrP(int);
    void updateBasicLimFsrR(int);
    void updateBasicLimFsrY(int);

    void updateBasicLimMraP(int);
    void updateBasicLimMraR(int);
    void updateBasicLimMraY(int);

    void activateLinkRate(void);
    void activateLinkRateExpert(void);
    void activateLinkAttitude(void);
    void activateLinkAttitudeExpert(void);
};

#endif // CONFIGSTABILIZATIONWIDGET_H
