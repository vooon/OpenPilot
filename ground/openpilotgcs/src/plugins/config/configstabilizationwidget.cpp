/**
 ******************************************************************************
 * @file       configstabilizationwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
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

// constants for URLs to be loaded upon clicking the respective buttons

static const QString GENERAL_HELP="http://wiki.openpilot.org/display/Doc/Stabilization+panel";
static const QString INNER_LOOP_HELP="http://wiki.openpilot.org/display/Doc/Stabilization+panel";
static const QString OUTER_LOOP_HELP="http://wiki.openpilot.org/display/Doc/Stabilization+panel";
static const QString STICKRANGE_HELP="http://wiki.openpilot.org/display/Doc/Stabilization+panel";

// ------------------------------------------------------------------------------------------------

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
    connect(m_stabilization->stabilizationReloadBoardData, SIGNAL(clicked()), this, SLOT (reloadBoardValues()));
    connect(m_stabilization->linkRatePitchRoll, SIGNAL(clicked()), this, SLOT (activateLinkRate()));
    connect(m_stabilization->linkRateRP, SIGNAL(clicked()), this, SLOT (activateLinkRateExpert()));
    connect(m_stabilization->linkAttPitchRoll, SIGNAL(clicked()), this, SLOT (activateLinkAttitude()));
    connect(m_stabilization->linkAttitudeRP, SIGNAL(clicked()), this, SLOT (activateLinkAttitudeExpert()));

    // connect buttons to activate external links to some website
    connect(m_stabilization->stabilizationHelp, SIGNAL(clicked()), this, SLOT(openHelp()));
    connect(m_stabilization->wikiLinkStabInner, SIGNAL(clicked()), this, SLOT (wikiLinkStabInner()));
    connect(m_stabilization->wikiLinkStabOuter, SIGNAL(clicked()), this, SLOT (wikiLinkStabOuter()));
    connect(m_stabilization->wikiLinkStabSticks, SIGNAL(clicked()), this, SLOT (wikiLinkStabSticks()));
}

ConfigStabilizationWidget::~ConfigStabilizationWidget()
{
    // Do nothing
}

void ConfigStabilizationWidget::reloadBoardValues()
{
    refreshWidgetsValues();
}

void ConfigStabilizationWidget::UpdateBasicSetting( double val, QCheckBox *linkBox, QDoubleSpinBox *expertBox,
                                                    double min, double max, QSpinBox *basicBox, QSlider *basicSlider, QSlider *basicLinkedSlider )
{
    if ((linkBox != NULL) && (linkBox->isChecked())) {
        expertBox->setValue(val);
    }
    // update basic tab dial
    if( val < min || val > max ) {
        basicBox->setStyleSheet("QSpinBox { color : red; }");
        basicSlider->setStyleSheet("QSlider { border: 2px solid #F00; }");
        basicBox->setValue( 0 );
        basicSlider->setEnabled( false );
        basicBox->setEnabled( false );
    } else {
        // calc position normalized to 0-100%
        basicBox->setStyleSheet("QSpinBox { color : black; }");
        basicSlider->setStyleSheet("QSlider { border: 0px solid #F00; }");
        basicSlider->setEnabled( true );
        basicBox->setEnabled( true );
        int basicVal = ( val - min) / ( max - min) * 100;
        basicSlider->blockSignals( true );
        basicSlider->setValue( basicVal );
        if ((linkBox != NULL) && (linkBox->isChecked())) {
            basicLinkedSlider->blockSignals( true );
            basicLinkedSlider->setValue( basicVal );
            basicLinkedSlider->blockSignals( false );
        }
        basicSlider->blockSignals( false );
        basicBox->setValue( basicVal );
    }
}

void ConfigStabilizationWidget::updateRateRollKP(double val)
{
    UpdateBasicSetting( val, m_stabilization->linkRateRP, m_stabilization->ratePitchKp,
                        RATE_KP_MIN, RATE_KP_MAX, m_stabilization->RPRL, m_stabilization->rateRollProp, m_stabilization->ratePitchProp );
}

void ConfigStabilizationWidget::updateRateRollKI(double val)
{
    UpdateBasicSetting( val, m_stabilization->linkRateRP, m_stabilization->ratePitchKi,
                        RATE_KI_MIN, RATE_KI_MAX, m_stabilization->RIRL, m_stabilization->rateRollInt, m_stabilization->ratePitchInt );
}

void ConfigStabilizationWidget::updateRateRollILimit(double val)
{
    if (m_stabilization->linkRateRP->isChecked()) {
        m_stabilization->ratePitchILimit->setValue(val);
    }
}

void ConfigStabilizationWidget::updateRatePitchKP(double val)
{
    UpdateBasicSetting( val, m_stabilization->linkRateRP, m_stabilization->rateRollKp,
                        RATE_KP_MIN, RATE_KP_MAX, m_stabilization->RPPL, m_stabilization->ratePitchProp, m_stabilization->rateRollProp );
}

void ConfigStabilizationWidget::updateRatePitchKI(double val)
{
    UpdateBasicSetting( val, m_stabilization->linkRateRP, m_stabilization->rateRollKi,
                        RATE_KI_MIN, RATE_KI_MAX, m_stabilization->RIPL, m_stabilization->ratePitchInt, m_stabilization->rateRollInt );
}

void ConfigStabilizationWidget::updateRateYawKP(double val)
{
    UpdateBasicSetting( val, NULL, NULL, RATE_KP_MIN, RATE_KP_MAX, m_stabilization->RPYL, m_stabilization->rateYawProp, NULL );
}

void ConfigStabilizationWidget::updateRateYawKI(double val)
{
    UpdateBasicSetting( val, NULL, NULL, RATE_KI_MIN, RATE_KI_MAX, m_stabilization->RIYL, m_stabilization->rateYawInt, NULL );
}

void ConfigStabilizationWidget::updateRatePitchILimit(double val)
{
    if (m_stabilization->linkRateRP->isChecked()) {
        m_stabilization->rateRollILimit->setValue(val);
    }
}

void ConfigStabilizationWidget::updateRollKP(double val)
{
    UpdateBasicSetting( val, m_stabilization->linkAttitudeRP, m_stabilization->pitchKp,
                        ATTITUDE_KP_MIN, ATTITUDE_KP_MAX, m_stabilization->APRL, m_stabilization->attRollProp, m_stabilization->attPitchProp );
}

void ConfigStabilizationWidget::updateRollKI(double val)
{
    UpdateBasicSetting( val, m_stabilization->linkAttitudeRP, m_stabilization->pitchKi,
                        ATTITUDE_KI_MIN, ATTITUDE_KI_MAX, m_stabilization->AIRL, m_stabilization->attRollInt, m_stabilization->attPitchInt );
}

void ConfigStabilizationWidget::updateRollILimit(double val)
{
    if (m_stabilization->linkAttitudeRP->isChecked()) {
        m_stabilization->pitchILimit->setValue(val);
    }
}

void ConfigStabilizationWidget::updatePitchKP(double val)
{
    UpdateBasicSetting( val, m_stabilization->linkAttitudeRP, m_stabilization->rollKp,
                        ATTITUDE_KP_MIN, ATTITUDE_KP_MAX, m_stabilization->APPL, m_stabilization->attPitchProp, m_stabilization->attRollProp );
}

void ConfigStabilizationWidget::updatePitchKI(double val)
{
    UpdateBasicSetting( val, m_stabilization->linkAttitudeRP, m_stabilization->rollKi,
                        ATTITUDE_KI_MIN, ATTITUDE_KI_MAX, m_stabilization->AIPL, m_stabilization->attPitchInt, m_stabilization->attRollInt );
}

void ConfigStabilizationWidget::updatePitchILimit(double val)
{
    if (m_stabilization->linkAttitudeRP->isChecked()) {
        m_stabilization->rollILimit->setValue(val);
    }
}

void ConfigStabilizationWidget::updateYawKP(double val)
{
    UpdateBasicSetting( val, NULL, NULL, ATTITUDE_KP_MIN, ATTITUDE_KP_MAX, m_stabilization->APYL, m_stabilization->attYawProp, NULL );
}

void ConfigStabilizationWidget::updateYawKI(double val)
{
    UpdateBasicSetting( val, NULL, NULL, ATTITUDE_KI_MIN, ATTITUDE_KI_MAX, m_stabilization->AIYL, m_stabilization->attYawInt, NULL );
}

void ConfigStabilizationWidget::updatePitchMax(int val)
{
    UpdateBasicSetting( val, NULL, NULL, FULL_STICK_ANGLE_MIN, FULL_STICK_ANGLE_MAX, m_stabilization->fsaPL, m_stabilization->fsaP, NULL );
}

void ConfigStabilizationWidget::updateRollMax(int val)
{
    UpdateBasicSetting( val, NULL, NULL, FULL_STICK_ANGLE_MIN, FULL_STICK_ANGLE_MAX, m_stabilization->fsaRL, m_stabilization->fsaR, NULL );
}

void ConfigStabilizationWidget::updateYawMax(int val)
{
    UpdateBasicSetting( val, NULL, NULL, FULL_STICK_ANGLE_MIN, FULL_STICK_ANGLE_MAX, m_stabilization->fsaYL, m_stabilization->fsaY, NULL );
}

void ConfigStabilizationWidget::updateManualPitch(int val)
{
    UpdateBasicSetting( val, NULL, NULL, FULL_STICK_RATE_MIN, FULL_STICK_RATE_MAX, m_stabilization->fsrPL, m_stabilization->fsrP, NULL );
}

void ConfigStabilizationWidget::updateManualRoll(int val)
{
    UpdateBasicSetting( val, NULL, NULL, FULL_STICK_RATE_MIN, FULL_STICK_RATE_MAX, m_stabilization->fsrRL, m_stabilization->fsrR, NULL );
}

void ConfigStabilizationWidget::updateManualYaw(int val)
{
    UpdateBasicSetting( val, NULL, NULL, FULL_STICK_RATE_MIN, FULL_STICK_RATE_MAX, m_stabilization->fsrYL, m_stabilization->fsrY, NULL );
}


void ConfigStabilizationWidget::updateMaximumPitch(int val)
{
    UpdateBasicSetting( val, NULL, NULL, MAXIMUM_RATE_MIN, MAXIMUM_RATE_MAX, m_stabilization->mraPL, m_stabilization->mraP, NULL );
}

void ConfigStabilizationWidget::updateMaximumRoll(int val)
{
    UpdateBasicSetting( val, NULL, NULL, MAXIMUM_RATE_MIN, MAXIMUM_RATE_MAX, m_stabilization->mraRL, m_stabilization->mraR, NULL );
}

void ConfigStabilizationWidget::updateMaximumYaw(int val)
{
    UpdateBasicSetting( val, NULL, NULL, MAXIMUM_RATE_MIN, MAXIMUM_RATE_MAX, m_stabilization->mraYL, m_stabilization->mraY, NULL );
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
    QDesktopServices::openUrl( QUrl(GENERAL_HELP, QUrl::StrictMode) );
}

void ConfigStabilizationWidget::wikiLinkStabInner()
{
    QDesktopServices::openUrl( QUrl(INNER_LOOP_HELP, QUrl::StrictMode) );
}

void ConfigStabilizationWidget::wikiLinkStabOuter()
{
    QDesktopServices::openUrl( QUrl(OUTER_LOOP_HELP, QUrl::StrictMode) );
}

void ConfigStabilizationWidget::wikiLinkStabSticks()
{
    QDesktopServices::openUrl( QUrl(STICKRANGE_HELP, QUrl::StrictMode) );
}
