/**
 ******************************************************************************
 *
 * @file       configgadgetwidget.cpp
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

#include "configahrswidget.h"
#include "configgadgetwidget.h"

#include "configairframewidget.h"
#include "configccattitudewidget.h"
#include "configinputwidget.h"
#include "configoutputwidget.h"
#include "configstabilizationwidget.h"
#include "configcamerastabilizationwidget.h"
#include "config_pro_hw_widget.h"
#include "config_cc_hw_widget.h"
#include "defaultattitudewidget.h"
#include "uavobjectutilmanager.h"

#include <QDebug>
#include <QStringList>
#include <QtGui/QWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>



ConfigGadgetWidget::ConfigGadgetWidget(QWidget *parent) :
        QWidget(parent),
        ftw(new MyTabbedStackWidget(this, this, true))
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    ftw->setIconSize(64);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(ftw);
    setLayout(layout);

    setBoard(DefaultHwSettingsWidget::UnknownBoard);
    //TODO: remove DefaultAttitudeWidget

    // *********************
/*    QWidget *qwd;

    qwd = new DefaultHwSettingsWidget(this);
    ftw->insertTab(ConfigGadgetWidget::hardware, qwd, QIcon(":/configgadget/images/hw_config.png"), QString("HW Settings"));

    qwd = new ConfigAirframeWidget(this);
    ftw->insertTab(ConfigGadgetWidget::aircraft, qwd, QIcon(":/configgadget/images/Airframe.png"), QString("Aircraft"));

    qwd = new ConfigInputWidget(this);
    ftw->insertTab(ConfigGadgetWidget::input, qwd, QIcon(":/configgadget/images/Transmitter.png"), QString("Input"));

    qwd = new ConfigOutputWidget(this);
    ftw->insertTab(ConfigGadgetWidget::output, qwd, QIcon(":/configgadget/images/Servo.png"), QString("Output"));

    qwd = new DefaultAttitudeWidget(this);
    ftw->insertTab(ConfigGadgetWidget::ins, qwd, QIcon(":/configgadget/images/AHRS-v1.3.png"), QString("INS"));

    qwd = new ConfigStabilizationWidget(this);
    ftw->insertTab(ConfigGadgetWidget::stabilization, qwd, QIcon(":/configgadget/images/gyroscope.png"), QString("Stabilization"));

    qwd = new ConfigCameraStabilizationWidget(this);
    ftw->insertTab(ConfigGadgetWidget::camerastabilization, qwd, QIcon(":/configgadget/images/camera.png"), QString("Camera Stab"));
*/

//    qwd = new ConfigPipXtremeWidget(this);
//    ftw->insertTab(5, qwd, QIcon(":/configgadget/images/PipXtreme.png"), QString("PipXtreme"));

    // *********************
    // Listen to autopilot connection events

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    TelemetryManager* telMngr = pm->getObject<TelemetryManager>();
    connect(telMngr, SIGNAL(connected()), this, SLOT(onAutopilotConnect()));
    connect(telMngr, SIGNAL(disconnected()), this, SLOT(onAutopilotDisconnect()));

    // And check whether by any chance we are not already connected
    if (telMngr->isConnected())
        onAutopilotConnect();    

    connect(ftw,SIGNAL(currentAboutToShow(int,bool*)),this,SLOT(tabAboutToChange(int,bool*)));//,Qt::BlockingQueuedConnection);

}

ConfigGadgetWidget::~ConfigGadgetWidget()
{
   // Do nothing

    // TODO: properly delete all the tabs in ftw before exiting
}

void ConfigGadgetWidget::resizeEvent(QResizeEvent *event)
{

    QWidget::resizeEvent(event);
}

void ConfigGadgetWidget::onAutopilotDisconnect() {
    setBoard(DefaultHwSettingsWidget::UnknownBoard);
    emit autopilotDisconnected();
}

void ConfigGadgetWidget::onAutopilotConnect() {

    qDebug()<<"ConfigGadgetWidget onAutopilotConnect";
    // First of all, check what Board type we are talking to, and
    // if necessary, remove/add tabs in the config gadget:
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectUtilManager* utilMngr = pm->getObject<UAVObjectUtilManager>();
    if (utilMngr) {
        int board = utilMngr->getBoardModel();
        qDebug() << "Board model: " << board;
        if ((board & 0xff00) == 1024) {
            // CopterControl family
            setBoard(DefaultHwSettingsWidget::CopterControlBoard);
        } else if ((board & 0xff00) == 256 ) {
            // Mainboard family
            setBoard(DefaultHwSettingsWidget::RevolutionBoard);
        }
    }
    emit autopilotConnected();
}

void ConfigGadgetWidget::addWidgetTab(const widgetTabs tab, QWidget *widget, const QIcon &icon, const QString &label)
{
    if (!widget) return;
    if (tab2index.find(tab) != tab2index.end() ) // widget tab already exists
        return;

    int index = ftw->insertTab(tab, widget, icon, label);
    QMap<widgetTabs, int>::iterator tab_iter = tab2index.insert(tab, index);

    // increment indicies of following entries
    ++tab_iter;
    while (tab_iter != tab2index.end())
    {
        tab_iter.value()++;
        ++tab_iter;
    }
}

