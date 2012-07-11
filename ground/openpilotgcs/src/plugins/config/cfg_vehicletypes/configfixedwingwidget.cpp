/**
 ******************************************************************************
 *
 * @file       configfixedwidget.cpp
 * @author     E. Lafargue & The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief ccpm configuration panel
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
#include "configfixedwingwidget.h"
#include "configvehicletypewidget.h"
#include "mixersettings.h"

#include <QDebug>
#include <QStringList>
#include <QtGui/QWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QBrush>
#include <math.h>
#include <QMessageBox>

#include "mixersettings.h"
#include "systemsettings.h"
#include "actuatorsettings.h"
#include "actuatorcommand.h"


/**
 Constructor
 */
ConfigFixedWingWidget::ConfigFixedWingWidget(Ui_AircraftWidget *aircraft, QWidget *parent) : VehicleConfig(parent)
{
    m_aircraft = aircraft;    
}

/**
 Destructor
 */
ConfigFixedWingWidget::~ConfigFixedWingWidget()
{
   // Do nothing
}


/**
 Virtual function to setup the UI
 */
void ConfigFixedWingWidget::setupUI(QString frameType)
{
    Q_ASSERT(m_aircraft);

	if (frameType == "FixedWing" || frameType == "Elevator aileron rudder") {
        // Setup the UI
        setComboCurrentIndex(m_aircraft->aircraftType, m_aircraft->aircraftType->findText("Fixed Wing"));
        setComboCurrentIndex(m_aircraft->fixedWingType, m_aircraft->fixedWingType->findText("Elevator aileron rudder"));
        m_aircraft->fwRudder1ChannelBox->setEnabled(true);
        m_aircraft->fwRudder1Label->setEnabled(true);
        m_aircraft->fwRudder2ChannelBox->setEnabled(true);
        m_aircraft->fwRudder2Label->setEnabled(true);
        m_aircraft->fwElevator1ChannelBox->setEnabled(true);
        m_aircraft->fwElevator1Label->setEnabled(true);
        m_aircraft->fwElevator2ChannelBox->setEnabled(true);
        m_aircraft->fwElevator2Label->setEnabled(true);
        m_aircraft->fwAileron1ChannelBox->setEnabled(true);
        m_aircraft->fwAileron1Label->setEnabled(true);
        m_aircraft->fwAileron2ChannelBox->setEnabled(true);
        m_aircraft->fwAileron2Label->setEnabled(true);
		
        m_aircraft->fwAileron1Label->setText("Aileron 1");
        m_aircraft->fwAileron2Label->setText("Aileron 2");
        m_aircraft->fwElevator1Label->setText("Elevator 1");
        m_aircraft->fwElevator2Label->setText("Elevator 2");
        m_aircraft->elevonMixBox->setHidden(true);
		
    } else if (frameType == "FixedWingElevon" || frameType == "Elevon") {
        setComboCurrentIndex(m_aircraft->aircraftType, m_aircraft->aircraftType->findText("Fixed Wing"));
        setComboCurrentIndex(m_aircraft->fixedWingType, m_aircraft->fixedWingType->findText("Elevon"));
        m_aircraft->fwAileron1Label->setText("Elevon 1");
        m_aircraft->fwAileron2Label->setText("Elevon 2");
        m_aircraft->fwElevator1ChannelBox->setEnabled(false);
        m_aircraft->fwElevator1Label->setEnabled(false);
        m_aircraft->fwElevator2ChannelBox->setEnabled(false);
        m_aircraft->fwElevator2Label->setEnabled(false);
        m_aircraft->fwRudder1ChannelBox->setEnabled(true);
        m_aircraft->fwRudder1Label->setEnabled(true);
        m_aircraft->fwRudder2ChannelBox->setEnabled(true);
        m_aircraft->fwRudder2Label->setEnabled(true);
        m_aircraft->fwElevator1Label->setText("Elevator 1");
        m_aircraft->fwElevator2Label->setText("Elevator 2");
        m_aircraft->elevonMixBox->setHidden(false);
        m_aircraft->elevonLabel1->setText("Roll");
        m_aircraft->elevonLabel2->setText("Pitch");
		
	} else if (frameType == "FixedWingVtail" || frameType == "Vtail") {
        setComboCurrentIndex(m_aircraft->aircraftType, m_aircraft->aircraftType->findText("Fixed Wing"));
        setComboCurrentIndex(m_aircraft->fixedWingType, m_aircraft->fixedWingType->findText("Vtail"));
        m_aircraft->fwRudder1ChannelBox->setEnabled(false);
        m_aircraft->fwRudder1Label->setEnabled(false);
        m_aircraft->fwRudder2ChannelBox->setEnabled(false);
        m_aircraft->fwRudder2Label->setEnabled(false);
        m_aircraft->fwElevator1ChannelBox->setEnabled(true);
        m_aircraft->fwElevator1Label->setEnabled(true);
        m_aircraft->fwElevator1Label->setText("Vtail 1");
        m_aircraft->fwElevator2Label->setText("Vtail 2");
        m_aircraft->elevonMixBox->setHidden(false);
        m_aircraft->fwElevator2ChannelBox->setEnabled(true);
        m_aircraft->fwElevator2Label->setEnabled(true);
        m_aircraft->fwAileron1Label->setText("Aileron 1");
        m_aircraft->fwAileron2Label->setText("Aileron 2");
        m_aircraft->elevonLabel1->setText("Rudder");
        m_aircraft->elevonLabel2->setText("Pitch");
	}
}

