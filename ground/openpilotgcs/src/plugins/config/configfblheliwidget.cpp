/**
 ******************************************************************************
 *
 * @file       configfblheliwidget.cpp
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

#include "systemsettings.h"
#include "mixersettings.h"

#include "configairframewidget.h"
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

    // set delegate for editable row
    SpinBoxDelegate *sbd = new SpinBoxDelegate();
    sbd->setMinVal( -100 );
    sbd->setMaxVal( 100 );
    ui->curveDetailTable->setItemDelegateForColumn( 1, sbd );

    // set table cell alignment, set first column read only
    for( int i = 0; i < ui->curveDetailTable->columnCount(); i++ )
        for( int j = 0; j < ui->curveDetailTable->rowCount(); j++ )
            if( ui->curveDetailTable->item( j, i )) {
                if( i == 0 )
                    ui->curveDetailTable->item( j, i )->setFlags( ui->curveDetailTable->item( j, i )->flags() & ~Qt::ItemIsEditable );
                ui->curveDetailTable->item( j, i )->setTextAlignment( Qt::AlignRight );
            }

    // setup slot assignments for connection curve widgets -> table values
    connect( ui->fblThrottleCurve, SIGNAL( curveUpdated( QList<double>, double )), this, SLOT( updateThrottleCurve( QList<double>, double )));
    connect( ui->fblAilElvCurve, SIGNAL( curveUpdated( QList<double>, double )), this, SLOT( updateAilElvCurve( QList<double>, double )));
    connect( ui->fblCollCurve, SIGNAL( curveUpdated( QList<double>, double )), this, SLOT( updateCollCurve( QList<double>, double )));
    connect( ui->fblTailCurve, SIGNAL( curveUpdated( QList<double>, double )), this, SLOT( updateTailCurve( QList<double>, double )));
}

void ConfigFBLHeliWidget::setupCurves( int bank, int which )
{
    setupPhase = true;
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

        objName = QString( "fblExpoB%1" ).arg( bank );
        field = obj->getField( objName );
        Q_ASSERT(field);
        ui->fblAilElvCurve->setExpo( field->getValue( 1 ).toInt( ));
        ui->fblAilElvExpo->setValue( field->getValue( 1 ).toInt( ));
    }

    // set collective pitch curve to widget
    if(( which & COLLECTIVE_CURVE_SEL ) != 0 ) {
        objName = QString( "fblCollectiveCurveB%1" ).arg( bank );
        field = obj->getField( objName );
        Q_ASSERT(field);
        setupCurve( ui->fblCollCurve, field );

        objName = QString( "fblExpoB%1" ).arg( bank );
        field = obj->getField( objName );
        Q_ASSERT(field);
        ui->fblCollCurve->setExpo( field->getValue( 2 ).toInt( ));
        ui->fblCollExpo->setValue( field->getValue( 2 ).toInt( ));
    }

    // set tail curve to widget
    if(( which & TAIL_CURVE_SEL ) != 0 ) {
        objName = QString( "fblTailCurveB%1" ).arg( bank );
        field = obj->getField( objName );
        Q_ASSERT(field);
        setupCurve( ui->fblTailCurve, field );

        objName = QString( "fblExpoB%1" ).arg( bank );
        field = obj->getField( objName );
        Q_ASSERT(field);
        ui->fblTailCurve->setExpo( field->getValue( 3 ).toInt( ));
        ui->fblTailExpo->setValue( field->getValue( 3 ).toInt( ));
    }
    setupPhase = false;

    currentCurveBank = bank;
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

    updateCurveTable( curveValues );
}

void ConfigFBLHeliWidget::updateCurveTable( QList<double> &values )
{
    if( values.size() < 2 )
        return;

    int cellVal = 0, minVal = 100, maxVal = -100;

    // set all fields to "-"
    for( int col = 0; col < 2; col++ )
        for( int row = 0; row < 10; row++ )
            ui->curveDetailTable->item( row, col )->setData(  Qt::DisplayRole, QVariant( "-" ));

    for( int i = 0; i < values.size(); i++ ) {
        // calc string value
        cellVal = -100 + values.at( i ) * 200;
        if( cellVal > maxVal )
            maxVal = cellVal;
        if( cellVal < minVal )
            minVal = cellVal;
        ui->curveDetailTable->item( i, 1 )->setData( Qt::DisplayRole, QVariant( cellVal ));
        QString posVal = QString( "%1%" ).arg( -100 + ( i * 200 / ( values.size() - 1 )));
        ui->curveDetailTable->item( i, 0 )->setData( Qt::DisplayRole, QVariant( posVal ));
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
        labelVal = QString( "Output: %1%2%" ).arg( response >= 0 ? "+" : "-" ).arg( abs( response ), 3, 10, QChar( '0' ));
        ui->fblThrottleVal->setText( labelVal );
    }
    if( ui->fblTmAilElv->isChecked( )) {
        response = ui->fblAilElvCurve->showStickResponse( value );
        labelVal = QString( "Output: %1%2%" ).arg( response >= 0 ? "+" : "-" ).arg( abs( response ), 3, 10, QChar( '0' ));
        ui->fblAilElvVal->setText( labelVal );
    }
    if( ui->fblTmColl->isChecked( )) {
        response = ui->fblCollCurve->showStickResponse( value );
        labelVal = QString( "Output: %1%2%" ).arg( response >= 0 ? "+" : "-" ).arg( abs( response ), 3, 10, QChar( '0' ));
        ui->fblCollVal->setText( labelVal );
    }
    if( ui->fblTmTail->isChecked( )) {
        response = ui->fblTailCurve->showStickResponse( value );
        labelVal = QString( "Output: %1%2%" ).arg( response >= 0 ? "+" : "-" ).arg( abs( response ), 3, 10, QChar( '0' ));
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
        ui->fblThrottleCurve->showDisabledBg( true );
        ui->fblThrottleCurve->setEnabled( false );
        ui->fblThrottleExpo->setEnabled( false );
        ui->fblThrottleExpoLabel->setEnabled( false );
        ui->fblTmThrottle->setChecked( false );
        ui->fblTmThrottle->setEnabled( false );
    } else {
        setupCurves( currentCurveBank, THROTTLE_CURVE_SEL );
        ui->fblThrottleCurve->setEnabled( true );
        ui->fblTmThrottle->setEnabled( true );
        ui->fblThrottleExpo->setEnabled( true );
        ui->fblThrottleExpoLabel->setEnabled( true );
        ui->fblThrottleCurve->showDisabledBg( false );
    }
}

void ConfigFBLHeliWidget::on_fblPtAilElv_toggled( bool checked )
{
    if( checked ) {
        ui->fblAilElvCurve->clearCurve();
        ui->fblAilElvCurve->showDisabledBg( true );
        ui->fblAilElvCurve->setEnabled( false );
        ui->fblAilElvExpo->setEnabled( false );
        ui->fblAilElvExpoLabel->setEnabled( false );
        ui->fblTmAilElv->setChecked( false );
        ui->fblTmAilElv->setEnabled( false );
    } else {
        setupCurves( currentCurveBank, AILELV_CURVE_SEL );
        ui->fblAilElvCurve->setEnabled( true );
        ui->fblTmAilElv->setEnabled( true );
        ui->fblAilElvExpo->setEnabled( true );
        ui->fblAilElvExpoLabel->setEnabled( true );
        ui->fblAilElvCurve->showDisabledBg( false );
    }
}

void ConfigFBLHeliWidget::on_fblPtColl_toggled( bool checked )
{
    if( checked ) {
        ui->fblCollCurve->clearCurve();
        ui->fblCollCurve->showDisabledBg( true );
        ui->fblCollCurve->setEnabled( false );
        ui->fblCollExpo->setEnabled( false );
        ui->fblCollExpoLabel->setEnabled( false );
        ui->fblTmColl->setChecked( false );
        ui->fblTmColl->setEnabled( false );
    } else {
        setupCurves( currentCurveBank, COLLECTIVE_CURVE_SEL );
        ui->fblCollCurve->setEnabled( true );
        ui->fblTmColl->setEnabled( true );
        ui->fblCollExpo->setEnabled( true );
        ui->fblCollExpoLabel->setEnabled( true );
        ui->fblCollCurve->showDisabledBg( false );
    }
}

void ConfigFBLHeliWidget::on_fblPtTail_toggled( bool checked )
{
    if( checked ) {
        ui->fblTailCurve->clearCurve();
        ui->fblTailCurve->showDisabledBg( true );
        ui->fblTailCurve->setEnabled( false );
        ui->fblTailExpo->setEnabled( false );
        ui->fblTailExpoLabel->setEnabled( false );
        ui->fblTmTail->setChecked( false );
        ui->fblTmTail->setEnabled( false );
    } else {
        setupCurves( currentCurveBank, TAIL_CURVE_SEL );
        ui->fblTailCurve->setEnabled( true );
        ui->fblTmTail->setEnabled( true );
        ui->fblTailExpo->setEnabled( true );
        ui->fblTailExpoLabel->setEnabled( true );
        ui->fblTailCurve->showDisabledBg( false );
    }
}

void ConfigFBLHeliWidget::on_fblCurveSelector_activated( int index )
{
    selectCurve( index );
}

void ConfigFBLHeliWidget::updateThrottleCurve( QList<double> list, double value )
{
    Q_UNUSED( value );

    // update table if this curve is currently selected
    if( ui->fblCurveSelector->currentIndex() == 0 )
        updateCurveTable( list );
}

void ConfigFBLHeliWidget::updateAilElvCurve( QList<double> list, double value )
{
    Q_UNUSED( value );

    // update table if this curve is currently selected
    if( ui->fblCurveSelector->currentIndex() == 1 )
        updateCurveTable( list );
}

void ConfigFBLHeliWidget::updateCollCurve( QList<double> list, double value )
{
    Q_UNUSED( value );

    // update table if this curve is currently selected
    if( ui->fblCurveSelector->currentIndex() == 2 )
        updateCurveTable( list );
}

void ConfigFBLHeliWidget::updateTailCurve( QList<double> list, double value )
{
    Q_UNUSED( value );

    // update table if this curve is currently selected
    if( ui->fblCurveSelector->currentIndex() == 3 )
        updateCurveTable( list );
}

void ConfigFBLHeliWidget::on_curveDetailTable_itemChanged( QTableWidgetItem *item )
{
    // check if the column is the one to check
    if( item->column() != 1 || setupPhase )
        return;

    // get changed value
    bool check;
    int value = item->text().toInt( &check );

    if( check ) {
        // set value to curve
        // select corresponding widget for data retrieval
        MixerCurveWidget* curCurve;

        switch( ui->fblCurveSelector->currentIndex()) {
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
        curveValues[ item->row() ] = ( 0.5 + ( 0.5 * ( value / 100.0 )));
        curCurve->setCurve( curveValues );
    }
}

void ConfigFBLHeliWidget::on_fblGenerateCurve_clicked()
{
    // find curve to deal with
    MixerCurveWidget* curCurve;

    switch( ui->fblCurveSelector->currentIndex()) {
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

    // generate point field
    QList<double> newCurveValues;
    // get curve parameters entered
    int numPoints = ui->fblNumIntPoints->text().toInt();
    QString maxValText = ui->fblMaximumPoint->text();
    QString minValText = ui->fblMinimumPoint->text();
    // rip off % sign at end
    maxValText.chop( 1 );
    minValText.chop( 1 );
    int maxVal = maxValText.toInt();
    int minVal = minValText.toInt();
    int increment;

    // generate curve using given parameters
    switch( ui->fblCurveType->currentIndex()) {
        case 0:
            // flat, generate
            for( int i = 0; i < numPoints; i++ )
                newCurveValues.append( ( 0.5 + ( 0.5 * ( maxVal / 100.0 ))));
            break;
        case 1:
            // linear rise
            increment = ( maxVal - minVal) / ( numPoints - 1 );
            for( int i = 0; i < numPoints; i++ )
                newCurveValues.append( ( 0.5 + ( 0.5 * (( minVal + i * increment ) / 100.0 ))));
            break;
        case 2:
            // linear fall
            increment = ( maxVal - minVal) / ( numPoints - 1 );
            for( int i = 0; i < numPoints; i++ )
                newCurveValues.append( ( 0.5 + ( 0.5 * (( maxVal - i * increment ) / 100.0 ))));
            break;
        case 3:
            // works only with odd number of points, check and inform user if not OK
            if( numPoints % 2 != 1 ) {
                QMessageBox::critical(0, "ERROR", "V shape curve only available for an odd number of interpolation points!");
                return;
            }
            // V shape
            increment = ( maxVal - minVal) / ( numPoints - 1 ) * 2;
            for( int i = 0; i < numPoints / 2 ; i++ )
                newCurveValues.append( ( 0.5 + ( 0.5 * (( maxVal - i * increment ) / 100.0 ))));
            newCurveValues.append( ( 0.5 + ( 0.5 * ( minVal / 100.0 ))));
            for( int i = 1; i < numPoints / 2 + 1; i++ )
                newCurveValues.append( ( 0.5 + ( 0.5 * (( minVal + i * increment ) / 100.0 ))));
            break;
        default:
            // not to happen
            Q_ASSERT( false );
    }
    // set new curve
    curCurve->blockSignals( true );
    curCurve->clearCurve();
    curCurve->initCurve( newCurveValues );
    updateCurveTable( newCurveValues );
    curCurve->blockSignals( false );
}


void ConfigFBLHeliWidget::on_fblResetBank_clicked()
{
    if( QMessageBox::question( this,
                               "Confirm",
                               QString( "Are you sure you want to reset to saved values of Bank %1?" ).arg( currentCurveBank + 1 ),
                               QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Ok ) {
        setupCurves( currentCurveBank );
        ui->fblBankSelector->setCurrentIndex( currentCurveBank );
    }
}

void ConfigFBLHeliWidget::on_fblLoadBank_clicked()
{
    if( QMessageBox::question( this,
                               "Confirm",
                               QString( "Are you sure you want to load the values of Bank %1?" ).arg( ui->fblBankSelector->currentIndex() + 1 ),
                               QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Ok ) {
        setupCurves( ui->fblBankSelector->currentIndex() );
    }
}

void ConfigFBLHeliWidget::on_fblSaveBank_clicked()
{
    if( QMessageBox::question( this,
                               "Confirm",
                               QString( "Are you sure you want to save the current values to Bank %1?" ).arg( ui->fblBankSelector->currentIndex() + 1 ),
                               QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Ok ) {

        int bank = ui->fblBankSelector->currentIndex();

        // retrieve mixer settings from uav objects
        MixerSettings * mixerSettings = MixerSettings::GetInstance(getObjectManager());
        Q_ASSERT(mixerSettings);
        MixerSettings::DataFields mixerSettingsData = mixerSettings->getData();

        // to avoid code duplication get pointers to the bank objects and use them
        float *curExpo, *curves[ 4 ];

        switch( bank ) {
            case 0:
                curves[ 0 ] = mixerSettingsData.fblThrottleCurveB0;
                curves[ 1 ] = mixerSettingsData.fblAilElvCurveB0;
                curves[ 2 ] = mixerSettingsData.fblCollectiveCurveB0;
                curves[ 3 ] = mixerSettingsData.fblTailCurveB0;
                curExpo = mixerSettingsData.fblExpoB0;
                break;
            case 1:
                curves[ 0 ] = mixerSettingsData.fblThrottleCurveB1;
                curves[ 1 ] = mixerSettingsData.fblAilElvCurveB1;
                curves[ 2 ] = mixerSettingsData.fblCollectiveCurveB1;
                curves[ 3 ] = mixerSettingsData.fblTailCurveB1;
                curExpo = mixerSettingsData.fblExpoB1;
                break;
            case 2:
                curves[ 0 ] = mixerSettingsData.fblThrottleCurveB2;
                curves[ 1 ] = mixerSettingsData.fblAilElvCurveB2;
                curves[ 2 ] = mixerSettingsData.fblCollectiveCurveB2;
                curves[ 3 ] = mixerSettingsData.fblTailCurveB2;
                curExpo = mixerSettingsData.fblExpoB2;
                break;
            default:
                Q_ASSERT( false );
        }
        // save expo settings to matching bank
        curExpo[ 0 ] = ui->fblThrottleCurve->getExpo();
        curExpo[ 1 ] = ui->fblAilElvCurve->getExpo();
        curExpo[ 2 ] = ui->fblCollCurve->getExpo();
        curExpo[ 3 ] = ui->fblTailCurve->getExpo();

        QList<double> curveValues;
        // save curve values
        for( int i = 0; i < 4; i++ ) {
            switch( i ) {
                case 0:
                    curveValues = ui->fblThrottleCurve->getCurve();
                    break;
                case 1:
                    curveValues = ui->fblAilElvCurve->getCurve();
                    break;
                case 2:
                    curveValues = ui->fblCollCurve->getCurve();
                    break;
                case 3:
                    curveValues = ui->fblTailCurve->getCurve();
                    break;
            }

            for( int j = 0; j < curveValues.size(); j++ ) {
                curves[ i ][ j ] = curveValues[ j ];
            }
        }

        mixerSettings->setData(mixerSettingsData);
        mixerSettings->updated();
    }
}

void ConfigFBLHeliWidget::on_fblThrottleExpo_valueChanged( int arg1 )
{
    ui->fblThrottleCurve->setExpo( arg1 );
}

void ConfigFBLHeliWidget::on_fblAilElvExpo_valueChanged( int arg1 )
{
    ui->fblAilElvCurve->setExpo( arg1 );
}

void ConfigFBLHeliWidget::on_fblCollExpo_valueChanged( int arg1 )
{
    ui->fblCollCurve->setExpo( arg1 );
}

void ConfigFBLHeliWidget::on_fblTailExpo_valueChanged( int arg1 )
{
    ui->fblTailCurve->setExpo( arg1 );
}

//--------------- Curves Tab END -----------------



//--------------- Expert Tab BEGIN ---------------


//--------------- Expert Tab END -----------------