void ConfigGadgetWidget::removeWidgetTab(widgetTabs tab)
{
    QMap<widgetTabs, int>::iterator tab_iter = tab2index.find(tab);
    if (tab2index.end() == tab_iter)
        // no entry for tab
        return;

    ftw->removeTab(tab_iter.value());
    tab_iter = tab2index.erase(tab_iter);

    // decrement indices of following entries
    while (tab_iter != tab2index.end())
    {
        tab_iter.value()--;
        ++tab_iter;
    }
}

void ConfigGadgetWidget::setBoard(const DefaultHwSettingsWidget::BoardType type)
{
    //TODO check if board type has changed
    QWidget *qwd;

    ftw->setCurrentIndex(ConfigGadgetWidget::hardware);

    switch(type)
    {
    case DefaultHwSettingsWidget::UnknownBoard:
        // remove unnecessary widgets
        removeWidgetTab(aircraft);
        removeWidgetTab(input);
        removeWidgetTab(output);
        removeWidgetTab(ins);
        removeWidgetTab(stabilization);
        removeWidgetTab(camerastabilization);
        if (!checkWidgetTab<DefaultHwSettingsWidget>(hardware))
        { // add default hardware widget
            qwd = new DefaultHwSettingsWidget(this);
            addWidgetTab(hardware, qwd, QIcon(":/configgadget/images/hw_config.png"), QString("HW Settings"));
            connect(qwd, SIGNAL(boardChanged(DefaultHwSettingsWidget::BoardType)),
                    this, SLOT(setBoard(DefaultHwSettingsWidget::BoardType)));
        }
        return;
    case DefaultHwSettingsWidget::CopterControlBoard:
    {
        // hardware widget
        if (!checkWidgetTab<ConfigCCHWWidget>(hardware))
        {
            qwd = new ConfigCCHWWidget(this);
            addWidgetTab(hardware, qwd, QIcon(":/configgadget/images/hw_config.png"), QString("HW Settings"));
        }
        // input widget
        if (checkWidgetTab<ConfigInputWidget>(input))
        {
            //TODO: For input CC should not allow the options for flight mode switch like position hold.
        }
        // output widget
        ConfigOutputWidget *outputWidget;
        if ( (outputWidget = checkWidgetTab<ConfigOutputWidget>(output)) )
        { // output widget already exist -> configure it
            outputWidget->channels(6);
        }
        else
        { // output widget doesn't exist
            qwd = new ConfigOutputWidget(6, this);
            addWidgetTab(output, qwd, QIcon(":/configgadget/images/Servo.png"), QString("Output"));
        }
        // INS widget
        if (!checkWidgetTab<ConfigCCAttitudeWidget>(ins))
        {
            qwd = new ConfigCCAttitudeWidget(this);
            addWidgetTab(ins, qwd, QIcon(":/configgadget/images/AHRS-v1.3.png"), QString("Attitude"));
        }
        break;
    }
    case DefaultHwSettingsWidget::RevolutionBoard:
        //TODO: Revo has 8 or 16 outputs
        // hardware widget
        if (!checkWidgetTab<ConfigProHWWidget>(hardware))
        {
            qwd = new ConfigProHWWidget(this);
            addWidgetTab(hardware, qwd, QIcon(":/configgadget/images/hw_config.png"), QString("HW Settings"));
        }
        // INS widget
        if (!checkWidgetTab<ConfigAHRSWidget>(ins))
        {
            QWidget *qwd = new ConfigAHRSWidget(this);
            addWidgetTab(ins, qwd, QIcon(":/configgadget/images/AHRS-v1.3.png"), QString("INS"));
        }
        break;
    default:
        break;
    }

    // add board unspecific widgets
    qwd = new ConfigAirframeWidget(this);
    addWidgetTab(aircraft, qwd, QIcon(":/configgadget/images/Airframe.png"), QString("Aircraft"));

    // FIXME: input is board specific
    qwd = new ConfigInputWidget(this);
    addWidgetTab(input, qwd, QIcon(":/configgadget/images/Transmitter.png"), QString("Input"));

    qwd = new ConfigStabilizationWidget(this);
    addWidgetTab(stabilization, qwd, QIcon(":/configgadget/images/gyroscope.png"), QString("Stabilization"));

    qwd = new ConfigCameraStabilizationWidget(this);
    addWidgetTab(camerastabilization, qwd, QIcon(":/configgadget/images/camera.png"), QString("Camera Stab"));

    ftw->setCurrentIndex(ConfigGadgetWidget::hardware);
}

//FIXME: use reference instead of pointer
void ConfigGadgetWidget::tabAboutToChange(int i,bool * proceed)
{
    Q_UNUSED(i);
    *proceed=true;
    ConfigTaskWidget * wid=qobject_cast<ConfigTaskWidget *>(ftw->currentWidget());
    if(!wid)
        return;
    if(wid->isDirty())
    {
        int ans=QMessageBox::warning(this,tr("Unsaved changes"),tr("The tab you are leaving has unsaved changes,"
                                                           "if you proceed they will be lost."
                                                           "Do you still want to proceed?"),QMessageBox::Yes,QMessageBox::No);
        if(ans==QMessageBox::No)
            *proceed=false;
        else
            wid->setDirty(false);
    }
}



