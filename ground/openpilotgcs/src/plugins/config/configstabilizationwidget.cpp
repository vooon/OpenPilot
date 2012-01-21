/**
 ******************************************************************************
 *
 * @file       configstabilizationwidget.h
 * @author     E. Lafargue & The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief The Configuration Gadget used to update settings in the firmware
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
#include "configstabilizationwidget.h"

#include <QDebug>
#include <QStringList>
#include <QtGui/QWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QDesktopServices>
#include <QUrl>

ConfigStabilizationWidget::ConfigStabilizationWidget(QWidget *parent) : ConfigTaskWidget(parent)
{
    stabSettings = StabilizationSettings::GetInstance(getObjectManager());

    m_stabilization = new Ui_StabilizationWidget();
    m_stabilization->setupUi(this);

    setupButtons(m_stabilization->saveStabilizationToRAM, m_stabilization->saveStabilizationToSD);

    addUAVObject("StabilizationSettings");

    refreshWidgetsValues();

    // Create a timer to regularly send the object update in case
    // we want realtime updates.
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateObjectsFromWidgets()));
    connect(m_stabilization->realTimeUpdates, SIGNAL(toggled(bool)), this, SLOT(realtimeUpdateToggle(bool)));

    // Connect the updates of the stab values
    connect(m_stabilization->rateRollKp, SIGNAL(valueChanged(double)), this, SLOT(updateRateRollKP(double)));
    connect(m_stabilization->rateRollKi, SIGNAL(valueChanged(double)), this, SLOT(updateRateRollKI(double)));
    connect(m_stabilization->rateRollILimit, SIGNAL(valueChanged(double)), this, SLOT(updateRateRollILimit(double)));

    connect(m_stabilization->ratePitchKp, SIGNAL(valueChanged(double)), this, SLOT(updateRatePitchKP(double)));
    connect(m_stabilization->ratePitchKi, SIGNAL(valueChanged(double)), this, SLOT(updateRatePitchKI(double)));
    connect(m_stabilization->ratePitchILimit, SIGNAL(valueChanged(double)), this, SLOT(updateRatePitchILimit(double)));

    connect(m_stabilization->rateYawKp, SIGNAL(valueChanged(double)), this, SLOT(updateRateYawKP(double)));
    connect(m_stabilization->rateYawKi, SIGNAL(valueChanged(double)), this, SLOT(updateRateYawKI(double)));

    connect(m_stabilization->rollKp, SIGNAL(valueChanged(double)), this, SLOT(updateRollKP(double)));
    connect(m_stabilization->rollKi, SIGNAL(valueChanged(double)), this, SLOT(updateRollKI(double)));
    connect(m_stabilization->rollILimit, SIGNAL(valueChanged(double)), this, SLOT(updateRollILimit(double)));

    connect(m_stabilization->pitchKp, SIGNAL(valueChanged(double)), this, SLOT(updatePitchKP(double)));
    connect(m_stabilization->pitchKi, SIGNAL(valueChanged(double)), this, SLOT(updatePitchKI(double)));
    connect(m_stabilization->pitchILimit, SIGNAL(valueChanged(double)), this, SLOT(updatePitchILimit(double)));

    // basic settings rate stabilization slot assignment
    connect(m_stabilization->ratePitchProp, SIGNAL(valueChanged(int)), this, SLOT(updateBasicPitchProp(int)));
    connect(m_stabilization->rateRollProp, SIGNAL(valueChanged(int)), this, SLOT(updateBasicRollProp(int)));
    connect(m_stabilization->ratePitchInt, SIGNAL(valueChanged(int)), this, SLOT(updateBasicPitchInt(int)));
    connect(m_stabilization->rateRollInt, SIGNAL(valueChanged(int)), this, SLOT(updateBasicRollInt(int)));
    connect(m_stabilization->rateYawProp, SIGNAL(valueChanged(int)), this, SLOT(updateBasicYawProp(int)));
    connect(m_stabilization->rateYawInt, SIGNAL(valueChanged(int)), this, SLOT(updateBasicYawInt(int)));

    addWidget(m_stabilization->rateRollKp);
    addWidget(m_stabilization->rateRollKi);
    addWidget(m_stabilization->rateRollILimit);
    addWidget(m_stabilization->ratePitchKp);
    addWidget(m_stabilization->ratePitchKi);
    addWidget(m_stabilization->ratePitchILimit);
    addWidget(m_stabilization->rateYawKp);
    addWidget(m_stabilization->rateYawKi);
    addWidget(m_stabilization->rateYawILimit);
    addWidget(m_stabilization->rollKp);
    addWidget(m_stabilization->rollKi);
    addWidget(m_stabilization->rollILimit);
    addWidget(m_stabilization->yawILimit);
    addWidget(m_stabilization->yawKi);
    addWidget(m_stabilization->yawKp);
    addWidget(m_stabilization->pitchKp);
    addWidget(m_stabilization->pitchKi);
    addWidget(m_stabilization->pitchILimit);
    addWidget(m_stabilization->rollMax);
    addWidget(m_stabilization->pitchMax);
    addWidget(m_stabilization->yawMax);
    addWidget(m_stabilization->manualRoll);
    addWidget(m_stabilization->manualPitch);
    addWidget(m_stabilization->manualYaw);
    addWidget(m_stabilization->maximumRoll);
    addWidget(m_stabilization->maximumPitch);
    addWidget(m_stabilization->maximumYaw);
    addWidget(m_stabilization->lowThrottleZeroIntegral);

    // Connect buttons
    connect(m_stabilization->stabilizationResetToDefaults, SIGNAL(clicked()), this, SLOT(resetToDefaults()));
    connect(m_stabilization->stabilizationHelp, SIGNAL(clicked()), this, SLOT(openHelp()));
    connect(m_stabilization->stabilizationReloadBoardData, SIGNAL(clicked()), this, SLOT (reloadBoardValues()));
    connect(m_stabilization->linkRatePitchRoll, SIGNAL(clicked()), this, SLOT (activateLinkRate()));
    connect(m_stabilization->linkRateRP, SIGNAL(clicked()), this, SLOT (activateLinkRateExpert()));
}

ConfigStabilizationWidget::~ConfigStabilizationWidget()
{
   // Do nothing
}

void ConfigStabilizationWidget::reloadBoardValues()
{
    refreshWidgetsValues();
}

void ConfigStabilizationWidget::updateRateRollKP(double val)
{
    if (m_stabilization->linkRateRP->isChecked()) {
        m_stabilization->ratePitchKp->setValue(val);
    }
    // update basic tab dial
    if( val < RATE_KP_MIN || val > RATE_KP_MAX ) {
        m_stabilization->RPRL->setText( "<font color='red'><center>OUT<br/>OF<br/>RANGE</cebter></font>" );
        m_stabilization->rateRollProp->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->RPRL->setText( "<font color='black'>0</font>" );
        m_stabilization->rateRollProp->setEnabled( true );
        int basicVal = ( val - RATE_KP_MIN) / ( RATE_KP_MAX - RATE_KP_MIN) * 100;
        m_stabilization->rateRollProp->blockSignals( true );
        m_stabilization->rateRollProp->setValue( basicVal );
        if (m_stabilization->linkRateRP->isChecked()) {
            m_stabilization->ratePitchProp->blockSignals( true );
            m_stabilization->ratePitchProp->setValue( basicVal );
            m_stabilization->ratePitchProp->blockSignals( false );
        }
        m_stabilization->rateRollProp->blockSignals( false );
        m_stabilization->RPRL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateRateRollKI(double val)
{
    if (m_stabilization->linkRateRP->isChecked()) {
        m_stabilization->ratePitchKi->setValue(val);
    }
    // update basic tab dial
    if( val < RATE_KI_MIN || val > RATE_KI_MAX ) {
        m_stabilization->RIRL->setText( "<font color='red'><center>OUT<br/>OF<br/>RANGE</cebter></font>" );
        m_stabilization->rateRollInt->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->RIRL->setText( "<font color='black'>0</font>" );
        m_stabilization->rateRollInt->setEnabled( true );
        int basicVal = ( val - RATE_KI_MIN) / ( RATE_KI_MAX - RATE_KI_MIN) * 100;
        m_stabilization->rateRollInt->blockSignals( true );
        m_stabilization->rateRollInt->setValue( basicVal );
        if (m_stabilization->linkRateRP->isChecked()) {
            m_stabilization->ratePitchInt->blockSignals( true );
            m_stabilization->ratePitchInt->setValue( basicVal );
            m_stabilization->ratePitchInt->blockSignals( false );
        }
        m_stabilization->rateRollInt->blockSignals( false );
        m_stabilization->RIRL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateRateRollILimit(double val)
{
    if (m_stabilization->linkRateRP->isChecked()) {
        m_stabilization->ratePitchILimit->setValue(val);
    }
}

void ConfigStabilizationWidget::updateRatePitchKP(double val)
{
    if (m_stabilization->linkRateRP->isChecked()) {
        m_stabilization->rateRollKp->setValue(val);
    }
    // update basic tab dial
    if( val < RATE_KP_MIN || val > RATE_KP_MAX ) {
        m_stabilization->RPPL->setText( "<font color='red'><center>OUT<br/>OF<br/>RANGE</cebter></font>" );
        m_stabilization->ratePitchProp->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->RPPL->setText( "<font color='black'>0</font>" );
        m_stabilization->ratePitchProp->setEnabled( true );
        int basicVal = ( val - RATE_KP_MIN) / ( RATE_KP_MAX - RATE_KP_MIN) * 100;
        m_stabilization->ratePitchProp->blockSignals( true );
        m_stabilization->ratePitchProp->setValue( basicVal );
        if (m_stabilization->linkRateRP->isChecked()) {
            m_stabilization->rateRollProp->blockSignals( true );
            m_stabilization->rateRollProp->setValue( basicVal );
            m_stabilization->rateRollProp->blockSignals( false );
        }
        m_stabilization->ratePitchProp->blockSignals( false );
        m_stabilization->RPPL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateRatePitchKI(double val)
{
    if (m_stabilization->linkRateRP->isChecked()) {
        m_stabilization->rateRollKi->setValue(val);
    }
    // update basic tab dial
    if( val < RATE_KI_MIN || val > RATE_KI_MAX ) {
        m_stabilization->RIPL->setText( "<font color='red'><center>OUT<br/>OF<br/>RANGE</cebter></font>" );
        m_stabilization->ratePitchInt->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->RIPL->setText( "<font color='black'>0</font>" );
        m_stabilization->ratePitchInt->setEnabled( true );
        int basicVal = ( val - RATE_KI_MIN) / ( RATE_KI_MAX - RATE_KI_MIN) * 100;
        m_stabilization->ratePitchInt->blockSignals( true );
        m_stabilization->ratePitchInt->setValue( basicVal );
        if (m_stabilization->linkRateRP->isChecked()) {
            m_stabilization->rateRollInt->blockSignals( true );
            m_stabilization->rateRollInt->setValue( basicVal );
            m_stabilization->rateRollInt->blockSignals( false );
        }
        m_stabilization->ratePitchInt->blockSignals( false );
        m_stabilization->RIPL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateRateYawKP(double val)
{
    // update basic tab dial
    if( val < RATE_KP_MIN || val > RATE_KP_MAX ) {
        m_stabilization->RPYL->setText( "<font color='red'><center>OUT<br/>OF<br/>RANGE</cebter></font>" );
        m_stabilization->rateYawProp->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->RPYL->setText( "<font color='black'>0</font>" );
        m_stabilization->rateYawProp->setEnabled( true );
        int basicVal = ( val - RATE_KP_MIN) / ( RATE_KP_MAX - RATE_KP_MIN) * 100;
        m_stabilization->rateYawProp->blockSignals( true );
        m_stabilization->rateYawProp->setValue( basicVal );
        m_stabilization->rateYawProp->blockSignals( false );
        m_stabilization->RPYL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateRateYawKI(double val)
{
    // update basic tab dial
    if( val < RATE_KI_MIN || val > RATE_KI_MAX ) {
        m_stabilization->RIYL->setText( "<font color='red'><center>OUT<br/>OF<br/>RANGE</cebter></font>" );
        m_stabilization->rateYawInt->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->RIYL->setText( "<font color='black'>0</font>" );
        m_stabilization->rateYawInt->setEnabled( true );
        int basicVal = ( val - RATE_KI_MIN) / ( RATE_KI_MAX - RATE_KI_MIN) * 100;
        m_stabilization->rateYawInt->blockSignals( true );
        m_stabilization->rateYawInt->setValue( basicVal );
        m_stabilization->rateYawInt->blockSignals( false );
        m_stabilization->RIYL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateRatePitchILimit(double val)
{
    if (m_stabilization->linkRateRP->isChecked()) {
        m_stabilization->rateRollILimit->setValue(val);
    }
}

void ConfigStabilizationWidget::updateRollKP(double val)
{
    if (m_stabilization->linkAttitudeRP->isChecked()) {
        m_stabilization->pitchKp->setValue(val);
    }
}

void ConfigStabilizationWidget::updateRollKI(double val)
{
    if (m_stabilization->linkAttitudeRP->isChecked()) {
        m_stabilization->pitchKi->setValue(val);
    }

}

void ConfigStabilizationWidget::updateRollILimit(double val)
{
    if (m_stabilization->linkAttitudeRP->isChecked()) {
        m_stabilization->pitchILimit->setValue(val);
    }
}

void ConfigStabilizationWidget::updatePitchKP(double val)
{
    if (m_stabilization->linkAttitudeRP->isChecked()) {
        m_stabilization->rollKp->setValue(val);
    }
}

void ConfigStabilizationWidget::updatePitchKI(double val)
{
    if (m_stabilization->linkAttitudeRP->isChecked()) {
        m_stabilization->rollKi->setValue(val);
    }

}

void ConfigStabilizationWidget::updatePitchILimit(double val)
{
    if (m_stabilization->linkAttitudeRP->isChecked()) {
        m_stabilization->rollILimit->setValue(val);
    }
}

void ConfigStabilizationWidget::updateBasicPitchProp(int val)
{
    if (m_stabilization->linkRatePitchRoll->isChecked()) {
        m_stabilization->rateRollProp->setValue(val);
    }

    // set value in expert tab
    m_stabilization->ratePitchKp->setValue((RATE_KP_MAX - RATE_KP_MIN) / 100.0 * val);
}

void ConfigStabilizationWidget::updateBasicPitchInt(int val)
{
    if (m_stabilization->linkRatePitchRoll->isChecked()) {
        m_stabilization->rateRollInt->setValue(val);
    }

    // set value in expert tab
    m_stabilization->ratePitchKi->setValue((RATE_KI_MAX - RATE_KI_MIN) / 100.0 * val);
}

void ConfigStabilizationWidget::updateBasicRollProp(int val)
{
    if (m_stabilization->linkRatePitchRoll->isChecked()) {
        m_stabilization->ratePitchProp->setValue(val);
    }

    // set value in expert tab
    m_stabilization->rateRollKp->setValue((RATE_KP_MAX - RATE_KP_MIN) / 100.0 * val);
}

void ConfigStabilizationWidget::updateBasicRollInt(int val)
{
    if (m_stabilization->linkRatePitchRoll->isChecked()) {
        m_stabilization->ratePitchInt->setValue(val);
    }

    // set value in expert tab
    m_stabilization->rateRollKi->setValue((RATE_KI_MAX - RATE_KI_MIN) / 100.0 * val);
}

void ConfigStabilizationWidget::updateBasicYawProp(int val)
{
    // set value in expert tab
    m_stabilization->rateYawKp->setValue((RATE_KP_MAX - RATE_KP_MIN) / 100.0 * val);
}

void ConfigStabilizationWidget::updateBasicYawInt(int val)
{
    // set value in expert tab
    m_stabilization->rateYawKi->setValue((RATE_KI_MAX - RATE_KI_MIN) / 100.0 * val);
}

void ConfigStabilizationWidget::activateLinkRate(void)
{
    m_stabilization->rateRollProp->setValue(m_stabilization->ratePitchProp->value());
    m_stabilization->rateRollInt->setValue(m_stabilization->ratePitchInt->value());

    // toggle button also in expert mode
    m_stabilization->linkRateRP->toggle();
}

void ConfigStabilizationWidget::activateLinkRateExpert(void)
{
    m_stabilization->rateRollKp->setValue(m_stabilization->ratePitchKp->value());
    m_stabilization->rateRollKi->setValue(m_stabilization->ratePitchKi->value());

    // toggle button also in basic mode
    m_stabilization->linkRatePitchRoll->toggle();
}

/*******************************
 * Stabilization Settings
 *****************************/

