/**
 ******************************************************************************
 *
 * @file       emptygadget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup EmptyGadgetPlugin Empty Gadget Plugin
 * @{
 * @brief A place holder gadget plugin 
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
#include "dviewgadget.h"
#include "dviewwidget.h"

DViewGadget::DViewGadget(QString classId, DViewWidget *widget, QWidget *parent) :
        IUAVGadget(classId, parent),
        m_widget(widget)
{
}

DViewGadget::~DViewGadget()
{
    delete m_widget;
}

/*
  This is called when a configuration is loaded, and updates the plugin's settings.
  Careful: the plugin is already drawn before the loadConfiguration method is called the
  first time, so you have to be careful not to assume all the plugin values are initialized
  the first time you use them
 */
void DViewGadget::loadConfiguration(IUAVGadgetConfiguration* config)
{
    // Delete the (old)parser, this also disconnects all signals.

     delete parser;

     //qDebug() << "Load configuration";
     parser = new TelemetryParser();

     connect(parser, SIGNAL(systemstats(double)), m_widget,SLOT(setSystemStats(double)));
     connect(parser, SIGNAL(dop(double,double,double)), m_widget, SLOT(setDOP(double,double,double)));
     connect(parser, SIGNAL(linkstatus(QString,double,double)), m_widget, SLOT(setLinkStatus(QString,double,double)));
     connect(parser, SIGNAL(attitude(double,double,double)), m_widget,SLOT(setAttitude(double,double,double)));
     connect(parser, SIGNAL(airspeed(double)), m_widget,SLOT(setAirSpeed(double)));
     connect(parser, SIGNAL(altitude(double,double,double)), m_widget,SLOT(setAltitude(double,double,double)));
     connect(parser, SIGNAL(battery(double,double,double)), m_widget,SLOT(setBattery(double,double,double)));

     connect(parser, SIGNAL(satsused(int)), m_widget,SLOT(setSats(int)));
     connect(parser, SIGNAL(position(double,double,double)), m_widget,SLOT(setPosition(double,double,double)));
     connect(parser, SIGNAL(speedheading(double,double)), m_widget,SLOT(setSpeedHeading(double,double)));
     connect(parser, SIGNAL(datetime(double,double)), m_widget,SLOT(setDateTime(double,double)));

     connect(parser, SIGNAL(fixtype(QString)), m_widget, SLOT(setFixType(QString)));
     connect(parser, SIGNAL(dop(double,double,double)), m_widget, SLOT(setDOP(double,double,double)));
}
