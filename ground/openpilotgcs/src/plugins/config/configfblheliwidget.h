/**
 ******************************************************************************
 *
 * @file       configcamerafblheliwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Flybarless Helicopter configuration panel
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

#ifndef CONFIGFBLHELIWIDGET_H
#define CONFIGFBLHELIWIDGET_H

#include <QWidget>
#include <QtDeclarative>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeComponent>
#include <QtDeclarative/QDeclarativeContext>


#include "stabilizationsettings.h"
#include "../uavobjectwidgetutils/configtaskwidget.h"

//--------------- Preset Tab BEGIN ---------------
//--------------- Preset Tab END -----------------

//--------------- Swash Tab BEGIN ---------------
//--------------- Swash Tab END -----------------

//--------------- Curves Tab BEGIN ---------------
static const int THROTTLE_CURVE = 0;
static const int AILELV_CURVE = 1;
static const int COLLECTIVE_CURVE = 2;
static const int TAIL_CURVE = 3;
static const int THROTTLE_CURVE_SEL = 1;
static const int AILELV_CURVE_SEL = 2;
static const int COLLECTIVE_CURVE_SEL = 4;
static const int TAIL_CURVE_SEL = 8;
//--------------- Curves Tab END -----------------

//--------------- Expert Tab BEGIN ---------------
//--------------- Expert Tab END -----------------


namespace Ui {
    class ConfigFBLHeliWidget;
}

class ConfigFBLHeliWidget : public ConfigTaskWidget
{
    Q_OBJECT

public:
    explicit ConfigFBLHeliWidget(QWidget *parent = 0);
    ~ConfigFBLHeliWidget();

    //--------------- Preset Tab BEGIN ---------------
    //--------------- Preset Tab END -----------------

    //--------------- Swash Tab BEGIN ---------------
    //--------------- Swash Tab END -----------------

    //--------------- Curves Tab BEGIN ---------------
    //--------------- Curves Tab END -----------------

    //--------------- Expert Tab BEGIN ---------------
    //--------------- Expert Tab END -----------------


private:
    Ui::ConfigFBLHeliWidget *ui;

    //--------------- Preset Tab BEGIN ---------------
    //--------------- Preset Tab END -----------------

    //--------------- Swash Tab BEGIN ---------------
    //--------------- Swash Tab END -----------------

    //--------------- Curves Tab BEGIN ---------------
    int currentCurveBank;
    void setupCurve( MixerCurveWidget *widget, UAVObjectField *data );
    void selectCurve( int curve );
    //--------------- Curves Tab END -----------------

    //--------------- Expert Tab BEGIN ---------------
    StabilizationSettings* stabilizationSettings;
    QDeclarativeEngine* engine;
    QDeclarativeComponent* component;
    QObject *scriptInstance;
    void setupExpert();

    //--------------- Expert Tab END -----------------


protected:

    //--------------- Preset Tab BEGIN ---------------
    //--------------- Preset Tab END -----------------

    //--------------- Swash Tab BEGIN ---------------
    //--------------- Swash Tab END -----------------

    //--------------- Curves Tab BEGIN ---------------
    void setupCurves( int bank, int which = 15 );
    void initCurveUi( void );
    //--------------- Curves Tab END -----------------

    //--------------- Expert Tab BEGIN ---------------
    //--------------- Expert Tab END -----------------


private slots:

    //--------------- Preset Tab BEGIN ---------------
    //--------------- Preset Tab END -----------------

    //--------------- Swash Tab BEGIN ---------------
    //--------------- Swash Tab END -----------------

    //--------------- Curves Tab BEGIN ---------------
    void on_fblTestSlider_valueChanged( int value );
    void on_fblTmThrottle_toggled( bool checked );
    void on_fblTmAilElv_toggled( bool checked );
    void on_fblTmColl_toggled( bool checked );
    void on_fblTmTail_toggled( bool checked );
    void on_fblPtThrottle_toggled( bool checked );
    void on_fblPtAilElv_toggled( bool checked );
    void on_fblPtColl_toggled( bool checked );
    void on_fblPtTail_toggled( bool checked );
    void on_fblCurveSelector_activated( int index );
    //--------------- Curves Tab END -----------------

    //--------------- Expert Tab BEGIN ---------------
    void evaluateScript(int value);
    void updateScript();
    void on_btnEvaluateScript_clicked();
    //--------------- Expert Tab END -----------------


public slots:

    //--------------- Preset Tab BEGIN ---------------
    //--------------- Preset Tab END -----------------

    //--------------- Swash Tab BEGIN ---------------
    //--------------- Swash Tab END -----------------

    //--------------- Curves Tab BEGIN ---------------
    //--------------- Curves Tab END -----------------

    //--------------- Expert Tab BEGIN ---------------
    //--------------- Expert Tab END -----------------

};

#endif // CONFIGFBLHELIWIDGET_H