void ConfigFixedWingWidget::ResetActuators(GUIConfigDataUnion* configData)
{
    configData->fixedwing.FixedWingPitch1 = 0;
    configData->fixedwing.FixedWingPitch2 = 0;
    configData->fixedwing.FixedWingRoll1 = 0;
    configData->fixedwing.FixedWingRoll2 = 0;
    configData->fixedwing.FixedWingYaw1 = 0;
    configData->fixedwing.FixedWingYaw2 = 0;
    configData->fixedwing.FixedWingFlap1 = 0;
    configData->fixedwing.FixedWingFlap2 = 0;
    configData->fixedwing.FixedWingSpoiler1 = 0;
    configData->fixedwing.FixedWingSpoiler2 = 0;
    configData->fixedwing.FixedWingThrottle = 0;
}

QStringList ConfigFixedWingWidget::getChannelDescriptions()
{
    int i;
    QStringList channelDesc;

    // init a channel_numelem list of channel desc defaults
    for (i=0; i < (int)(ConfigFixedWingWidget::CHANNEL_NUMELEM); i++)
    {
        channelDesc.append(QString("-"));
    }

    // get the gui config data
    GUIConfigDataUnion configData = GetConfigData();

    if (configData.fixedwing.FixedWingPitch1 > 0)
        channelDesc[configData.fixedwing.FixedWingPitch1-1] = QString("FixedWingPitch1");
    if (configData.fixedwing.FixedWingPitch2 > 0)
        channelDesc[configData.fixedwing.FixedWingPitch2-1] = QString("FixedWingPitch2");
    if (configData.fixedwing.FixedWingRoll1 > 0)
        channelDesc[configData.fixedwing.FixedWingRoll1-1] = QString("FixedWingRoll1");
    if (configData.fixedwing.FixedWingRoll2 > 0)
        channelDesc[configData.fixedwing.FixedWingRoll2-1] = QString("FixedWingRoll2");
    if (configData.fixedwing.FixedWingYaw1 > 0)
        channelDesc[configData.fixedwing.FixedWingYaw1-1] = QString("FixedWingYaw1");
    if (configData.fixedwing.FixedWingYaw2 > 0)
        channelDesc[configData.fixedwing.FixedWingYaw2-1] = QString("FixedWingYaw2");
    if (configData.fixedwing.FixedWingFlap1 > 0)
        channelDesc[configData.fixedwing.FixedWingFlap1-1] = QString("FixedWingFlap1");
    if (configData.fixedwing.FixedWingFlap2 > 0)
        channelDesc[configData.fixedwing.FixedWingFlap2-1] = QString("FixedWingFlap2");
    if (configData.fixedwing.FixedWingSpoiler1 > 0)
        channelDesc[configData.fixedwing.FixedWingSpoiler1-1] = QString("FixedWingSpoiler1");
    if (configData.fixedwing.FixedWingSpoiler2 > 0)
        channelDesc[configData.fixedwing.FixedWingSpoiler2-1] = QString("FixedWingSpoiler2");
    if (configData.fixedwing.FixedWingThrottle > 0)
        channelDesc[configData.fixedwing.FixedWingThrottle-1] = QString("FixedWingThrottle");

    return channelDesc;
}

/**
 Virtual function to update the UI widget objects
 */
