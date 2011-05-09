/**
 ******************************************************************************
 *
 * @file       emptygadgetwidget.cpp
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
#include "dviewwidget.h"
#include <math.h>
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"

#include <iostream>

#include <QDebug>
#include <QStringList>
#include <QtGui/QWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QTableWidget>


DViewWidget::DViewWidget(QWidget *parent) : QWidget(parent)
{

    setupUi(this);
    //qDebug() << "initialize the widget";

}

DViewWidget::~DViewWidget()
{
   // Do nothing
}

void DViewWidget::setSystemStats(double ftime)
{
    QString str;
    ftime_value->setText(str.sprintf("%.0f s",(ftime/1000)));
    //qDebug() << "                            setSystemStats";
}

void DViewWidget::setDOP(double hdop, double vdop, double pdop)
{
    QString str;
    str.sprintf("%.2f / %.2f / %.2f", hdop, vdop, pdop);
    dop_value->setText(str);
    //qDebug() << "                             setDOP";
}

void DViewWidget::setLinkStatus(QString s, double v1, double v2)
{
    QString str; 
    s_value->setText(s);
    v1_value_2->setText(str.sprintf("%.0f Tx",v1));
    v2_value_2->setText(str.sprintf("%.0f Rx",v2));
     //       qDebug() << "                         setLinkStatus";
}

void DViewWidget::setAttitude(double roll, double yaw, double pitch)
{
    QString str;
    roll_value->setText(str.sprintf("roll %.0f deg",roll));
    yaw_value->setText(str.sprintf("yaw %.0f deg",yaw));
    pitch_value->setText(str.sprintf("pitch %.0f deg",pitch));
     //       qDebug() << "                         setAttitude";
}

void DViewWidget::setAirSpeed(double groundspeed)
{
    QString str;
    groundspeed_value->setText(str.sprintf("%.02f m/s",groundspeed));
    //        qDebug() << "                         setAirSpeed";
}

void DViewWidget::setAltitude(double alt, double temp, double press)
{
    QString str;
    alt_value->setText(str.sprintf("%.0f m",alt));
    temp_value->setText(str.sprintf("%.0f degC",temp));
    press_value->setText(str.sprintf("%.0f KPa",press));
     //       qDebug() << "                         setBaroAltitude";
}

void DViewWidget::setBattery(double v0, double v1, double v2)
{
    QString str;
    v0_value->setText(str.sprintf("%.2f V",v0));
    v1_value->setText(str.sprintf("%.2f A",v1));
    v2_value->setText(str.sprintf("%.0f mAh",v2));
    //        qDebug() << "                         setBattery";
}

void DViewWidget::setSpeedHeading(double speed, double heading)
{
    QString str;
    speed_value->setText(str.sprintf("%.02f m/s",speed));
    bear_value->setText(str.sprintf("%.02f deg",heading));
    //            qDebug() << "                         setSpeedHeading";
}

void DViewWidget::setDateTime(double date, double time)
{
    QString dstring1,dstring2;
    dstring1.sprintf("%06.0f",date);
    dstring1.insert(dstring1.length()-2,".");
    dstring1.insert(dstring1.length()-5,".");
    dstring2.sprintf("%06.0f",time);
    dstring2.insert(dstring2.length()-2,":");
    dstring2.insert(dstring2.length()-5,":");
    time_value->setText(dstring1 + "    " + dstring2 + " GMT");
     //           qDebug() << "                         setDateTime";
}

void DViewWidget::setFixType(const QString &fixtype)
{
    if(fixtype =="NoGPS") {
        fix_value->setText("No GPS");
    } else if (fixtype == "NoFix") {
        fix_value->setText("Fix not available");
    } else if (fixtype == "Fix2D") {
        fix_value->setText("2D");
    } else if (fixtype =="Fix3D") {
        fix_value->setText("3D");
    } else {
        fix_value->setText("Unknown");
    }
    //            qDebug() << "                         setFixType";
}


void DViewWidget::setSats(int sats)
{
    QString str;
    sats_value->setText(str.sprintf("%.1i",sats));
    //            qDebug() << "                         setSats";
}

void DViewWidget::setPosition(double lat, double lon, double alt)
{
    //lat *= 1E-7;
    //lon *= 1E-7;
    double deg = floor(fabs(lat));
    double min = (fabs(lat)-deg)*60;
    QString str1;
    str1.sprintf("%.0f%c%.3f' ", deg,0x00b0, min);
    if (lat>0)
        str1.append("N");
    else
        str1.append("S");
    coord_value->setText(str1);
    deg = floor(fabs(lon));
    min = (fabs(lon)-deg)*60;
    QString str2;
    str2.sprintf("%.0f%c%.3f' ", deg,0x00b0, min);
    if (lon>0)
        str2.append("E");
    else
        str2.append("W");
    coord_value_2->setText(str2);
    QString str3;
    str3.sprintf("%.2f m", alt);
    coord_value_3->setText(str3);
     //           qDebug() << "                         setPosition";
}


