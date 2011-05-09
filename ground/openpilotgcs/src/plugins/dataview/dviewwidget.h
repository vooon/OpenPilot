/**
 ******************************************************************************
 *
 * @file       emptygadgetwidget.h
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

#ifndef DVIEWWIDGET_H_
#define DVIEWWIDGET_H_

#include <QtGui/QLabel>
#include "ui_dviewwidget.h"

#include "uavobject.h"
#include <QGraphicsView>

class Ui_DViewWidget;

class DViewWidget : public QWidget, public Ui_DViewWidget
{
    Q_OBJECT

public:
    DViewWidget(QWidget *parent = 0);
   ~DViewWidget();

private slots:
   void setSystemStats(double ftime);
   void setDOP(double hdop, double vdop, double pdop);
   void setLinkStatus(QString s, double v1, double v2);
   void setAttitude(double roll, double yaw, double pitch);
   void setAirSpeed(double groundspeed);
   void setAltitude(double alt, double temp, double press);
   void setBattery(double v0, double v1, double v2);

   void setSats(int);
   void setPosition(double, double, double);
   void setDateTime(double, double);
   void setSpeedHeading(double, double);
   void dumpPacket(const QString &packet);
   void setFixType(const QString &fixtype);

private:
};

#endif /* DVIEWWIDGET_H_ */