QString ConfigFixedWingWidget::updateConfigObjectsFromWidgets()
{
	QString airframeType = "FixedWing";
	
	// Save the curve (common to all Fixed wing frames)
    UAVDataObject* mixer = dynamic_cast<UAVDataObject*>(getObjectManager()->getObject(QString("MixerSettings")));
    Q_ASSERT(mixer);

	// Remove Feed Forward, it is pointless on a plane:
    UAVObjectField* field = mixer->getField(QString("FeedForward"));
	field->setDouble(0);
	
    // Set the throttle curve
    setThrottleCurve(mixer,VehicleConfig::MIXER_THROTTLECURVE1, m_aircraft->fixedWingThrottle->getCurve());

	//All airframe types must start with "FixedWing"
	if (m_aircraft->fixedWingType->currentText() == "Elevator aileron rudder" ) {
		airframeType = "FixedWing";
        setupFrameFixedWing( airframeType );
	} else if (m_aircraft->fixedWingType->currentText() == "Elevon") {
		airframeType = "FixedWingElevon";
		setupFrameElevon( airframeType );
	} else { // "Vtail"
		airframeType = "FixedWingVtail";
		setupFrameVtail( airframeType );
	}

	return airframeType;
}


/**
 Virtual function to refresh the UI widget values
 */
void ConfigFixedWingWidget::refreshWidgetsValues(QString frameType)
{
    Q_ASSERT(m_aircraft);

    GUIConfigDataUnion config = GetConfigData();
    fixedGUISettingsStruct fixed = config.fixedwing;

    // Then retrieve how channels are setup
    setComboCurrentIndex(m_aircraft->fwEngineChannelBox, fixed.FixedWingThrottle);
    setComboCurrentIndex(m_aircraft->fwAileron1ChannelBox, fixed.FixedWingRoll1);
    setComboCurrentIndex(m_aircraft->fwAileron2ChannelBox, fixed.FixedWingRoll2);
    setComboCurrentIndex(m_aircraft->fwElevator1ChannelBox, fixed.FixedWingPitch1);
    setComboCurrentIndex(m_aircraft->fwElevator2ChannelBox, fixed.FixedWingPitch2);
    setComboCurrentIndex(m_aircraft->fwRudder1ChannelBox, fixed.FixedWingYaw1);
    setComboCurrentIndex(m_aircraft->fwRudder2ChannelBox, fixed.FixedWingYaw2);
    setComboCurrentIndex(m_aircraft->fwFlap1ChannelBox, fixed.FixedWingFlap1);
    setComboCurrentIndex(m_aircraft->fwFlap2ChannelBox, fixed.FixedWingFlap2);
    setComboCurrentIndex(m_aircraft->fwSpoiler1ChannelBox, fixed.FixedWingSpoiler1);
    setComboCurrentIndex(m_aircraft->fwSpoiler2ChannelBox, fixed.FixedWingSpoiler2);

    UAVDataObject* mixer= dynamic_cast<UAVDataObject*>(getObjectManager()->getObject(QString("MixerSettings")));
    Q_ASSERT(mixer);

    int channel;
	if (frameType == "FixedWingElevon") {
        // If the airframe is elevon, restore the slider setting
        // Find the channel number for Elevon1 (FixedWingRoll1)
        channel = m_aircraft->fwAileron1ChannelBox->currentIndex()-1;
        if (channel > -1) { // If for some reason the actuators were incoherent, we might fail here, hence the check.
            m_aircraft->elevonSlider1->setValue(getMixerVectorValue(mixer,channel,VehicleConfig::MIXERVECTOR_ROLL)*100);
            m_aircraft->elevonSlider2->setValue(getMixerVectorValue(mixer,channel,VehicleConfig::MIXERVECTOR_PITCH)*100);
		}
	}
    if (frameType == "FixedWingVtail") {
        channel = m_aircraft->fwElevator1ChannelBox->currentIndex()-1;
        if (channel > -1) { // If for some reason the actuators were incoherent, we might fail here, hence the check.
            m_aircraft->elevonSlider1->setValue(getMixerVectorValue(mixer,channel,VehicleConfig::MIXERVECTOR_YAW)*100);
            m_aircraft->elevonSlider2->setValue(getMixerVectorValue(mixer,channel,VehicleConfig::MIXERVECTOR_PITCH)*100);
        }
	}	
}



/**
 Setup Elevator/Aileron/Rudder airframe.
 
 If both Aileron channels are set to 'None' (EasyStar), do Pitch/Rudder mixing
 
 Returns False if impossible to create the mixer.
 */