/**
  * Refresh UI with new settings of StabilizationSettings object
  * (either from active configuration or just loaded defaults
  * to be applied or saved)
  */
void ConfigStabilizationWidget::refreshUIValues(StabilizationSettings::DataFields &stabData)
{
    // Now fill in all the fields, this is fairly tedious:
    m_stabilization->rateRollKp->setValue(stabData.RollRatePID[StabilizationSettings::ROLLRATEPID_KP]);
    updateRateRollKP(stabData.RollRatePID[StabilizationSettings::ROLLRATEPID_KP]);
    m_stabilization->rateRollKi->setValue(stabData.RollRatePID[StabilizationSettings::ROLLRATEPID_KI]);
    updateRateRollKI(stabData.RollRatePID[StabilizationSettings::ROLLRATEPID_KI]);
    m_stabilization->rateRollILimit->setValue(stabData.RollRatePID[StabilizationSettings::ROLLRATEPID_ILIMIT]);

    m_stabilization->ratePitchKp->setValue(stabData.PitchRatePID[StabilizationSettings::PITCHRATEPID_KP]);
    updateRatePitchKP(stabData.PitchRatePID[StabilizationSettings::PITCHRATEPID_KP]);
    m_stabilization->ratePitchKi->setValue(stabData.PitchRatePID[StabilizationSettings::PITCHRATEPID_KI]);
    updateRatePitchKI(stabData.PitchRatePID[StabilizationSettings::PITCHRATEPID_KI]);
    m_stabilization->ratePitchILimit->setValue(stabData.PitchRatePID[StabilizationSettings::PITCHRATEPID_ILIMIT]);

    m_stabilization->rateYawKp->setValue(stabData.YawRatePID[StabilizationSettings::YAWRATEPID_KP]);
    updateRateYawKP(stabData.YawRatePID[StabilizationSettings::YAWRATEPID_KP]);
    m_stabilization->rateYawKi->setValue(stabData.YawRatePID[StabilizationSettings::YAWRATEPID_KI]);
    updateRateYawKI(stabData.YawRatePID[StabilizationSettings::YAWRATEPID_KI]);
    m_stabilization->rateYawILimit->setValue(stabData.YawRatePID[StabilizationSettings::YAWRATEPID_ILIMIT]);

    m_stabilization->rollKp->setValue(stabData.RollPI[StabilizationSettings::ROLLPI_KP]);
    m_stabilization->rollKi->setValue(stabData.RollPI[StabilizationSettings::ROLLPI_KI]);
    m_stabilization->rollILimit->setValue(stabData.RollPI[StabilizationSettings::ROLLPI_ILIMIT]);

    m_stabilization->pitchKp->setValue(stabData.PitchPI[StabilizationSettings::PITCHPI_KP]);
    m_stabilization->pitchKi->setValue(stabData.PitchPI[StabilizationSettings::PITCHPI_KI]);
    m_stabilization->pitchILimit->setValue(stabData.PitchPI[StabilizationSettings::PITCHPI_ILIMIT]);

    m_stabilization->yawKp->setValue(stabData.YawPI[StabilizationSettings::YAWPI_KP]);
    m_stabilization->yawKi->setValue(stabData.YawPI[StabilizationSettings::YAWPI_KI]);
    m_stabilization->yawILimit->setValue(stabData.YawPI[StabilizationSettings::YAWPI_ILIMIT]);

    m_stabilization->rollMax->setValue(stabData.RollMax);
    m_stabilization->pitchMax->setValue(stabData.PitchMax);
    m_stabilization->yawMax->setValue(stabData.YawMax);

    m_stabilization->manualRoll->setValue(stabData.ManualRate[StabilizationSettings::MANUALRATE_ROLL]);
    m_stabilization->manualPitch->setValue(stabData.ManualRate[StabilizationSettings::MANUALRATE_PITCH]);
    m_stabilization->manualYaw->setValue(stabData.ManualRate[StabilizationSettings::MANUALRATE_YAW]);

    m_stabilization->maximumRoll->setValue(stabData.MaximumRate[StabilizationSettings::MAXIMUMRATE_ROLL]);
    m_stabilization->maximumPitch->setValue(stabData.MaximumRate[StabilizationSettings::MAXIMUMRATE_PITCH]);
    m_stabilization->maximumYaw->setValue(stabData.MaximumRate[StabilizationSettings::MAXIMUMRATE_YAW]);

    m_stabilization->lowThrottleZeroIntegral->setChecked(stabData.LowThrottleZeroIntegral==StabilizationSettings::LOWTHROTTLEZEROINTEGRAL_TRUE ? true : false);
}

