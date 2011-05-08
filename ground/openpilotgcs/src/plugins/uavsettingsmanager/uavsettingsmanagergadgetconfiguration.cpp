/**
 ******************************************************************************
 *
 * @file       uavsettingsmanagergadgetconfiguration.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @brief      Configuration for UAV Settings Manager Plugin
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup   uavsettingsmanagerplugin
 * @{
 *
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

#include "uavsettingsmanagergadgetconfiguration.h"
#include "uavobjectmanager.h"
#include <extensionsystem/pluginmanager.h>
#include <QDomDocument>
#include <QDebug>

static const QString VERSION = "1.0.0";

/**
 * Loads a saved configuration or defaults if non exist.
 *
 */
UAVSettingsManagerGadgetConfiguration::UAVSettingsManagerGadgetConfiguration(QString classId, QSettings* qSettings, UAVConfigInfo *configInfo, QObject *parent) :
        IUAVGadgetConfiguration(classId, parent)
{
    if ( ! qSettings )
        return;

    iniFile = qSettings->value("iniFile", "gcs.xml").toString();

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    m_objList = pm->getObject<UAVObjectManager>()->getDataObjects();
    for (int i = 0; i < m_objList.size(); i++ ) {
        for ( int j = 0; j < m_objList.at(i).size(); j++){
            m_objList[i][j] = m_objList.at(i).at(j)->clone();
            m_checkState[m_objList.at(i).at(j)->getObjID()] = Qt::Unchecked;
        }
    }

    readConfig(qSettings, configInfo);

}

/**
 * Clones a configuration.
 *
 */
IUAVGadgetConfiguration *UAVSettingsManagerGadgetConfiguration::clone()
{
    UAVSettingsManagerGadgetConfiguration *m = new UAVSettingsManagerGadgetConfiguration(this->classId());
    m->iniFile = iniFile;
    m->m_objList = m_objList;
    for (int i = 0; i < m_objList.size(); i++ ) {
        for ( int j = 0; j < m_objList.at(i).size(); j++){
            m->m_objList[i][j] = m_objList.at(i).at(j)->clone();
            m->m_checkState[m_objList[i][j]->getObjID()] = m_checkState[m_objList[i][j]->getObjID()];
        }
    }
    return m;
}

/**
 * Saves a configuration.
 *
 */
void UAVSettingsManagerGadgetConfiguration::saveConfig(QSettings* qSettings, Core::UAVConfigInfo *configInfo) const {
    configInfo->setVersion(VERSION);
    qSettings->setValue("iniFile", iniFile);


    // iterate over settings objects
    qSettings->beginGroup("objects");
    foreach (QList<UAVDataObject*> list, m_objList) {
        foreach (UAVDataObject* obj, list) {
            if (obj->isSettings()) {

                // add each object to the XML
                qSettings->beginGroup(obj->getName());

                qSettings->setValue("id", obj->getObjID());

                // iterate over fields
                foreach (UAVObjectField* field, obj->getFields()) {

                    qSettings->beginWriteArray(field->getName());
                    qint32 nelem = field->getNumElements();
                    for (int n = 0; n < nelem; ++n) {
                        qSettings->setArrayIndex(n);
                        qSettings->setValue("value", field->getValue(n).toString());
                    }
                    qSettings->endArray();

                }
                qSettings->endGroup();
            }
        }
    }
    qSettings->endGroup();

    qSettings->beginWriteArray("checkStates");

    QHashIterator<quint32, Qt::CheckState> iter(m_checkState);
    int idx = 0;
     while (iter.hasNext()) {
         iter.next();
         qSettings->setArrayIndex(idx++);
         qSettings->setValue("id", iter.key());
         qSettings->setValue("checked", iter.value());
     }
     qSettings->endArray();

}

/**
 * Read a configuration from Settings.
 *
 */
void UAVSettingsManagerGadgetConfiguration::readConfig(QSettings* qSettings, UAVConfigInfo* configInfo){
    UAVDataObject* obj;

    if ( configInfo->version() == UAVConfigVersion() )
        configInfo->setVersion("1.0.0");

    if ( !configInfo->standardVersionHandlingOK(VERSION))
        return;


    qSettings->beginGroup("objects");
    QStringList objNames = qSettings->childGroups();
    foreach ( QString objName, objNames ){
        // qDebug() << "UAVSettingsManagerGadgetConfiguration::readConfig Object:" << objName;
        if ( 0 == (obj = findObject(objName)) ){
            qDebug() << "UAVSettingsManagerGadgetConfiguration::readConfig Object not found:" << objName;
            continue;
        }
        // Get the Fields
        qSettings->beginGroup(objName);
        QStringList fieldNames = qSettings->childGroups();
        foreach ( QString fieldName, fieldNames ){
            // qDebug() << "UAVSettingsManagerGadgetConfiguration::readConfig     Field:" << fieldName;
            if ( ! obj->getField(fieldName) ){
                qDebug() << "UAVSettingsManagerGadgetConfiguration::readConfig Field not found:" << fieldName;
                continue;
            }
            int size = qSettings->beginReadArray(fieldName);
            for ( int i = 0; i < size; ++i){
                qSettings->setArrayIndex(i);
                QString value = qSettings->value("value").toString();
                // qDebug() << "UAVSettingsManagerGadgetConfiguration::readConfig Value:" << value;
                obj->getField(fieldName)->setValue(value, i);
            }
            qSettings->endArray();
        }

        qSettings->endGroup();
    }
    qSettings->endGroup();

    int size = qSettings->beginReadArray("checkStates");
    for ( int i = 0; i < size; ++i){
        qSettings->setArrayIndex(i);
        quint32 id = qSettings->value("id").toInt();
        Qt::CheckState checked = static_cast<Qt::CheckState>(qSettings->value("checked",Qt::Unchecked).toInt());
        m_checkState[id] = checked;
    }
    qSettings->endArray();
}

UAVDataObject* UAVSettingsManagerGadgetConfiguration::findObject(QString& name) const{

    foreach (QList<UAVDataObject*> list, m_objList) {
        foreach (UAVDataObject* obj, list) {
            if (obj->getName() == name) {
                return obj;
            }
        }
    }
    return 0;
}

/**
 * @}
 * @}
 */