bool ConfigFixedWingWidget::setupFrameFixedWing(QString airframeType)
{
    // Check coherence:
	//Show any config errors in GUI
    if (throwConfigError(airframeType)) {
			return false;
		}

    // Now setup the channels:
	
    GUIConfigDataUnion config = GetConfigData();
    ResetActuators(&config);

    config.fixedwing.FixedWingPitch1 = m_aircraft->fwElevator1ChannelBox->currentIndex();
    config.fixedwing.FixedWingPitch2 = m_aircraft->fwElevator2ChannelBox->currentIndex();
    config.fixedwing.FixedWingRoll1 = m_aircraft->fwAileron1ChannelBox->currentIndex();
    config.fixedwing.FixedWingRoll2 = m_aircraft->fwAileron2ChannelBox->currentIndex();
    config.fixedwing.FixedWingYaw1 = m_aircraft->fwRudder1ChannelBox->currentIndex();
    config.fixedwing.FixedWingYaw2 = m_aircraft->fwRudder2ChannelBox->currentIndex();
    config.fixedwing.FixedWingFlap1 = m_aircraft->fwFlap1ChannelBox->currentIndex();
    config.fixedwing.FixedWingFlap2 = m_aircraft->fwFlap2ChannelBox->currentIndex();
    config.fixedwing.FixedWingSpoiler1 = m_aircraft->fwSpoiler1ChannelBox->currentIndex();
    config.fixedwing.FixedWingSpoiler2 = m_aircraft->fwSpoiler2ChannelBox->currentIndex();
    config.fixedwing.FixedWingThrottle = m_aircraft->fwEngineChannelBox->currentIndex();

    SetConfigData(config);
	
    UAVDataObject* mixer = dynamic_cast<UAVDataObject*>(getObjectManager()->getObject(QString("MixerSettings")));
    Q_ASSERT(mixer);

    // ... and compute the matrix:
    // In order to make code a bit nicer, we assume:
    // - Channel dropdowns start with 'None', then 0 to 7

    // 1. Assign the servo/motor/none for each channel

    //disable all
    for (int channel=0; channel<VehicleConfig::CHANNEL_NUMELEM; channel++)
    {
        setMixerType(mixer,channel,VehicleConfig::MIXERTYPE_DISABLED);
        resetMixerVector(mixer, channel);
    }

    //motor
    int motorChannel = m_aircraft->fwEngineChannelBox->currentIndex()-1;
    setMixerType(mixer,motorChannel,VehicleConfig::MIXERTYPE_MOTOR);
    setMixerVectorValue(mixer,motorChannel,VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);

    //rudder
    int rudderChannel = m_aircraft->fwRudder1ChannelBox->currentIndex()-1;
    setMixerType(mixer,rudderChannel,VehicleConfig::MIXERTYPE_SERVO);
    setMixerVectorValue(mixer, rudderChannel, VehicleConfig::MIXERVECTOR_YAW, 127);

    //ailerons
    int aileronChannel1 = m_aircraft->fwAileron1ChannelBox->currentIndex()-1;
    int aileronChannel2 = m_aircraft->fwAileron2ChannelBox->currentIndex()-1;
    if (aileronChannel1 > -1) {
        setMixerType(mixer,aileronChannel1,VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, aileronChannel1, VehicleConfig::MIXERVECTOR_ROLL, 127);

        setMixerType(mixer,aileronChannel2,VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, aileronChannel2, VehicleConfig::MIXERVECTOR_ROLL, 127);
    }

    //elevators
    int elevatorChannel1 = m_aircraft->fwElevator1ChannelBox->currentIndex()-1;
    int elevatorChannel2 = m_aircraft->fwElevator2ChannelBox->currentIndex()-1;
    if (elevatorChannel1 > -1) {
        setMixerType(mixer,elevatorChannel1,VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, elevatorChannel1, VehicleConfig::MIXERVECTOR_PITCH, 127);

        setMixerType(mixer,elevatorChannel2,VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, elevatorChannel2, VehicleConfig::MIXERVECTOR_PITCH, 127);
    }

    //Setup flaps and spoilers
    configureFlapsSpoilers(mixer, aileronChannel1, aileronChannel2);


    m_aircraft->fwStatusLabel->setText("Mixer generated");
	
    return true;
}


/**
 Setup Elevon
 */
