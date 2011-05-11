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
#ifndef UAVSETTINGSMANAGERGADGETWIDGET_H
#define UAVSETTINGSMANAGERGADGETWIDGET_H

#include <QWidget>
#include <QString>
#include <QItemSelection>
#include <coreplugin/iconfigurableplugin.h>
#include "objectpersistence.h"
#include <uavobjectbrowser/uavobjecttreemodel.h>
#include "uavsettingsmanagergadgetconfiguration.h"

namespace Ui
{
class UAVSettingsManagerGadgetWidget;
}

class UAVSETTINGSMANAGER_EXPORT UAVSettingsManagerGadgetWidget : public QWidget
{
    Q_OBJECT
public:
    UAVSettingsManagerGadgetWidget(QWidget *parent = 0);
    ~UAVSettingsManagerGadgetWidget();

    void loadConfiguration( UAVSettingsManagerGadgetConfiguration* config);

public slots:
    void showMetaData(bool show);

signals:
    void done();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::UAVSettingsManagerGadgetWidget *ui;
    void setCheckboxes(const QModelIndex& parent);
    static bool copyUAVObject(UAVObject* obj, UAVObject* targetObj);
    static UAVDataObject* dataObject(TreeItem *item);
    static UAVDataObject* dataObjectFromMgr(UAVDataObject *object);
    static void updateObjectPersistance(ObjectPersistence::OperationOptions op, UAVObject *obj);

    ObjectTreeItem *findObjectTreeItem(QModelIndex index);
    UAVObjectTreeModel* m_model;
    UAVSettingsManagerGadgetConfiguration* m_config;

private slots:
    void on_importButton_clicked();
    void on_exportButton_clicked();
    void on_loadButton_clicked();
    void on_saveButton_clicked();
    void on_sendButton_clicked();
    void on_requestButton_clicked();
    void on_invertSelectionButton_clicked();
    void on_helpButton_clicked();
    void dataChanged(const QModelIndex &index, const QModelIndex &index2);
};

#endif // UAVSETTINGSMANAGERGADGETWIDGET_H
/**
 * @}
 * @}
 */
