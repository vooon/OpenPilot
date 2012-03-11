/**
 ******************************************************************************
 *
 * @file       configcamerafblheliwidget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
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

#include "systemsettings.h"
#include "mixersettings.h"

#include "configfblheliwidget.h"
#include "ui_fblheli.h"

ConfigFBLHeliWidget::ConfigFBLHeliWidget(QWidget *parent) :
    ConfigTaskWidget(parent),
    ui(new Ui::ConfigFBLHeliWidget)
{
    ui->setupUi(this);

    // curve tab setup
    currentCurveBank = 0;
    initCurveUi();
    setupCurves( currentCurveBank );
    selectCurve( THROTTLE_CURVE );
}

ConfigFBLHeliWidget::~ConfigFBLHeliWidget()
{
    delete ui;
}

//--------------- Preset Tab BEGIN ---------------


//--------------- Preset Tab END -----------------




//--------------- Swash Tab BEGIN ---------------


//--------------- Swash Tab END -----------------



//--------------- Curves Tab BEGIN ---------------

void ConfigFBLHeliWidget::initCurveUi( void )
{
    ui->fblAilElvVal->hide();
    ui->fblThrottleVal->hide();
    ui->fblCollVal->hide();
    ui->fblTailVal->hide();

    // set table cell alignment
    for( int i = 0; i < ui->curveDetailTable->columnCount(); i++ )
        for( int j = 0; j < ui->curveDetailTable->rowCount(); j++ )
            if( ui->curveDetailTable->item( j, i ))
                ui->curveDetailTable->item( j, i )->setTextAlignment( Qt::AlignRight );
}

void ConfigFBLHeliWidget::setupCurves( int bank, int which )
{
    // setup string for bank transfer name generation
    QString objName;
    UAVObjectField *field;

    // retrieve mixer settings from uav objects
    UAVDataObject* obj = dynamic_cast<UAVDataObject*>( getObjectManager()->getObject( QString( "MixerSettings" )));
    Q_ASSERT(obj);

    // set throttle curve to widget
    if(( which & THROTTLE_CURVE_SEL ) != 0 ) {
        objName = QString( "fblThrottleCurveB%1" ).arg( bank );
        field = obj->getField( objName );
        Q_ASSERT(field);
        setupCurve( ui->fblThrottleCurve, field );

        objName = QString( "fblExpoB%1" ).arg( bank );
        field = obj->getField( objName );
        Q_ASSERT(field);
        ui->fblThrottleCurve->setExpo( field->getValue( 0 ).toInt( ));
        ui->fblThrottleExpo->setValue( field->getValue( 0 ).toInt( ));
    }

    // set aileron/elevator curve to widget
    if(( which & AILELV_CURVE_SEL ) != 0 ) {
        objName = QString( "fblAilElvCurveB%1" ).arg( bank );
        field = obj->getField( objName );
        Q_ASSERT(field);
        setupCurve( ui->fblAilElvCurve, field );
    }

    // set collective pitch curve to widget
    if(( which & COLLECTIVE_CURVE_SEL ) != 0 ) {
        objName = QString( "fblCollectiveCurveB%1" ).arg( bank );
        field = obj->getField( objName );
        Q_ASSERT(field);
        setupCurve( ui->fblCollCurve, field );
    }

    // set tail curve to widget
    if(( which & TAIL_CURVE_SEL ) != 0 ) {
        objName = QString( "fblTailCurveB%1" ).arg( bank );
        field = obj->getField( objName );
        Q_ASSERT(field);
        setupCurve( ui->fblTailCurve, field );
    }
}

void ConfigFBLHeliWidget::setupCurve( MixerCurveWidget *widget, UAVObjectField *data )
{
    QList<double> curveValues;
    // If the 1st element of the curve is <= -10, then the curve
    // is a straight line (that's how the mixer works on the mainboard):
    if( data->getValue( 0 ).toInt() <= -10 ) {
        widget->initLinearCurve( data->getNumElements(), (double)1 );
    } else {
        double temp=0;
        double value;
        for( unsigned int i=0; i < data->getNumElements(); i++ ) {
            value = data->getValue( i ).toDouble();
            temp += value;
            curveValues.append( value );
        }
        if( temp == 0 )
            widget->initLinearCurve( data->getNumElements(), (double)1 );
        else
            widget->initCurve( curveValues );
    }
}

void ConfigFBLHeliWidget::selectCurve( int curve )
{
    // select corresponding widget for data retrieval
    MixerCurveWidget* curCurve;

    switch( curve ) {
        case THROTTLE_CURVE:
            curCurve = ui->fblThrottleCurve;
            break;
        case AILELV_CURVE:
            curCurve = ui->fblAilElvCurve;
            break;
        case COLLECTIVE_CURVE:
            curCurve = ui->fblCollCurve;
            break;
        case TAIL_CURVE:
            curCurve = ui->fblTailCurve;
            break;
    }

    // set curve data to data area
    QList<double> curveValues = curCurve->getCurve();
    ui->fblNumIntPoints->setValue( curveValues.size());

    int cellVal = 0, minVal = 100, maxVal = -100;
    for( int i = 0; i < curveValues.size(); i++ ) {
        // calc string value
        cellVal = -100 + curveValues.at( i ) * 200;
        if( cellVal > maxVal )
            maxVal = cellVal;
        if( cellVal < minVal )
            minVal = cellVal;
        ui->curveDetailTable->item( i, 1 )->setData( Qt::DisplayRole, QVariant( cellVal ));
    }
    ui->fblMaximumPoint->setValue( maxVal );
    ui->fblMinimumPoint->setValue( minVal );
}

void ConfigFBLHeliWidget::on_fblTestSlider_valueChanged(int value)
{
    int response;

    // set value to label to show stick input value
    QString labelVal = QString( "Stick Input: %1%2%" ).arg( value >= 0 ? "+" : "-" ).arg( abs( value ), 3, 10, QChar( '0' ));
    ui->fblTestSliderVal->setText( labelVal );

    // check what curves are in test mode and propagate the value to them to show the line
    if( ui->fblTmThrottle->isChecked( )) {
        response = ui->fblThrottleCurve->showStickResponse( value );
        labelVal = QString( "Output Value: %1%2%" ).arg( value >= 0 ? "+" : "-" ).arg( abs( response ), 3, 10, QChar( '0' ));
        ui->fblThrottleVal->setText( labelVal );
    }
    if( ui->fblTmAilElv->isChecked( )) {
        response = ui->fblAilElvCurve->showStickResponse( value );
        labelVal = QString( "Output Value: %1%2%" ).arg( value >= 0 ? "+" : "-" ).arg( abs( response ), 3, 10, QChar( '0' ));
        ui->fblAilElvVal->setText( labelVal );
    }
    if( ui->fblTmColl->isChecked( )) {
        response = ui->fblCollCurve->showStickResponse( value );
        labelVal = QString( "Output Value: %1%2%" ).arg( value >= 0 ? "+" : "-" ).arg( abs( response ), 3, 10, QChar( '0' ));
        ui->fblCollVal->setText( labelVal );
    }
    if( ui->fblTmTail->isChecked( )) {
        response = ui->fblTailCurve->showStickResponse( value );
        labelVal = QString( "Output Value: %1%2%" ).arg( value >= 0 ? "+" : "-" ).arg( abs( response ), 3, 10, QChar( '0' ));
        ui->fblTailVal->setText( labelVal );
    }
}

void ConfigFBLHeliWidget::on_fblTmThrottle_toggled( bool checked )
{
    if( checked ) {
        ui->fblThrottleVal->show();
        on_fblTestSlider_valueChanged( ui->fblTestSlider->value());
    } else {
        ui->fblThrottleVal->hide();
        ui->fblThrottleCurve->endTestMode();
    }
}

void ConfigFBLHeliWidget::on_fblTmAilElv_toggled( bool checked )
{
    if( checked ) {
        ui->fblAilElvVal->show();
        on_fblTestSlider_valueChanged( ui->fblTestSlider->value());
    } else {
        ui->fblAilElvVal->hide();
        ui->fblAilElvCurve->endTestMode();
    }
}

void ConfigFBLHeliWidget::on_fblTmColl_toggled( bool checked )
{
    if( checked ) {
        ui->fblCollVal->show();
        on_fblTestSlider_valueChanged( ui->fblTestSlider->value());
    } else {
        ui->fblCollVal->hide();
        ui->fblCollCurve->endTestMode();
    }
}

void ConfigFBLHeliWidget::on_fblTmTail_toggled( bool checked )
{
    if( checked ) {
        ui->fblTailVal->show();
        on_fblTestSlider_valueChanged( ui->fblTestSlider->value());
    } else {
        ui->fblTailVal->hide();
        ui->fblTailCurve->endTestMode();
    }
}


void ConfigFBLHeliWidget::on_fblPtThrottle_toggled( bool checked )
{
    if( checked ) {
        ui->fblThrottleCurve->clearCurve();
        ui->fblThrottleCurve->setEnabled( false );
        ui->fblTmThrottle->setChecked( false );
        ui->fblTmThrottle->setEnabled( false );
    } else {
        setupCurves( currentCurveBank, THROTTLE_CURVE_SEL );
        ui->fblThrottleCurve->setEnabled( true );
        ui->fblTmThrottle->setEnabled( true );
    }
}

void ConfigFBLHeliWidget::on_fblPtAilElv_toggled( bool checked )
{
    if( checked ) {
        ui->fblAilElvCurve->clearCurve();
        ui->fblAilElvCurve->setEnabled( false );
        ui->fblTmAilElv->setChecked( false );
        ui->fblTmAilElv->setEnabled( false );
    } else {
        setupCurves( currentCurveBank, AILELV_CURVE_SEL );
        ui->fblAilElvCurve->setEnabled( true );
        ui->fblTmAilElv->setEnabled( true );
    }
}

void ConfigFBLHeliWidget::on_fblPtColl_toggled( bool checked )
{
    if( checked ) {
        ui->fblCollCurve->clearCurve();
        ui->fblCollCurve->setEnabled( false );
        ui->fblTmColl->setChecked( false );
        ui->fblTmColl->setEnabled( false );
    } else {
        setupCurves( currentCurveBank, COLLECTIVE_CURVE_SEL );
        ui->fblCollCurve->setEnabled( true );
        ui->fblTmColl->setEnabled( true );
    }
}

void ConfigFBLHeliWidget::on_fblPtTail_toggled( bool checked )
{
    if( checked ) {
        ui->fblTailCurve->clearCurve();
        ui->fblTailCurve->setEnabled( false );
        ui->fblTmTail->setChecked( false );
        ui->fblTmTail->setEnabled( false );
    } else {
        setupCurves( currentCurveBank, TAIL_CURVE_SEL );
        ui->fblTailCurve->setEnabled( true );
        ui->fblTmTail->setEnabled( true );
    }
}

void ConfigFBLHeliWidget::on_fblCurveSelector_activated( int index )
{
    selectCurve( index );
}


//--------------- Curves Tab END -----------------



//--------------- Expert Tab BEGIN ---------------


//--------------- Expert Tab END -----------------