bool ConfigFixedWingWidget::setupFrameElevon(QString airframeType)
{
    // Check coherence:
	//Show any config errors in GUI
    if (throwConfigError(airframeType)) {
        return false;
    }
	
    GUIConfigDataUnion config = GetConfigData();
    ResetActuators(&config);

    config.fixedwing.FixedWingRoll1 = m_aircraft->fwAileron1ChannelBox->currentIndex();
    config.fixedwing.FixedWingRoll2 = m_aircraft->fwAileron2ChannelBox->currentIndex();
    config.fixedwing.FixedWingYaw1 = m_aircraft->fwRudder1ChannelBox->currentIndex();
    config.fixedwing.FixedWingYaw2 = m_aircraft->fwRudder2ChannelBox->currentIndex();
    config.fixedwing.FixedWingFlap1 = m_aircraft->fwFlap1ChannelBox->currentIndex();
    config.fixedwing.FixedWingFlap2 = m_aircraft->fwFlap2ChannelBox->currentIndex();
    config.fixedwing.FixedWingSpoiler1 = m_aircraft->fwSpoiler1ChannelBox->currentIndex();
    config.fixedwing.FixedWingSpoiler2 = m_aircraft->fwSpoiler2ChannelBox->currentIndex();
    config.fixedwing.FixedWingThrottle = m_aircraft->fwEngineChannelBox->currentIndex();

    SetConfigData(config);
	    
    UAVDataObject* mixer = dynamic_cast<UAVDataObject*>(getObjectManager()->getObject(QString("MixerSettings")));
    Q_ASSERT(mixer);

    // Save the curve:
    // ... and compute the matrix:
    // In order to make code a bit nicer, we assume:
    // - Channel dropdowns start with 'None', then 0 to 7

    // 1. Assign the servo/motor/none for each channel

    double value;
    //disable all
    for (int channel=0; channel<VehicleConfig::CHANNEL_NUMELEM; channel++)
    {
        setMixerType(mixer,channel,VehicleConfig::MIXERTYPE_DISABLED);
        resetMixerVector(mixer, channel);
    }

    //motor
    int motorChannel = m_aircraft->fwEngineChannelBox->currentIndex()-1;
    setMixerType(mixer,motorChannel,VehicleConfig::MIXERTYPE_MOTOR);
    setMixerVectorValue(mixer,motorChannel,VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);

    //rudders
    int rudderChannel1 = m_aircraft->fwRudder1ChannelBox->currentIndex()-1;
    setMixerType(mixer,rudderChannel1,VehicleConfig::MIXERTYPE_SERVO);
    setMixerVectorValue(mixer, rudderChannel1, VehicleConfig::MIXERVECTOR_YAW, 127);

    int rudderChannel2 = m_aircraft->fwRudder2ChannelBox->currentIndex()-1;
    setMixerType(mixer,rudderChannel2,VehicleConfig::MIXERTYPE_SERVO);
    setMixerVectorValue(mixer, rudderChannel2, VehicleConfig::MIXERVECTOR_YAW, -127);

    //ailerons
    int aileronChannel1 = m_aircraft->fwAileron1ChannelBox->currentIndex()-1;
    int aileronChannel2 = m_aircraft->fwAileron2ChannelBox->currentIndex()-1;
    if (aileronChannel1 > -1) {
        setMixerType(mixer,aileronChannel1,VehicleConfig::MIXERTYPE_SERVO);
        value = (double)(m_aircraft->elevonSlider2->value()*1.27);
        setMixerVectorValue(mixer, aileronChannel1, VehicleConfig::MIXERVECTOR_PITCH, value);
        value = (double)(m_aircraft->elevonSlider1->value()*1.27);
        setMixerVectorValue(mixer, aileronChannel1, VehicleConfig::MIXERVECTOR_ROLL, value);

        setMixerType(mixer,aileronChannel2,VehicleConfig::MIXERTYPE_SERVO);
        value = (double)(m_aircraft->elevonSlider2->value()*1.27);
        setMixerVectorValue(mixer, aileronChannel2, VehicleConfig::MIXERVECTOR_PITCH, value);
        value = (double)(m_aircraft->elevonSlider1->value()*1.27);
        setMixerVectorValue(mixer, aileronChannel2, VehicleConfig::MIXERVECTOR_ROLL, -value);
    }

    //Setup flaps and spoilers
    configureFlapsSpoilers(mixer, aileronChannel1, aileronChannel2);


    m_aircraft->fwStatusLabel->setText("Mixer generated");
    return true;
}



/**
 Setup VTail
 */
