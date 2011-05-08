/**
 ******************************************************************************
 * @file
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup uavsettingsmanagerplugin
 * @{
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

#ifndef UAVSETTINGSMANAGERGADGETCONFIGURATION_H
#define UAVSETTINGSMANAGERGADGETCONFIGURATION_H

#include "uavsettingsmanager_global.h"
#include <coreplugin/iuavgadgetconfiguration.h>
#include <uavobjectbrowser/uavobjecttreemodel.h>

using namespace Core;

/* This is a generic bargraph dial
   supporting one indicator.
  */
class UAVSETTINGSMANAGER_EXPORT UAVSettingsManagerGadgetConfiguration : public IUAVGadgetConfiguration
{
    Q_OBJECT
public:
    UAVSettingsManagerGadgetConfiguration(QString classId, QSettings* qSettings = 0, UAVConfigInfo *configInfo = 0, QObject *parent = 0);

    //set dial configuration functions
    void setIniFile(QString filename) {iniFile = filename;}

    //get dial configuration functions
    QString getIniFile() const{return iniFile;}
    QList< QList<UAVDataObject*> > getObjList() const{return m_objList;}
    void setCheckState(UAVObject* obj, Qt::CheckState checked){m_checkState[obj->getObjID()] = checked;}
    Qt::CheckState checkState(UAVObject* obj){return obj ? m_checkState[obj->getObjID()] : Qt::Unchecked;}

    void readConfig(QSettings* qSettings, UAVConfigInfo* configInfo);
    void saveConfig(QSettings* settings, Core::UAVConfigInfo *configInfo) const;
    IUAVGadgetConfiguration *clone();

private:
    UAVDataObject* findObject(QString& name) const;
    QList< QList<UAVDataObject*> > m_objList;
    QHash<quint32, Qt::CheckState> m_checkState; // Keys are UAVObject-IDs
    QString iniFile;
};

#endif // UAVSETTINGSMANAGERGADGETCONFIGURATION_H
/**
 * @}
 * @}
 */