/**
  Request stabilization settings from the board
  */
void ConfigStabilizationWidget::refreshWidgetsValues()
{
    bool dirty=isDirty();
    // Not needed anymore as this slot is only called whenever we get
    // a signal that the object was just updated
    // stabSettings->requestUpdate();
    StabilizationSettings::DataFields stabData = stabSettings->getData();
    refreshUIValues(stabData);
    setDirty(dirty);
}

/**
  Send telemetry settings to the board
  */
void ConfigStabilizationWidget::updateObjectsFromWidgets()
{
    StabilizationSettings::DataFields stabData = stabSettings->getData();

    stabData.RollRatePID[StabilizationSettings::ROLLRATEPID_KP] = m_stabilization->rateRollKp->value();
    stabData.RollRatePID[StabilizationSettings::ROLLRATEPID_KI] = m_stabilization->rateRollKi->value();
    stabData.RollRatePID[StabilizationSettings::ROLLRATEPID_ILIMIT] = m_stabilization->rateRollILimit->value();

    stabData.PitchRatePID[StabilizationSettings::PITCHRATEPID_KP] = m_stabilization->ratePitchKp->value();
    stabData.PitchRatePID[StabilizationSettings::PITCHRATEPID_KI] = m_stabilization->ratePitchKi->value();
    stabData.PitchRatePID[StabilizationSettings::PITCHRATEPID_ILIMIT] = m_stabilization->ratePitchILimit->value();

    stabData.YawRatePID[StabilizationSettings::YAWRATEPID_KP] = m_stabilization->rateYawKp->value();
    stabData.YawRatePID[StabilizationSettings::YAWRATEPID_KI] = m_stabilization->rateYawKi->value();
    stabData.YawRatePID[StabilizationSettings::YAWRATEPID_ILIMIT] = m_stabilization->rateYawILimit->value();

    stabData.RollPI[StabilizationSettings::ROLLPI_KP] = m_stabilization->rollKp->value();
    stabData.RollPI[StabilizationSettings::ROLLPI_KI] = m_stabilization->rollKi->value();
    stabData.RollPI[StabilizationSettings::ROLLPI_ILIMIT] = m_stabilization->rollILimit->value();

    stabData.PitchPI[StabilizationSettings::PITCHPI_KP] = m_stabilization->pitchKp->value();
    stabData.PitchPI[StabilizationSettings::PITCHPI_KI] = m_stabilization->pitchKi->value();
    stabData.PitchPI[StabilizationSettings::PITCHPI_ILIMIT] = m_stabilization->pitchILimit->value();

    stabData.YawPI[StabilizationSettings::YAWPI_KP] = m_stabilization->yawKp->value();
    stabData.YawPI[StabilizationSettings::YAWPI_KI] = m_stabilization->yawKi->value();
    stabData.YawPI[StabilizationSettings::YAWPI_ILIMIT] = m_stabilization->yawILimit->value();

    stabData.RollMax = m_stabilization->rollMax->value();
    stabData.PitchMax = m_stabilization->pitchMax->value();
    stabData.YawMax = m_stabilization->yawMax->value();

    stabData.ManualRate[StabilizationSettings::MANUALRATE_ROLL] = m_stabilization->manualRoll->value();
    stabData.ManualRate[StabilizationSettings::MANUALRATE_PITCH] = m_stabilization->manualPitch->value();
    stabData.ManualRate[StabilizationSettings::MANUALRATE_YAW] = m_stabilization->manualYaw->value();

    stabData.MaximumRate[StabilizationSettings::MAXIMUMRATE_ROLL] = m_stabilization->maximumRoll->value();
    stabData.MaximumRate[StabilizationSettings::MAXIMUMRATE_PITCH] = m_stabilization->maximumPitch->value();
    stabData.MaximumRate[StabilizationSettings::MAXIMUMRATE_YAW] = m_stabilization->maximumYaw->value();

    stabData.LowThrottleZeroIntegral = (m_stabilization->lowThrottleZeroIntegral->isChecked() ? StabilizationSettings::LOWTHROTTLEZEROINTEGRAL_TRUE :StabilizationSettings::LOWTHROTTLEZEROINTEGRAL_FALSE);

    stabSettings->setData(stabData); // this is atomic
}

void ConfigStabilizationWidget::realtimeUpdateToggle(bool state)
{
    if (state) {
        updateTimer.start(300);
    } else {
        updateTimer.stop();
    }
}

void ConfigStabilizationWidget::resetToDefaults()
{
    StabilizationSettings stabDefaults;
    StabilizationSettings::DataFields defaults = stabDefaults.getData();
    bool dirty=isDirty();
    refreshUIValues(defaults);
    setDirty(dirty);
}

void ConfigStabilizationWidget::openHelp()
{
    QDesktopServices::openUrl( QUrl("http://wiki.openpilot.org/display/Doc/Stabilization+panel", QUrl::StrictMode) );
}
