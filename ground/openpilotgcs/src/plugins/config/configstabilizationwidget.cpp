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

// constants for range limits for the dials in basic mode
static const double RATE_KP_MIN = 0.0;
static const double RATE_KP_MAX = 0.01;

static const double RATE_KI_MIN = 0.0;
static const double RATE_KI_MAX = 0.01;

static const double ATTITUDE_KP_MIN = 0.0;
static const double ATTITUDE_KP_MAX = 10.0;

static const double ATTITUDE_KI_MIN = 0.0;
static const double ATTITUDE_KI_MAX = 10.0;

static const double FULL_STICK_ANGLE_MIN = 0;
static const double FULL_STICK_ANGLE_MAX = 180;

static const double FULL_STICK_RATE_MIN = 0;
static const double FULL_STICK_RATE_MAX = 500;

static const double MAXIMUM_RATE_MIN = 0;
static const double MAXIMUM_RATE_MAX = 500;

// ------------------------------------------------------

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

    connect(m_stabilization->rollMax, SIGNAL(valueChanged(int)), this, SLOT(updateRollMax(int)));
    connect(m_stabilization->pitchMax, SIGNAL(valueChanged(int)), this, SLOT(updatePitchMax(int)));
    connect(m_stabilization->yawMax, SIGNAL(valueChanged(int)), this, SLOT(updateYawMax(int)));

    connect(m_stabilization->manualRoll, SIGNAL(valueChanged(int)), this, SLOT(updateManualRoll(int)));
    connect(m_stabilization->manualPitch, SIGNAL(valueChanged(int)), this, SLOT(updateManualPitch(int)));
    connect(m_stabilization->manualYaw, SIGNAL(valueChanged(int)), this, SLOT(updateManualYaw(int)));

    connect(m_stabilization->maximumRoll, SIGNAL(valueChanged(int)), this, SLOT(updateMaximumRoll(int)));
    connect(m_stabilization->maximumPitch, SIGNAL(valueChanged(int)), this, SLOT(updateMaximumPitch(int)));
    connect(m_stabilization->maximumYaw, SIGNAL(valueChanged(int)), this, SLOT(updateMaximumYaw(int)));

    // basic settings rate stabilization slot assignment
    connect(m_stabilization->ratePitchProp, SIGNAL(valueChanged(int)), this, SLOT(updateBasicPitchProp(int)));
    connect(m_stabilization->rateRollProp, SIGNAL(valueChanged(int)), this, SLOT(updateBasicRollProp(int)));
    connect(m_stabilization->ratePitchInt, SIGNAL(valueChanged(int)), this, SLOT(updateBasicPitchInt(int)));
    connect(m_stabilization->rateRollInt, SIGNAL(valueChanged(int)), this, SLOT(updateBasicRollInt(int)));
    connect(m_stabilization->rateYawProp, SIGNAL(valueChanged(int)), this, SLOT(updateBasicYawProp(int)));
    connect(m_stabilization->rateYawInt, SIGNAL(valueChanged(int)), this, SLOT(updateBasicYawInt(int)));

    connect(m_stabilization->attPitchProp, SIGNAL(valueChanged(int)), this, SLOT(updateBasicAttPitchProp(int)));
    connect(m_stabilization->attRollProp, SIGNAL(valueChanged(int)), this, SLOT(updateBasicAttRollProp(int)));
    connect(m_stabilization->attPitchInt, SIGNAL(valueChanged(int)), this, SLOT(updateBasicAttPitchInt(int)));
    connect(m_stabilization->attRollInt, SIGNAL(valueChanged(int)), this, SLOT(updateBasicAttRollInt(int)));
    connect(m_stabilization->attYawProp, SIGNAL(valueChanged(int)), this, SLOT(updateBasicAttYawProp(int)));
    connect(m_stabilization->attYawInt, SIGNAL(valueChanged(int)), this, SLOT(updateBasicAttYawInt(int)));

    connect(m_stabilization->fsaP, SIGNAL(valueChanged(int)), this, SLOT(updateBasicLimFsaP(int)));
    connect(m_stabilization->fsaR, SIGNAL(valueChanged(int)), this, SLOT(updateBasicLimFsaR(int)));
    connect(m_stabilization->fsaY, SIGNAL(valueChanged(int)), this, SLOT(updateBasicLimFsaY(int)));

    connect(m_stabilization->fsrP, SIGNAL(valueChanged(int)), this, SLOT(updateBasicLimFsrP(int)));
    connect(m_stabilization->fsrR, SIGNAL(valueChanged(int)), this, SLOT(updateBasicLimFsrR(int)));
    connect(m_stabilization->fsrY, SIGNAL(valueChanged(int)), this, SLOT(updateBasicLimFsrY(int)));

    connect(m_stabilization->mraP, SIGNAL(valueChanged(int)), this, SLOT(updateBasicLimMraP(int)));
    connect(m_stabilization->mraR, SIGNAL(valueChanged(int)), this, SLOT(updateBasicLimMraR(int)));
    connect(m_stabilization->mraY, SIGNAL(valueChanged(int)), this, SLOT(updateBasicLimMraY(int)));

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
    connect(m_stabilization->linkAttPitchRoll, SIGNAL(clicked()), this, SLOT (activateLinkAttitude()));
    connect(m_stabilization->linkAttitudeRP, SIGNAL(clicked()), this, SLOT (activateLinkAttitudeExpert()));
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
        m_stabilization->RPRL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->RPRL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->rateRollProp->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->RPRL->setStyleSheet("QLabel { color : black; }");
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
        m_stabilization->RIRL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->RIRL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->rateRollInt->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->RIRL->setStyleSheet("QLabel { color : black; }");
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
        m_stabilization->RPPL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->RPPL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->ratePitchProp->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->RPPL->setStyleSheet("QLabel { color : black; }");
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
        m_stabilization->RIPL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->RIPL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->ratePitchInt->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->RIPL->setStyleSheet("QLabel { color : black; }");
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
        m_stabilization->RPYL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->RPYL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->rateYawProp->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->RPYL->setStyleSheet("QLabel { color : black; }");
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
        m_stabilization->RIYL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->RIYL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->rateYawInt->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->RIYL->setStyleSheet("QLabel { color : black; }");
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

    // update basic tab dial
    if( val < ATTITUDE_KP_MIN || val > ATTITUDE_KP_MAX ) {
        m_stabilization->APRL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->APRL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->attRollProp->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->APRL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->attRollProp->setEnabled( true );
        int basicVal = ( val - ATTITUDE_KP_MIN) / ( ATTITUDE_KP_MAX - ATTITUDE_KP_MIN) * 100;
        m_stabilization->attRollProp->blockSignals( true );
        m_stabilization->attRollProp->setValue( basicVal );
        if (m_stabilization->linkAttitudeRP->isChecked()) {
            m_stabilization->attPitchProp->blockSignals( true );
            m_stabilization->attPitchProp->setValue( basicVal );
            m_stabilization->attPitchProp->blockSignals( false );
        }
        m_stabilization->attRollProp->blockSignals( false );
        m_stabilization->APRL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateRollKI(double val)
{
    if (m_stabilization->linkAttitudeRP->isChecked()) {
        m_stabilization->pitchKi->setValue(val);
    }
    // update basic tab dial
    if( val < ATTITUDE_KI_MIN || val > ATTITUDE_KI_MAX ) {
        m_stabilization->AIRL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->AIRL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->attRollInt->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->AIRL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->attRollInt->setEnabled( true );
        int basicVal = ( val - ATTITUDE_KI_MIN) / ( ATTITUDE_KI_MAX - ATTITUDE_KI_MIN) * 100;
        m_stabilization->attRollInt->blockSignals( true );
        m_stabilization->attRollInt->setValue( basicVal );
        if (m_stabilization->linkAttitudeRP->isChecked()) {
            m_stabilization->attPitchInt->blockSignals( true );
            m_stabilization->attPitchInt->setValue( basicVal );
            m_stabilization->attPitchInt->blockSignals( false );
        }
        m_stabilization->attRollInt->blockSignals( false );
        m_stabilization->AIRL->setNum( basicVal );
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
    // update basic tab dial
    if( val < ATTITUDE_KP_MIN || val > ATTITUDE_KP_MAX ) {
        m_stabilization->APPL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->APPL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->attPitchProp->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->APPL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->attPitchProp->setEnabled( true );
        int basicVal = ( val - ATTITUDE_KP_MIN) / ( ATTITUDE_KP_MAX - ATTITUDE_KP_MIN) * 100;
        m_stabilization->attPitchProp->blockSignals( true );
        m_stabilization->attPitchProp->setValue( basicVal );
        if (m_stabilization->linkAttitudeRP->isChecked()) {
            m_stabilization->attRollProp->blockSignals( true );
            m_stabilization->attRollProp->setValue( basicVal );
            m_stabilization->attRollProp->blockSignals( false );
        }
        m_stabilization->attPitchProp->blockSignals( false );
        m_stabilization->APPL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updatePitchKI(double val)
{
    if (m_stabilization->linkAttitudeRP->isChecked()) {
        m_stabilization->rollKi->setValue(val);
    }
    // update basic tab dial
    if( val < ATTITUDE_KI_MIN || val > ATTITUDE_KI_MAX ) {
        m_stabilization->AIPL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->AIPL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->attPitchInt->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->AIPL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->attPitchInt->setEnabled( true );
        int basicVal = ( val - ATTITUDE_KI_MIN) / ( ATTITUDE_KI_MAX - ATTITUDE_KI_MIN) * 100;
        m_stabilization->attPitchInt->blockSignals( true );
        m_stabilization->attPitchInt->setValue( basicVal );
        if (m_stabilization->linkAttitudeRP->isChecked()) {
            m_stabilization->attRollInt->blockSignals( true );
            m_stabilization->attRollInt->setValue( basicVal );
            m_stabilization->attRollInt->blockSignals( false );
        }
        m_stabilization->attPitchInt->blockSignals( false );
        m_stabilization->AIPL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updatePitchILimit(double val)
{
    if (m_stabilization->linkAttitudeRP->isChecked()) {
        m_stabilization->rollILimit->setValue(val);
    }
}

void ConfigStabilizationWidget::updateYawKP(double val)
{
    // update basic tab dial
    if( val < ATTITUDE_KP_MIN || val > ATTITUDE_KP_MAX ) {
        m_stabilization->APYL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->APYL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->attYawProp->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->APYL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->attYawProp->setEnabled( true );
        int basicVal = ( val - ATTITUDE_KP_MIN) / ( ATTITUDE_KP_MAX - ATTITUDE_KP_MIN) * 100;
        m_stabilization->attYawProp->blockSignals( true );
        m_stabilization->attYawProp->setValue( basicVal );
        m_stabilization->attYawProp->blockSignals( false );
        m_stabilization->APYL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateYawKI(double val)
{
    // update basic tab dial
    if( val < ATTITUDE_KI_MIN || val > ATTITUDE_KI_MAX ) {
        m_stabilization->AIYL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->AIYL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->attYawInt->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->AIYL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->attYawInt->setEnabled( true );
        int basicVal = ( val - RATE_KI_MIN) / ( RATE_KI_MAX - RATE_KI_MIN) * 100;
        m_stabilization->attYawInt->blockSignals( true );
        m_stabilization->attYawInt->setValue( basicVal );
        m_stabilization->attYawInt->blockSignals( false );
        m_stabilization->AIYL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updatePitchMax(int val)
{
    // update basic tab dial
    if( val < FULL_STICK_ANGLE_MIN || val > FULL_STICK_ANGLE_MAX ) {
        m_stabilization->fsaPL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->fsaPL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->fsaP->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->fsaPL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->fsaP->setEnabled( true );
        int basicVal = ( val - FULL_STICK_ANGLE_MIN ) / ( FULL_STICK_ANGLE_MAX - FULL_STICK_ANGLE_MIN) * 100;
        m_stabilization->fsaP->blockSignals( true );
        m_stabilization->fsaP->setValue( basicVal );
        m_stabilization->fsaP->blockSignals( false );
        m_stabilization->fsaPL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateRollMax(int val)
{
    // update basic tab dial
    if( val < FULL_STICK_ANGLE_MIN || val > FULL_STICK_ANGLE_MAX ) {
        m_stabilization->fsaRL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->fsaRL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->fsaR->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->fsaRL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->fsaR->setEnabled( true );
        int basicVal = ( val - FULL_STICK_ANGLE_MIN ) / ( FULL_STICK_ANGLE_MAX - FULL_STICK_ANGLE_MIN) * 100;
        m_stabilization->fsaR->blockSignals( true );
        m_stabilization->fsaR->setValue( basicVal );
        m_stabilization->fsaR->blockSignals( false );
        m_stabilization->fsaRL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateYawMax(int val)
{
    // update basic tab dial
    if( val < FULL_STICK_ANGLE_MIN || val > FULL_STICK_ANGLE_MAX ) {
        m_stabilization->fsaYL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->fsaYL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->fsaY->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->fsaYL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->fsaY->setEnabled( true );
        int basicVal = ( val - FULL_STICK_ANGLE_MIN ) / ( FULL_STICK_ANGLE_MAX - FULL_STICK_ANGLE_MIN) * 100;
        m_stabilization->fsaY->blockSignals( true );
        m_stabilization->fsaY->setValue( basicVal );
        m_stabilization->fsaY->blockSignals( false );
        m_stabilization->fsaYL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateManualPitch(int val)
{
    // update basic tab dial
    if( val < FULL_STICK_RATE_MIN || val > FULL_STICK_RATE_MAX ) {
        m_stabilization->fsrPL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->fsrPL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->fsrP->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->fsrPL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->fsrP->setEnabled( true );
        int basicVal = ( val - FULL_STICK_RATE_MIN ) / ( FULL_STICK_RATE_MAX - FULL_STICK_RATE_MIN) * 100;
        m_stabilization->fsrP->blockSignals( true );
        m_stabilization->fsrP->setValue( basicVal );
        m_stabilization->fsrP->blockSignals( false );
        m_stabilization->fsrPL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateManualRoll(int val)
{
    // update basic tab dial
    if( val < FULL_STICK_RATE_MIN || val > FULL_STICK_RATE_MAX ) {
        m_stabilization->fsrRL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->fsrRL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->fsrR->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->fsrRL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->fsrR->setEnabled( true );
        int basicVal = ( val - FULL_STICK_RATE_MIN ) / ( FULL_STICK_RATE_MAX - FULL_STICK_RATE_MIN) * 100;
        m_stabilization->fsrR->blockSignals( true );
        m_stabilization->fsrR->setValue( basicVal );
        m_stabilization->fsrR->blockSignals( false );
        m_stabilization->fsrRL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateManualYaw(int val)
{
    // update basic tab dial
    if( val < FULL_STICK_RATE_MIN || val > FULL_STICK_RATE_MAX ) {
        m_stabilization->fsrYL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->fsrYL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->fsrY->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->fsrYL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->fsrY->setEnabled( true );
        int basicVal = ( val - FULL_STICK_RATE_MIN ) / ( FULL_STICK_RATE_MAX - FULL_STICK_RATE_MIN) * 100;
        m_stabilization->fsrY->blockSignals( true );
        m_stabilization->fsrY->setValue( basicVal );
        m_stabilization->fsrY->blockSignals( false );
        m_stabilization->fsrYL->setNum( basicVal );
    }
}


void ConfigStabilizationWidget::updateMaximumPitch(int val)
{
    // update basic tab dial
    if( val < MAXIMUM_RATE_MIN || val > MAXIMUM_RATE_MAX ) {
        m_stabilization->mraPL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->mraPL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->mraP->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->mraPL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->mraP->setEnabled( true );
        int basicVal = ( val - MAXIMUM_RATE_MIN ) / ( MAXIMUM_RATE_MAX - MAXIMUM_RATE_MIN) * 100;
        m_stabilization->mraP->blockSignals( true );
        m_stabilization->mraP->setValue( basicVal );
        m_stabilization->mraP->blockSignals( false );
        m_stabilization->mraPL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateMaximumRoll(int val)
{
    // update basic tab dial
    if( val < MAXIMUM_RATE_MIN || val > MAXIMUM_RATE_MAX ) {
        m_stabilization->mraRL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->mraRL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->mraR->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->mraRL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->mraR->setEnabled( true );
        int basicVal = ( val - MAXIMUM_RATE_MIN ) / ( MAXIMUM_RATE_MAX - MAXIMUM_RATE_MIN) * 100;
        m_stabilization->mraR->blockSignals( true );
        m_stabilization->mraR->setValue( basicVal );
        m_stabilization->mraR->blockSignals( false );
        m_stabilization->mraRL->setNum( basicVal );
    }
}

void ConfigStabilizationWidget::updateMaximumYaw(int val)
{
    // update basic tab dial
    if( val < FULL_STICK_RATE_MIN || val > MAXIMUM_RATE_MAX ) {
        m_stabilization->mraYL->setStyleSheet("QLabel { color : red; }");
        m_stabilization->mraYL->setText( "OUT\nOF\nRANGE" );
        m_stabilization->mraY->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        m_stabilization->mraYL->setStyleSheet("QLabel { color : black; }");
        m_stabilization->mraY->setEnabled( true );
        int basicVal = ( val - MAXIMUM_RATE_MIN ) / ( MAXIMUM_RATE_MAX - MAXIMUM_RATE_MIN) * 100;
        m_stabilization->mraY->blockSignals( true );
        m_stabilization->mraY->setValue( basicVal );
        m_stabilization->mraY->blockSignals( false );
        m_stabilization->mraYL->setNum( basicVal );
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

void ConfigStabilizationWidget::updateBasicAttPitchProp(int val)
{
    if (m_stabilization->linkAttPitchRoll->isChecked()) {
        m_stabilization->attRollProp->setValue(val);
    }

    // set value in expert tab
    m_stabilization->pitchKp->setValue((ATTITUDE_KP_MAX - ATTITUDE_KP_MIN) / 100.0 * val);
}

void ConfigStabilizationWidget::updateBasicAttPitchInt(int val)
{
    if (m_stabilization->linkAttPitchRoll->isChecked()) {
        m_stabilization->attRollInt->setValue(val);
    }

    // set value in expert tab
    m_stabilization->pitchKi->setValue((ATTITUDE_KI_MAX - ATTITUDE_KI_MIN) / 100.0 * val);
}

void ConfigStabilizationWidget::updateBasicAttRollProp(int val)
{
    if (m_stabilization->linkAttPitchRoll->isChecked()) {
        m_stabilization->attPitchProp->setValue(val);
    }

    // set value in expert tab
    m_stabilization->rollKp->setValue((ATTITUDE_KP_MAX - ATTITUDE_KP_MIN) / 100.0 * val);
}

void ConfigStabilizationWidget::updateBasicAttRollInt(int val)
{
    if (m_stabilization->linkAttPitchRoll->isChecked()) {
        m_stabilization->attPitchInt->setValue(val);
    }

    // set value in expert tab
    m_stabilization->rollKi->setValue((ATTITUDE_KI_MAX - ATTITUDE_KI_MIN) / 100.0 * val);
}

void ConfigStabilizationWidget::updateBasicAttYawProp(int val)
{
    // set value in expert tab
    m_stabilization->yawKp->setValue((ATTITUDE_KP_MAX - ATTITUDE_KP_MIN) / 100.0 * val);
}

void ConfigStabilizationWidget::updateBasicAttYawInt(int val)
{
    // set value in expert tab
    m_stabilization->yawKi->setValue((ATTITUDE_KI_MAX - ATTITUDE_KI_MIN) / 100.0 * val);
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

void ConfigStabilizationWidget::activateLinkAttitude(void)
{
    m_stabilization->attRollProp->setValue(m_stabilization->attPitchProp->value());
    m_stabilization->attRollInt->setValue(m_stabilization->attPitchInt->value());

    // toggle button also in expert mode
    m_stabilization->linkAttitudeRP->toggle();
}

void ConfigStabilizationWidget::activateLinkAttitudeExpert(void)
{
    m_stabilization->rollKp->setValue(m_stabilization->pitchKp->value());
    m_stabilization->rollKi->setValue(m_stabilization->pitchKi->value());

    // toggle button also in basic mode
    m_stabilization->linkAttPitchRoll->toggle();
}


void ConfigStabilizationWidget::updateBasicLimFsaP(int val)
{
    // set value in expert tab
    m_stabilization->pitchMax->blockSignals( true );
    m_stabilization->pitchMax->setValue((FULL_STICK_ANGLE_MAX - FULL_STICK_ANGLE_MIN) / 100.0 * val);
    m_stabilization->pitchMax->blockSignals( false );
}

void ConfigStabilizationWidget::updateBasicLimFsaR(int val)
{
    // set value in expert tab
    m_stabilization->rollMax->blockSignals( true );
    m_stabilization->rollMax->setValue((FULL_STICK_ANGLE_MAX - FULL_STICK_ANGLE_MIN) / 100.0 * val);
    m_stabilization->rollMax->blockSignals( false );
}

void ConfigStabilizationWidget::updateBasicLimFsaY(int val)
{
    // set value in expert tab
    m_stabilization->yawMax->blockSignals( true );
    m_stabilization->yawMax->setValue((FULL_STICK_ANGLE_MAX - FULL_STICK_ANGLE_MIN) / 100.0 * val);
    m_stabilization->yawMax->blockSignals( false );
}

void ConfigStabilizationWidget::updateBasicLimFsrP(int val)
{
    // set value in expert tab
    m_stabilization->manualPitch->blockSignals( true );
    m_stabilization->manualPitch->setValue((FULL_STICK_RATE_MAX - FULL_STICK_RATE_MIN) / 100.0 * val);
    m_stabilization->manualPitch->blockSignals( false );
}

void ConfigStabilizationWidget::updateBasicLimFsrR(int val)
{
    // set value in expert tab
    m_stabilization->manualRoll->blockSignals( true );
    m_stabilization->manualRoll->setValue((FULL_STICK_RATE_MAX - FULL_STICK_RATE_MIN) / 100.0 * val);
    m_stabilization->manualRoll->blockSignals( false );
}

void ConfigStabilizationWidget::updateBasicLimFsrY(int val)
{
    // set value in expert tab
    m_stabilization->manualYaw->blockSignals( true );
    m_stabilization->manualYaw->setValue((FULL_STICK_RATE_MAX - FULL_STICK_RATE_MIN) / 100.0 * val);
    m_stabilization->manualYaw->blockSignals( false );
}

void ConfigStabilizationWidget::updateBasicLimMraP(int val)
{
    // set value in expert tab
    m_stabilization->maximumPitch->blockSignals( true );
    m_stabilization->maximumPitch->setValue((MAXIMUM_RATE_MAX - MAXIMUM_RATE_MIN) / 100.0 * val);
    m_stabilization->maximumPitch->blockSignals( false );
}

void ConfigStabilizationWidget::updateBasicLimMraR(int val)
{
    // set value in expert tab
    m_stabilization->maximumRoll->blockSignals( true );
    m_stabilization->maximumRoll->setValue((MAXIMUM_RATE_MAX - MAXIMUM_RATE_MIN) / 100.0 * val);
    m_stabilization->maximumRoll->blockSignals( false );
}

void ConfigStabilizationWidget::updateBasicLimMraY(int val)
{
    // set value in expert tab
    m_stabilization->maximumYaw->blockSignals( true );
    m_stabilization->maximumYaw->setValue((MAXIMUM_RATE_MAX - MAXIMUM_RATE_MIN) / 100.0 * val);
    m_stabilization->maximumYaw->blockSignals( false );
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
    updateRollKP(stabData.RollPI[StabilizationSettings::ROLLPI_KP]);
    m_stabilization->rollKi->setValue(stabData.RollPI[StabilizationSettings::ROLLPI_KI]);
    updateRollKI(stabData.RollPI[StabilizationSettings::ROLLPI_KI]);
    m_stabilization->rollILimit->setValue(stabData.RollPI[StabilizationSettings::ROLLPI_ILIMIT]);

    m_stabilization->pitchKp->setValue(stabData.PitchPI[StabilizationSettings::PITCHPI_KP]);
    updatePitchKP(stabData.PitchPI[StabilizationSettings::PITCHPI_KP]);
    m_stabilization->pitchKi->setValue(stabData.PitchPI[StabilizationSettings::PITCHPI_KI]);
    updatePitchKI(stabData.PitchPI[StabilizationSettings::PITCHPI_KI]);
    m_stabilization->pitchILimit->setValue(stabData.PitchPI[StabilizationSettings::PITCHPI_ILIMIT]);

    m_stabilization->yawKp->setValue(stabData.YawPI[StabilizationSettings::YAWPI_KP]);
    updateYawKP(stabData.YawPI[StabilizationSettings::YAWPI_KP]);
    m_stabilization->yawKi->setValue(stabData.YawPI[StabilizationSettings::YAWPI_KI]);
    updateYawKI(stabData.YawPI[StabilizationSettings::YAWPI_KI]);
    m_stabilization->yawILimit->setValue(stabData.YawPI[StabilizationSettings::YAWPI_ILIMIT]);

    m_stabilization->rollMax->setValue(stabData.RollMax);
    updateRollMax(stabData.RollMax);
    m_stabilization->pitchMax->setValue(stabData.PitchMax);
    updatePitchMax(stabData.PitchMax);
    m_stabilization->yawMax->setValue(stabData.YawMax);
    updateYawMax(stabData.YawMax);

    m_stabilization->manualRoll->setValue(stabData.ManualRate[StabilizationSettings::MANUALRATE_ROLL]);
    updateManualRoll(stabData.ManualRate[StabilizationSettings::MANUALRATE_ROLL]);
    m_stabilization->manualPitch->setValue(stabData.ManualRate[StabilizationSettings::MANUALRATE_PITCH]);
    updateManualPitch(stabData.ManualRate[StabilizationSettings::MANUALRATE_PITCH]);
    m_stabilization->manualYaw->setValue(stabData.ManualRate[StabilizationSettings::MANUALRATE_YAW]);
    updateManualYaw(stabData.ManualRate[StabilizationSettings::MANUALRATE_YAW]);

    m_stabilization->maximumRoll->setValue(stabData.MaximumRate[StabilizationSettings::MAXIMUMRATE_ROLL]);
    updateMaximumRoll(stabData.MaximumRate[StabilizationSettings::MAXIMUMRATE_ROLL]);
    m_stabilization->maximumPitch->setValue(stabData.MaximumRate[StabilizationSettings::MAXIMUMRATE_PITCH]);
    updateMaximumPitch(stabData.MaximumRate[StabilizationSettings::MAXIMUMRATE_PITCH]);
    m_stabilization->maximumYaw->setValue(stabData.MaximumRate[StabilizationSettings::MAXIMUMRATE_YAW]);
    updateMaximumYaw(stabData.MaximumRate[StabilizationSettings::MAXIMUMRATE_YAW]);

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