bool ConfigFixedWingWidget::setupFrameVtail(QString airframeType)
{
    // Check coherence:
	//Show any config errors in GUI
    if (throwConfigError(airframeType)) {
        return false;
    }
	
    GUIConfigDataUnion config = GetConfigData();
    ResetActuators(&config);

    config.fixedwing.FixedWingPitch1 = m_aircraft->fwElevator1ChannelBox->currentIndex();
    config.fixedwing.FixedWingPitch2 = m_aircraft->fwElevator2ChannelBox->currentIndex();
    config.fixedwing.FixedWingRoll1 = m_aircraft->fwAileron1ChannelBox->currentIndex();
    config.fixedwing.FixedWingRoll2 = m_aircraft->fwAileron2ChannelBox->currentIndex();
    config.fixedwing.FixedWingFlap1 = m_aircraft->fwFlap1ChannelBox->currentIndex();
    config.fixedwing.FixedWingFlap2 = m_aircraft->fwFlap2ChannelBox->currentIndex();
    config.fixedwing.FixedWingSpoiler1 = m_aircraft->fwSpoiler1ChannelBox->currentIndex();
    config.fixedwing.FixedWingSpoiler2 = m_aircraft->fwSpoiler2ChannelBox->currentIndex();
    config.fixedwing.FixedWingThrottle = m_aircraft->fwEngineChannelBox->currentIndex();

    SetConfigData(config);
	    
    UAVDataObject* mixer = dynamic_cast<UAVDataObject*>(getObjectManager()->getObject(QString("MixerSettings")));
    Q_ASSERT(mixer);

    // Save the curve:
    // ... and compute the matrix:
    // In order to make code a bit nicer, we assume:
    // - Channel dropdowns start with 'None', then 0 to 7

    // 1. Assign the servo/motor/none for each channel

    double value;
    //disable all
    for (int channel=0; channel<VehicleConfig::CHANNEL_NUMELEM; channel++)
    {
        setMixerType(mixer,channel,VehicleConfig::MIXERTYPE_DISABLED);
        resetMixerVector(mixer, channel);
    }

    //motor
    int motorChannel = m_aircraft->fwEngineChannelBox->currentIndex()-1;
    setMixerType(mixer,motorChannel,VehicleConfig::MIXERTYPE_MOTOR);
    setMixerVectorValue(mixer,motorChannel,VehicleConfig::MIXERVECTOR_THROTTLECURVE1, 127);

    //rudders
    int rudderChannel1 = m_aircraft->fwRudder1ChannelBox->currentIndex()-1;
    setMixerType(mixer,rudderChannel1,VehicleConfig::MIXERTYPE_SERVO);
    setMixerVectorValue(mixer, rudderChannel1, VehicleConfig::MIXERVECTOR_YAW, 127);

    int rudderChannel2 = m_aircraft->fwRudder2ChannelBox->currentIndex()-1;
    setMixerType(mixer,rudderChannel2,VehicleConfig::MIXERTYPE_SERVO);
    setMixerVectorValue(mixer, rudderChannel2, VehicleConfig::MIXERVECTOR_YAW, -127);

    //ailerons
    int aileronChannel1 = m_aircraft->fwAileron1ChannelBox->currentIndex()-1;
    int aileronChannel2 = m_aircraft->fwAileron2ChannelBox->currentIndex()-1;
    if (aileronChannel1 > -1) {
        setMixerType(mixer,aileronChannel1,VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, aileronChannel1, VehicleConfig::MIXERVECTOR_ROLL, 127);

        setMixerType(mixer,aileronChannel2,VehicleConfig::MIXERTYPE_SERVO);
        setMixerVectorValue(mixer, aileronChannel2, VehicleConfig::MIXERVECTOR_ROLL, -127);
    }

    //vtail
    int elevatorChannel1 = m_aircraft->fwElevator1ChannelBox->currentIndex()-1;
    int elevatorChannel2 = m_aircraft->fwElevator2ChannelBox->currentIndex()-1;
    if (elevatorChannel1 > -1) {
        setMixerType(mixer,elevatorChannel1,VehicleConfig::MIXERTYPE_SERVO);
        value = (double)(m_aircraft->elevonSlider2->value()*1.27);
        setMixerVectorValue(mixer, elevatorChannel1, VehicleConfig::MIXERVECTOR_PITCH, value);
        value = (double)(m_aircraft->elevonSlider1->value()*1.27);
        setMixerVectorValue(mixer, elevatorChannel1, VehicleConfig::MIXERVECTOR_YAW, value);

        setMixerType(mixer,elevatorChannel2,VehicleConfig::MIXERTYPE_SERVO);
        value = (double)(m_aircraft->elevonSlider2->value()*1.27);
        setMixerVectorValue(mixer, elevatorChannel2, VehicleConfig::MIXERVECTOR_PITCH, value);
        value = (double)(m_aircraft->elevonSlider1->value()*1.27);
        setMixerVectorValue(mixer, elevatorChannel2, VehicleConfig::MIXERVECTOR_YAW, -value);
    }

    //Setup flaps and spoilers
    configureFlapsSpoilers(mixer, aileronChannel1, aileronChannel2);

    m_aircraft->fwStatusLabel->setText("Mixer generated");
    return true;
}


