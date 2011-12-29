#ifndef CONFIGBANDWIDTH_H
#define CONFIGBANDWIDTH_H

#include "configtaskwidget.h"
#include "ui_configbandwidth.h"

class UAVObject;

class ConfigBandwidth : public ConfigTaskWidget
{
    Q_OBJECT

public:
    ConfigBandwidth(QWidget *parent);
    ~ConfigBandwidth();

protected:
    virtual void enableControls(bool enable);

private:
    Ui::ConfigBandwidthForm ui;

private slots:
    void addUAVObject(UAVObject *object);
    void removeUAVObject(UAVObject *object);
};

#endif // CONFIGBANDWIDTH_H
