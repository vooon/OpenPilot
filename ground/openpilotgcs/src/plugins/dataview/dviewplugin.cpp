/**
 ******************************************************************************
 *
 * @file       dviewplugin.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup DViewPlugin Do Nothing Plugin
 * @{
 * @brief A minimal example plugin 
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
#include "dviewplugin.h"
#include "dviewgadgetfactory.h"
#include <QtPlugin> 
#include <QStringList> 
#include <extensionsystem/pluginmanager.h>

DViewPlugin::DViewPlugin()
{ 
   // Do nothing 
} 

DViewPlugin::~DViewPlugin()
{ 
   // Do nothing 
} 

bool DViewPlugin::initialize(const QStringList& args, QString *errMsg)
{ 
   Q_UNUSED(args); 
   Q_UNUSED(errMsg); 
   mf = new DViewGadgetFactory(this);
   addAutoReleasedObject(mf);

   return true; 
} 

void DViewPlugin::extensionsInitialized()
{ 
   // Do nothing 
} 

void DViewPlugin::shutdown()
{ 
   // Do nothing 
}
Q_EXPORT_PLUGIN(DViewPlugin)