void ConfigFixedWingWidget::configureFlapsSpoilers(UAVDataObject* mixer, int aileronChannel1, int aileronChannel2)
{
    //flaps
    int flapsChannel1 = m_aircraft->fwFlap1ChannelBox->currentIndex()-1;
    int flapsChannel2 = m_aircraft->fwFlap2ChannelBox->currentIndex()-1;
    if (flapsChannel1 > -1 && m_aircraft->fwFlapsType->currentText()!="None") {

        if (m_aircraft->fwFlapsType->currentText()=="Classic"){
            setMixerType(mixer,flapsChannel1,VehicleConfig::MIXERTYPE_SERVO);
            setMixerType(mixer,flapsChannel2,VehicleConfig::MIXERTYPE_SERVO);

            setMixerVectorValue(mixer, flapsChannel1, VehicleConfig::MIXERVECTOR_FLAPS, 127);
            setMixerVectorValue(mixer, flapsChannel2, VehicleConfig::MIXERVECTOR_FLAPS, -127);
        } else if (m_aircraft->fwFlapsType->currentText()=="Crow flaps" && (aileronChannel1 > 0 && aileronChannel2 > 0)){
            setMixerType(mixer,flapsChannel1,VehicleConfig::MIXERTYPE_SERVO);
            setMixerType(mixer,flapsChannel2,VehicleConfig::MIXERTYPE_SERVO);

            setMixerVectorValue(mixer, flapsChannel1, VehicleConfig::MIXERVECTOR_FLAPS, 127);
            setMixerVectorValue(mixer, flapsChannel2, VehicleConfig::MIXERVECTOR_FLAPS, -127);

            setMixerVectorValue(mixer, aileronChannel1, VehicleConfig::MIXERVECTOR_FLAPS, 50);
            setMixerVectorValue(mixer, aileronChannel2, VehicleConfig::MIXERVECTOR_FLAPS, -50);

        } else if (m_aircraft->fwFlapsType->currentText()=="Flaperons" && (aileronChannel1 > 0 && aileronChannel2 > 0)){
            setMixerVectorValue(mixer, aileronChannel1, VehicleConfig::MIXERVECTOR_FLAPS, 50);
            setMixerVectorValue(mixer, aileronChannel2, VehicleConfig::MIXERVECTOR_FLAPS, -50);
        }
    }

    //spoilers
    int spoilersChannel1 = m_aircraft->fwSpoiler1ChannelBox->currentIndex()-1;
    int spoilersChannel2 = m_aircraft->fwSpoiler2ChannelBox->currentIndex()-1;
    if (spoilersChannel1 > -1 && m_aircraft->fwSpoilersType->currentText()!="None") {
        setMixerType(mixer,spoilersChannel1,VehicleConfig::MIXERTYPE_SERVO);
        setMixerType(mixer,spoilersChannel2,VehicleConfig::MIXERTYPE_SERVO);

        if (m_aircraft->fwSpoilersType->currentText()=="Spoilers"){
            setMixerVectorValue(mixer, spoilersChannel1, VehicleConfig::MIXERVECTOR_SPOILERS, 127);
            setMixerVectorValue(mixer, spoilersChannel2, VehicleConfig::MIXERVECTOR_SPOILERS, -127);
        } else if (m_aircraft->fwSpoilersType->currentText()=="Airbrakes"){
            setMixerVectorValue(mixer, spoilersChannel1, VehicleConfig::MIXERVECTOR_SPOILERS, 127);
            setMixerVectorValue(mixer, spoilersChannel2, VehicleConfig::MIXERVECTOR_SPOILERS, -127);
        } else if (m_aircraft->fwSpoilersType->currentText()=="Spoilerons"){
            setMixerVectorValue(mixer, spoilersChannel1, VehicleConfig::MIXERVECTOR_SPOILERS, 127);
            setMixerVectorValue(mixer, spoilersChannel2, VehicleConfig::MIXERVECTOR_SPOILERS, -127);

            setMixerVectorValue(mixer, spoilersChannel1, VehicleConfig::MIXERVECTOR_ROLL, 127);
            setMixerVectorValue(mixer, spoilersChannel2, VehicleConfig::MIXERVECTOR_ROLL, 127);
        }
    }
}

