/**
 ******************************************************************************
 *
 * @file       gpsdisplaygadgetconfiguration.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup GPSGadgetPlugin GPS Gadget Plugin
 * @{
 * @brief A gadget that displays GPS status and enables basic configuration 
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

#include "dviewgadgetconfiguration.h"
#include <qextserialport/src/qextserialport.h>

/**
 * Loads a saved configuration or defaults if non exist.
 *
 */
DViewGadgetConfiguration::DViewGadgetConfiguration(QString classId, QSettings* qSettings, QObject *parent) :
    IUAVGadgetConfiguration(classId, parent),
    m_connectionMode("Serial"),
    m_defaultPort("Unknown"),
    m_defaultSpeed(BAUD4800),
    m_defaultDataBits(DATA_8),
    m_defaultFlow(FLOW_OFF),
    m_defaultParity(PAR_NONE),
    m_defaultStopBits(STOP_1),
    m_defaultTimeOut(5000)
{
    //if a saved configuration exists load it
    if(qSettings != 0) {


    }

}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *DViewGadgetConfiguration::clone()
{


}

/**
 * Saves a configuration.
 *
 */
void DViewGadgetConfiguration::saveConfig(QSettings* settings) const {



}
