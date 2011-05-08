/**
 ******************************************************************************
 *
 * @file       uavsettingsmanagergadgetwidget.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @brief      Widget for UAV Settings Manager Plugin
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
#include "uavsettingsmanagergadgetwidget.h"
#include "ui_uavsettingsmanagergadgetwidget.h"
#include "importexport/xmlconfig.h"
#include "coreplugin/uavgadgetinstancemanager.h"
#include "coreplugin/icore.h"
#include "uavobjectmanager.h"
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <uavobjectbrowser/browseritemdelegate.h>
#include <QtDebug>
#include <QSettings>
#include <QMessageBox>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>

UAVSettingsManagerGadgetWidget::UAVSettingsManagerGadgetWidget(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::UAVSettingsManagerGadgetWidget),
        m_model(0)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    ui->setupUi(this);

    ui->configFile->setExpectedKind(Utils::PathChooser::File);
    ui->configFile->setPromptDialogFilter(tr("XML file (*.xml)"));
    ui->configFile->setPromptDialogTitle(tr("Choose configuration file"));


    connect(ui->metaCheckBox, SIGNAL(toggled(bool)), this, SLOT(showMetaData(bool)));
}

UAVSettingsManagerGadgetWidget::~UAVSettingsManagerGadgetWidget()
{
    delete ui;
}

void UAVSettingsManagerGadgetWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
void UAVSettingsManagerGadgetWidget::loadConfiguration(UAVSettingsManagerGadgetConfiguration* config)
{
    Q_ASSERT(config);
    if ( !config )
        return;

    m_config = config;

    if ( m_model){
        disconnect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged(QModelIndex,QModelIndex)));
        delete m_model;
        m_model = 0;
    }
    m_model = new UAVObjectTreeModel(config->getObjList(), false);

    setCheckboxes(m_model->index(0,0));

    ui->configurationName->setText(config->name());
    ui->treeView->setModel(m_model);
    ui->treeView->setColumnWidth(0, 300);
    ui->treeView->expand(m_model->index(0,0));
    BrowserItemDelegate *m_delegate = new BrowserItemDelegate();
    ui->treeView->setItemDelegate(m_delegate);
    ui->treeView->setEditTriggers(QAbstractItemView::AllEditTriggers);
    ui->treeView->setSelectionBehavior(QAbstractItemView::SelectItems);
    connect(m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged(QModelIndex,QModelIndex)));
    showMetaData(ui->metaCheckBox->isChecked());
}

void UAVSettingsManagerGadgetWidget::setCheckboxes(const QModelIndex& index)
{
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    if (dynamic_cast<ObjectTreeItem*>(item) || dynamic_cast<TopTreeItem*>(item)) {
        item->setCheckable(true);
    }

    ObjectTreeItem* objTreeItem = dynamic_cast<ObjectTreeItem*>(item);
    if (objTreeItem) {
        item->setChecked(m_config->checkState(objTreeItem->object()));
    }

    for ( int row = 0; row < m_model->rowCount(index); row++ ){
        setCheckboxes( index.child(row,0));
    }
}

void UAVSettingsManagerGadgetWidget::on_helpButton_clicked()
{
    qDebug() << "Show Help";
    QDesktopServices::openUrl(QUrl("http://wiki.openpilot.org/UAV_Settings_Manager_plugin"));
}


ObjectTreeItem *UAVSettingsManagerGadgetWidget::findObjectTreeItem(QModelIndex index)
{
    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
    ObjectTreeItem *objItem = 0;
    while (item) {
        objItem = dynamic_cast<ObjectTreeItem*>(item);
        if (objItem)
            break;
        item = item->parent();
    }
    return objItem;
}

ObjectTreeItem *UAVSettingsManagerGadgetWidget::findCurrentObjectTreeItem()
{
    return findObjectTreeItem(ui->treeView->currentIndex());
}

void UAVSettingsManagerGadgetWidget::sendUpdate()
{
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();
    Q_ASSERT(objItem);
    // objItem->apply(); // What is this and who needs this?
    UAVObject *obj = objItem->object();
    Q_ASSERT(obj);
    obj->updated();
}

void UAVSettingsManagerGadgetWidget::saveObject()
{
    // Send update so that the latest value is saved
    sendUpdate();
    // Save object
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();
    Q_ASSERT(objItem);
    UAVObject *obj = objItem->object();
    Q_ASSERT(obj);
//    updateObjectPersistance(ObjectPersistence::OPERATION_SAVE, obj);
}

void UAVSettingsManagerGadgetWidget::loadObject()
{
    // Load object
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();
    Q_ASSERT(objItem);
    UAVObject *obj = objItem->object();
    Q_ASSERT(obj);
//    updateObjectPersistance(ObjectPersistence::OPERATION_LOAD, obj);
    // Retrieve object so that latest value is displayed
    // requestUpdate();
}

void UAVSettingsManagerGadgetWidget::eraseObject()
{
    ObjectTreeItem *objItem = findCurrentObjectTreeItem();
    Q_ASSERT(objItem);
    UAVObject *obj = objItem->object();
    Q_ASSERT(obj);
//    updateObjectPersistance(ObjectPersistence::OPERATION_DELETE, obj);
}


void UAVSettingsManagerGadgetWidget::dataChanged(const QModelIndex &index, const QModelIndex &index2)
{
    Q_UNUSED(index2);
    qDebug() << "dataChanged column: " << index.column() << " row: " << index.row();
    ObjectTreeItem* objItem = findObjectTreeItem(index);
    if ( objItem ){
        objItem->apply();
        m_config->setCheckState(objItem->object(), objItem->checked());
    }
}

void UAVSettingsManagerGadgetWidget::showMetaData(bool show)
{
    int topRowCount = m_model->rowCount(QModelIndex());
    for (int i = 0; i < topRowCount; ++i) {
        QModelIndex index = m_model->index(i, 0, QModelIndex());
        int subRowCount = m_model->rowCount(index);
        for (int j = 0; j < subRowCount; ++j) {
            ui->treeView->setRowHidden(0, index.child(j,0), !show);
        }
    }
}


void UAVSettingsManagerGadgetWidget::on_invertSelectionButton_clicked()
{
    m_model->invertChecked();
}

bool UAVSettingsManagerGadgetWidget::copyUAVObject(UAVObject* obj, UAVObject* targetObj)
{
    if ( !obj || !targetObj ) return false;
    QList<UAVObjectField*> fields = obj->getFields();
    foreach ( UAVObjectField* field, fields ){
        UAVObjectField* targetField = targetObj->getField(field->getName());
        Q_ASSERT(targetField);
        for ( quint32 i = 0; i < field->getNumElements(); ++i){
            targetField->setValue(field->getValue(i), i);
        }
    }
    return true;
}

UAVDataObject* UAVSettingsManagerGadgetWidget::dataObject(TreeItem *item)
{
    //qDebug() << "Visit: " << item->data(0) << " Name: " << item->metaObject()->className() << " Checked: " << item->checked();
    ObjectTreeItem *objItem = dynamic_cast<DataObjectTreeItem*>(item); // ignore MetaObject for now
    if ( !objItem || !(item->checked() == Qt::Checked) ){
        return 0;
    }
    return static_cast<UAVDataObject*>(objItem->object());
}

UAVDataObject* UAVSettingsManagerGadgetWidget::dataObjectFromMgr(UAVDataObject *object)
{
    if (!object) return 0;

    UAVDataObject *targetObj = static_cast<UAVDataObject*>(ExtensionSystem::PluginManager::instance()->getObject<UAVObjectManager>()->getObject(object->getObjID()));
    if ( !targetObj ){
        qDebug() << "dataObjectFromMgr: Object-ID: " << object->getObjID() <<
                " for Object " << object->getName() << " not found!";
        return 0;
    }
    return targetObj;
}

void UAVSettingsManagerGadgetWidget::updateObjectPersistance(ObjectPersistence::OperationOptions op, UAVObject *obj)
{
    if ( !obj ) return;

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager *objManager = pm->getObject<UAVObjectManager>();
    ObjectPersistence* objper = dynamic_cast<ObjectPersistence*>( objManager->getObject(ObjectPersistence::NAME) );
    ObjectPersistence::DataFields data;

    data.Operation = op;
    data.Selection = ObjectPersistence::SELECTION_SINGLEOBJECT;
    data.ObjectID = obj->getObjID();
    data.InstanceID = obj->getInstID();
    objper->setData(data);
    objper->updated();
}


void UAVSettingsManagerGadgetWidget::on_sendButton_clicked()
{
    // qDebug() << "on_sendButton_clicked: Visit: " << item->data(0) << " Name: " << item->metaObject()->className() << " Checked: " << item->checked();

    class : public UAVObjectTreeModelVisitor
    {
        virtual void visit(TreeItem *item){

            UAVDataObject *obj = dataObject(item);
            UAVDataObject *mgrObj = dataObjectFromMgr(obj);

            if ( copyUAVObject(obj, mgrObj) ) mgrObj->updated();
        }
    } visitor;

    m_model->accept(visitor);
}


void UAVSettingsManagerGadgetWidget::on_requestButton_clicked()
{
    class : public UAVObjectTreeModelVisitor
    {
    public:
        virtual void visit(TreeItem *item){

            UAVDataObject *obj = dataObject(item);
            UAVDataObject *mgrObj = dataObjectFromMgr(obj);

            copyUAVObject(mgrObj, obj);
        }
    } visitor;

    m_model->accept(visitor);
    loadConfiguration(m_config); // Load whole configuuration because of update-problems with TreeItems.

}

void UAVSettingsManagerGadgetWidget::on_saveButton_clicked()
{
    class : public UAVObjectTreeModelVisitor
    {
        virtual void visit(TreeItem *item){

            UAVDataObject *obj = dataObject(item);
            UAVDataObject *mgrObj = dataObjectFromMgr(obj);

            if ( copyUAVObject(obj, mgrObj) ){
                updateObjectPersistance(ObjectPersistence::OPERATION_SAVE, mgrObj);
                mgrObj->updated();
            }
        }
    } visitor;

    m_model->accept(visitor);

}

void UAVSettingsManagerGadgetWidget::on_loadButton_clicked()
{
    class : public UAVObjectTreeModelVisitor
    {
        virtual void visit(TreeItem *item){

            UAVDataObject *obj = dataObject(item);
            UAVDataObject *mgrObj = dataObjectFromMgr(obj);

            updateObjectPersistance(ObjectPersistence::OPERATION_LOAD, mgrObj);
            copyUAVObject(mgrObj, obj);
        }

    } visitor;

    m_model->accept(visitor);
    loadConfiguration(m_config); // Load whole configuuration because of update-problems with TreeItems.

}

void UAVSettingsManagerGadgetWidget::on_exportButton_clicked()
{
    QString file = ui->configFile->path();
    qDebug() << "Export pressed! Write to file " << QFileInfo(file).absoluteFilePath();

    if ( QFileInfo(file).exists() ){
        QMessageBox msgBox;
        msgBox.setText(tr("File already exists."));
        msgBox.setInformativeText(tr("Do you want to overwrite the existing file?"));
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Ok);
        if ( msgBox.exec() == QMessageBox::Ok ){
            QFileInfo(file).absoluteDir().remove(QFileInfo(file).fileName());
        }
        else{
            qDebug() << "Export canceled!";
            return;
        }
    }
    QMessageBox msgBox;
    QDir dir = QFileInfo(file).absoluteDir();
    if (! dir.exists()) {
        msgBox.setText(tr("Can't write file ") + QFileInfo(file).absoluteFilePath()
                       + " since directory "+ dir.absolutePath() + " doesn't exist!");
        msgBox.exec();
        return;
    }

    QSettings::Format format = XmlConfig::XmlSettingsFormat;

    QSettings qs(file, format);

    UAVConfigInfo configInfo;

    qs.beginGroup("UAVSettings");
    qs.beginGroup("data");
    m_config->saveConfig(&qs, &configInfo);
    qs.endGroup();
    configInfo.save(&qs);
    qs.endGroup();

    msgBox.setText(tr("The settings have been exported to ") + QFileInfo(file).absoluteFilePath());
    msgBox.exec();

}

void UAVSettingsManagerGadgetWidget::on_importButton_clicked()
{
    QString file = ui->configFile->path();
    qDebug() << "Import pressed! Read from file " << QFileInfo(file).absoluteFilePath();
    QMessageBox msgBox;
    if (! QFileInfo(file).isReadable()) {
        msgBox.setText(tr("Can't read file ") + QFileInfo(file).absoluteFilePath());
        msgBox.exec();
        return;
    }

    QSettings::Format format = XmlConfig::XmlSettingsFormat;

    QSettings qs(file, format);

    UAVConfigInfo configInfo;

    qs.beginGroup("UAVSettings");
    configInfo.read(&qs);
    configInfo.setNameOfConfigurable("UAVSettings");
    qs.beginGroup("data");
    m_config->readConfig(&qs, &configInfo);

    qs.endGroup();
    qs.endGroup();

    loadConfiguration(m_config);
    msgBox.setText(tr("The settings have been imported from ") + QFileInfo(file).absoluteFilePath());
    msgBox.exec();

}

/**
 * @}
 * @}
 */


