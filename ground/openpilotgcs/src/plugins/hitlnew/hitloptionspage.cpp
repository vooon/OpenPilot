/**
 ******************************************************************************
 *
 * @file       hitloptionspage.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup HITLPlugin HITL Plugin
 * @{
 * @brief The Hardware In The Loop plugin 
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

#include "hitloptionspage.h"
#include "hitlconfiguration.h"
#include "ui_hitloptionspage.h"
#include <hitlplugin.h>

#include <QFileDialog>
#include <QtAlgorithms>
#include <QStringList>
#include <simulator.h>

HITLOptionsPage::HITLOptionsPage(HITLConfiguration *conf, QObject *parent) :
    IOptionsPage(parent),
    config(conf)
{

}

QWidget *HITLOptionsPage::createPage(QWidget *parent)
{
    // Create page
    m_optionsPage = new Ui::HITLOptionsPage();
    QWidget* optionsPageWidget = new QWidget;
    m_optionsPage->setupUi(optionsPageWidget);
    int index = 0;
    foreach (SimulatorCreator* creator, HITLPlugin::typeSimulators) {
        m_optionsPage->chooseFlightSimulator->insertItem(index++, creator->Description(),creator->ClassId());
    }

    m_optionsPage->executablePath->setExpectedKind(Utils::PathChooser::File);
    m_optionsPage->executablePath->setPromptDialogTitle(tr("Choose flight simulator executable"));
    m_optionsPage->dataPath->setExpectedKind(Utils::PathChooser::Directory);
    m_optionsPage->dataPath->setPromptDialogTitle(tr("Choose flight simulator data directory"));

    // Restore the contents from the settings:
    foreach (SimulatorCreator* creator, HITLPlugin::typeSimulators) {
        QString id = config->Settings().simulatorId;
        if(creator->ClassId() == id)
            m_optionsPage->chooseFlightSimulator->setCurrentIndex(HITLPlugin::typeSimulators.indexOf(creator));
    }

    m_optionsPage->hostAddress->setText(config->Settings().hostAddress);
    m_optionsPage->remoteHostAddress->setText(config->Settings().remoteHostAddress);
    m_optionsPage->inputPort->setText(QString::number(config->Settings().inPort));
    m_optionsPage->outputPort->setText(QString::number(config->Settings().outPort));
    if (config->Settings().isUdp != FALSE) {
        m_optionsPage->protocolUdp->setChecked(1);
    } else {
        m_optionsPage->protocolTcp->setChecked(1);
    }

    m_optionsPage->manualControl->setChecked(config->Settings().manual);
    m_optionsPage->inputGroupBox->setDisabled(config->Settings().manual);
    {
        m_optionsPage->AttitudeActualSpin->setEnabled(config->Settings().isAttAct);
        m_optionsPage->AttitudeActualCheck->setChecked(config->Settings().isAttAct);
        m_optionsPage->AttitudeActualSpin->setValue(config->Settings().attActRate);

        m_optionsPage->AttitudeRawSpin->setEnabled(config->Settings().isAttRaw);
        m_optionsPage->AttitudeRawCheck->setChecked(config->Settings().isAttRaw);
        m_optionsPage->AttitudeRawSpin->setValue(config->Settings().attRawRate);

        m_optionsPage->BaroAltitudeSpin->setEnabled(config->Settings().isBaroAlt);
        m_optionsPage->BaroAltitudeCheck->setChecked(config->Settings().isBaroAlt);
        m_optionsPage->BaroAltitudeSpin->setValue(config->Settings().baroAltRate);

        m_optionsPage->GPSPositionSpin->setEnabled(config->Settings().isGPSPos);
        m_optionsPage->GPSPositionCheck->setChecked(config->Settings().isGPSPos);
        m_optionsPage->GPSPositionSpin->setValue(config->Settings().GPSPosRate);

        m_optionsPage->PositionActualSpin->setEnabled(config->Settings().isPosAct);
        m_optionsPage->PositionActualCheck->setChecked(config->Settings().isPosAct);
        m_optionsPage->PositionActualSpin->setValue(config->Settings().posActRate);

        m_optionsPage->VelocityActualSpin->setEnabled(config->Settings().isVelAct);
        m_optionsPage->VelocityActualCheck->setChecked(config->Settings().isVelAct);
        m_optionsPage->VelocityActualSpin->setValue(config->Settings().velActRate);

        m_optionsPage->HomeLocationSpin->setEnabled(config->Settings().isHomeLoc);
        m_optionsPage->HomeLocationCheck->setChecked(config->Settings().isHomeLoc);
        m_optionsPage->HomeLocationSpin->setValue(config->Settings().homeLocRate);
    }
    m_optionsPage->ActuatorDesiredSpin->setEnabled(config->Settings().isActDes);
    m_optionsPage->ActuatorDesiredCheck->setChecked(config->Settings().isActDes);
    m_optionsPage->ActuatorDesiredSpin->setValue(config->Settings().actDesRate);

    m_optionsPage->latitude->setValue(config->Settings().latitude);
    m_optionsPage->longitude->setValue(config->Settings().longitude);

    m_optionsPage->startSim->setChecked(config->Settings().startSim);
    m_optionsPage->startsimGroupBox->setEnabled(config->Settings().startSim);
    {
        m_optionsPage->executablePath->setPath(config->Settings().binPath);
        m_optionsPage->dataPath->setPath(config->Settings().dataPath);
    }

    return optionsPageWidget;
}

void HITLOptionsPage::apply()
{
    SimulatorSettings settings;
    int i = m_optionsPage->chooseFlightSimulator->currentIndex();
    settings.simulatorId = m_optionsPage->chooseFlightSimulator->itemData(i).toString();

    settings.hostAddress = m_optionsPage->hostAddress->text();
    settings.remoteHostAddress = m_optionsPage->remoteHostAddress->text();
    settings.inPort = m_optionsPage->inputPort->text().toInt();
    settings.outPort = m_optionsPage->outputPort->text().toInt();
    settings.isUdp = m_optionsPage->protocolUdp->isChecked();

    settings.manual = m_optionsPage->manualControl->isChecked();
    {
        settings.isAttAct = m_optionsPage->AttitudeActualCheck->isChecked();
        settings.attActRate = m_optionsPage->AttitudeActualSpin->value();
        settings.isAttRaw = m_optionsPage->AttitudeRawCheck->isChecked();
        settings.attRawRate = m_optionsPage->AttitudeRawSpin->value();
        settings.isBaroAlt = m_optionsPage->BaroAltitudeCheck->isChecked();
        settings.baroAltRate = m_optionsPage->BaroAltitudeSpin->value();
        settings.isGPSPos = m_optionsPage->GPSPositionCheck->isChecked();
        settings.GPSPosRate = m_optionsPage->GPSPositionSpin->value();
        settings.isPosAct = m_optionsPage->PositionActualCheck->isChecked();
        settings.posActRate = m_optionsPage->PositionActualSpin->value();
        settings.isVelAct = m_optionsPage->VelocityActualCheck->isChecked();
        settings.velActRate = m_optionsPage->VelocityActualSpin->value();
        settings.isHomeLoc = m_optionsPage->HomeLocationCheck->isChecked();
        settings.homeLocRate = m_optionsPage->HomeLocationSpin->value();
    }
    settings.isActDes = m_optionsPage->ActuatorDesiredCheck->isChecked();
    settings.actDesRate = m_optionsPage->ActuatorDesiredSpin->value();

    settings.longitude = m_optionsPage->longitude->value();
    settings.latitude = m_optionsPage->latitude->value();

    settings.startSim = m_optionsPage->startSim->isChecked();
    {
        settings.binPath = m_optionsPage->executablePath->path();
        settings.dataPath = m_optionsPage->dataPath->path();
    }

    config->setSimulatorSettings(settings);
}

void HITLOptionsPage::finish()
{
    delete m_optionsPage;
}
