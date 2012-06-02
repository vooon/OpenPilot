#ifndef SMARTSAVEBUTTON_H
#define SMARTSAVEBUTTON_H

#include "uavtalk/telemetrymanager.h"
#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"
#include <QPushButton>
#include <QList>
#include <QEventLoop>
#include "uavobjectutilmanager.h"
#include <QObject>
#include <QDebug>
class smartSaveButton:public QObject
{
public:
    Q_OBJECT
public:
    smartSaveButton(QPushButton * update,QPushButton * save);
    void setObjects(QList<UAVDataObject *>);
    void addObject(UAVDataObject *);
    void clearObjects();
signals:
    void preProcessOperations();
    void saveSuccessfull();
    void beginOp();
    void endOp();
private slots:
    void processClick();
    void transaction_finished(UAVObject* obj, bool result);
    void saving_finished(int,bool);

private:
    QPushButton *bupdate;
    QPushButton *bsave;
    quint32 current_objectID;
    UAVDataObject * current_object;
    bool up_result;
    bool sv_result;
    QEventLoop loop;
    QList<UAVDataObject *> objects;
protected:
public slots:
    void enableControls(bool value);

};


#endif // SMARTSAVEBUTTON_H