/**
 This function displays text and color formatting in order to help the user understand what channels have not yet been configured.
 */
bool ConfigFixedWingWidget::throwConfigError(QString airframeType)
{
	//Initialize configuration error flag
	bool error=false;
	
	//Create a red block. All combo boxes are the same size, so any one should do as a model
	int size = m_aircraft->fwEngineChannelBox->style()->pixelMetric(QStyle::PM_SmallIconSize);
	QPixmap pixmap(size,size);
	pixmap.fill(QColor("red"));
	
	if (airframeType == "FixedWing" ) {
		if (m_aircraft->fwEngineChannelBox->currentText() == "None"){
			m_aircraft->fwEngineChannelBox->setItemData(0, pixmap, Qt::DecorationRole);//Set color palettes 
			error=true;
		}
		else{
			m_aircraft->fwEngineChannelBox->setItemData(0, 0, Qt::DecorationRole);//Reset color palettes 
		}
		
		if (m_aircraft->fwElevator1ChannelBox->currentText() == "None"){
			m_aircraft->fwElevator1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole);//Set color palettes 
			error=true;
		}
		else{
			m_aircraft->fwElevator1ChannelBox->setItemData(0, 0, Qt::DecorationRole);//Reset color palettes 
		}	
	
		if	((m_aircraft->fwAileron1ChannelBox->currentText() == "None") &&	 (m_aircraft->fwRudder1ChannelBox->currentText() == "None")) {
			pixmap.fill(QColor("green"));
			m_aircraft->fwAileron1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole);//Set color palettes 
			m_aircraft->fwRudder1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole);//Set color palettes 
			error=true;
		}
		else{
			m_aircraft->fwAileron1ChannelBox->setItemData(0, 0, Qt::DecorationRole);//Reset color palettes 
			m_aircraft->fwRudder1ChannelBox->setItemData(0, 0, Qt::DecorationRole);//Reset color palettes 
		}	
	} else if (airframeType == "FixedWingElevon"){
		if (m_aircraft->fwEngineChannelBox->currentText() == "None"){
			m_aircraft->fwEngineChannelBox->setItemData(0, pixmap, Qt::DecorationRole);//Set color palettes 
			error=true;
		}
		else{
			m_aircraft->fwEngineChannelBox->setItemData(0, 0, Qt::DecorationRole);//Reset color palettes 
		}	
		
		if(m_aircraft->fwAileron1ChannelBox->currentText() == "None"){
			m_aircraft->fwAileron1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole);//Set color palettes 
			error=true;
		}
		else{
			m_aircraft->fwAileron1ChannelBox->setItemData(0, 0, Qt::DecorationRole);//Reset color palettes 
		}	
		
		if (m_aircraft->fwAileron2ChannelBox->currentText() == "None"){
			m_aircraft->fwAileron2ChannelBox->setItemData(0, pixmap, Qt::DecorationRole);//Set color palettes 
			error=true;
		}
		else{
			m_aircraft->fwAileron2ChannelBox->setItemData(0, 0, Qt::DecorationRole);//Reset color palettes 
		}	
			
	} else if ( airframeType == "FixedWingVtail"){
		if (m_aircraft->fwEngineChannelBox->currentText() == "None"){
			m_aircraft->fwEngineChannelBox->setItemData(0, pixmap, Qt::DecorationRole);//Set color palettes 
			error=true;
		}
		else{
			m_aircraft->fwEngineChannelBox->setItemData(0, 0, Qt::DecorationRole);//Reset color palettes 
		}	
		
		if(m_aircraft->fwElevator1ChannelBox->currentText() == "None"){
			m_aircraft->fwElevator1ChannelBox->setItemData(0, pixmap, Qt::DecorationRole);//Set color palettes 
			error=true;
		}
		else{
			m_aircraft->fwElevator1ChannelBox->setItemData(0, 0, Qt::DecorationRole);//Reset color palettes 
		}	
		
		if(m_aircraft->fwElevator2ChannelBox->currentText() == "None"){
			m_aircraft->fwElevator2ChannelBox->setItemData(0, pixmap, Qt::DecorationRole);//Set color palettes 
			error=true;
		}		
		else{
			m_aircraft->fwElevator2ChannelBox->setItemData(0, 0, Qt::DecorationRole);//Reset color palettes 
		}	
		
	}
	
	if (error){
		m_aircraft->fwStatusLabel->setText(QString("<font color='red'>ERROR: Assign all necessary channels</font>"));
	}

    return error;
}
