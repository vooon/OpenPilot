/**
 ******************************************************************************
 *
 * @file       configfblheliwidget.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
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
#include <QMessageBox>
#include "../uavobjectwidgetutils/configtaskwidget.h"

//--------------- Preset Tab BEGIN ---------------
//--------------- Preset Tab END -----------------

//--------------- Swash Tab BEGIN ---------------
//--------------- Swash Tab END -----------------

//--------------- Curves Tab BEGIN ---------------
static const int THROTTLE_CURVE = 0;        /*!< ID for throttle curve                  */
static const int AILELV_CURVE = 1;          /*!< ID for aileron/elevator curve          */
static const int COLLECTIVE_CURVE = 2;      /*!< ID for collective pitch curve          */
static const int TAIL_CURVE = 3;            /*!< ID for tail rotor curve                */
static const int THROTTLE_CURVE_SEL = 1;    /*!< BITWISE id for throttle curve          */
static const int AILELV_CURVE_SEL = 2;      /*!< BITWISE id for aileron/elevator curve  */
static const int COLLECTIVE_CURVE_SEL = 4;  /*!< BITWISE id for collective pitch curve  */
static const int TAIL_CURVE_SEL = 8;        /*!< BITWISE id for tail rotor curve        */
//--------------- Curves Tab END -----------------

//--------------- Expert Tab BEGIN ---------------
//--------------- Expert Tab END -----------------


namespace Ui {
    class ConfigFBLHeliWidget;
}

//! Flybarless Helicopter Configuration test class.
/*!
    This class holds the code connected to the flybarless helicopter configuration widget
    usd inside GCS. It consists of 4 TABs that allow simple usage for non-techie users
    but at the same time allows experts to configure all aspects of the OpenPilot flight
    software.
*/
class ConfigFBLHeliWidget : public ConfigTaskWidget
{
    Q_OBJECT

public:
    //! Class constructor
    /*!
      simple class constructor to set up the widget
      \param parent the parent widget
    */
    explicit ConfigFBLHeliWidget(QWidget *parent = 0);
    //! Class destructor
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
    Ui::ConfigFBLHeliWidget *ui;        /*!< pointer to the root widget */

    //--------------- Preset Tab BEGIN ---------------
    //--------------- Preset Tab END -----------------

    //--------------- Swash Tab BEGIN ---------------
    //--------------- Swash Tab END -----------------

    //--------------- Curves Tab BEGIN ---------------

    int currentCurveBank;               /*!< holds currently selected setting bank */
    bool setupPhase;                    /*!< guard for setup phase to minimize signal/slot activity */

    //! loads a curve from the given UAV Object and puts it to the given curve widget
    /*!
      This method is called by setupCurves() to set a single curve. Was implemented
      to avoid code duplication there.
      \param widget the curve widget to set the curve data to
      \param data UAVObject holding the curve data
      \sa setupCurves()
    */
    void setupCurve( MixerCurveWidget *widget, UAVObjectField *data );

    //! selects a specific curve and loads it to the curve details
    /*!
      This method loads the details of a specific curve to the curve detail widgets on
      the right of the tab.
      \param curve ID of the curve to load the data from
    */
    void selectCurve( int curve );

    //! loads the points of a specific curve to the detail data table
    /*!
      This method loads the points of a specific curve data points and populates the
      table on the right of the tab.
      \param values list of curve point values
    */
    void updateCurveTable( QList<double> &values );

    //--------------- Curves Tab END -----------------

    //--------------- Expert Tab BEGIN ---------------
    //--------------- Expert Tab END -----------------


protected:

    //--------------- Preset Tab BEGIN ---------------
    //--------------- Preset Tab END -----------------

    //--------------- Swash Tab BEGIN ---------------
    //--------------- Swash Tab END -----------------

    //--------------- Curves Tab BEGIN ---------------

    //! loads the selected curves off a specific setting bankand sets them to the widgets
    /*!
      This method uses a bitwise representation for each of the curves to allow selecting
      multiple curves by bitwise or operations using the defined IDs
      \param bank bank number to load data from
      \param which bitfield selecting which curves to load, default is all (15)
    */
    void setupCurves( int bank, int which = 15 );

    //! setup the curve definition tab of the fbl heli widget
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

    //! test slider changes slot
    /*!
      Update the test lines on all curves that have the test checkbox activated
      \param value slider value from slider widget
    */
    void on_fblTestSlider_valueChanged( int value );

    //! test mode for throttle curve slot
    /*!
      \param checked status of the checkbox
    */
    void on_fblTmThrottle_toggled( bool checked );

    //! test mode for Aileron/Elevator curve slot
    /*!
      \param checked status of the checkbox
    */
    void on_fblTmAilElv_toggled( bool checked );

    //! test mode for Collective Pitch curve slot
    /*!
      \param checked status of the checkbox
    */
    void on_fblTmColl_toggled( bool checked );

    //! test mode for Tail Rotor curve slot
    /*!
      \param checked status of the checkbox
    */
    void on_fblTmTail_toggled( bool checked );

    //! bypass mode for throttle curve slot
    /*!
      \param checked status of the checkbox
    */
    void on_fblPtThrottle_toggled( bool checked );

    //! bypass mode for Aileron/Elevator curve slot
    /*!
      \param checked status of the checkbox
    */
    void on_fblPtAilElv_toggled( bool checked );

    //! bypass mode for Collective Pitch curve slot
    /*!
      \param checked status of the checkbox
    */
    void on_fblPtColl_toggled( bool checked );

    //! bypass mode for Tail Rotor curve slot
    /*!
      \param checked status of the checkbox
    */
    void on_fblPtTail_toggled( bool checked );

    //! service method for combobox to select displayed curve
    /*!
      \param index index of curve selected
    */
    void on_fblCurveSelector_activated( int index );

    //! callback slot method for editing throttle curve inside curve widget
    /*!
      \param list list of points after change
      \param value unused here
    */
    void updateThrottleCurve( QList<double> list, double value );

    //! callback slot method for editing Aileron/Elevatorcurve inside curve widget
    /*!
      \param list list of points after change
      \param value unused here
    */
    void updateAilElvCurve( QList<double> list, double value );

    //! callback slot method for editing collective pitch curve inside curve widget
    /*!
      \param list list of points after change
      \param value unused here
    */
    void updateCollCurve( QList<double> list, double value );

    //! callback slot method for editing tail rotor curve inside curve widget
    /*!
      \param list list of points after change
      \param value unused here
    */
    void updateTailCurve( QList<double> list, double value );

    //! callback slot method for editing curve values inside the detail table
    /*!
      \param item table item that changed
    */
    void on_curveDetailTable_itemChanged(QTableWidgetItem *item);

    //! slot for generate curve button, generates curve based on parameters given
    void on_fblGenerateCurve_clicked();

    //! slot for reset button for bank switching, resets back to originak bank values
    void on_fblResetBank_clicked();

    //! slot for load button for bank switching, loads bank settings
    void on_fblLoadBank_clicked();

    //! slot for save button for bank switching, saves bank settings
    void on_fblSaveBank_clicked();

    //--------------- Curves Tab END -----------------

    //--------------- Expert Tab BEGIN ---------------
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
