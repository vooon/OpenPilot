#ifndef BANDWIDTHFORM_H
#define BANDWIDTHFORM_H

#include <QWidget>
#include <stdexcept>
#include "ui_bandwidthform.h"

class UAVObject;
//class smartSaveButton;

class BandwidthForm : public QWidget
{
    Q_OBJECT

public:
    BandwidthForm(UAVObject &uavObject, QWidget *parent = NULL, const bool showLegend = false) throw(const std::invalid_argument&);
    ~BandwidthForm();

    bool isLinked() const;

private:
    Ui::BandwidthForm m_ui;
    UAVObject &m_uavObject;
//    smartSaveButton *m_smartSaveButton;

    void setDescription(QString description);
    void setName(const QString &name);
    void setPeriod(const int period);

private slots:
    void disable();
    void enable();
    void updateUAVObject(int period);
    void updateWidgets(UAVObject *object);

};

inline void BandwidthForm::disable()
{
    setEnabled(false);
}

inline void BandwidthForm::enable()
{
    setEnabled(true);
}

inline bool BandwidthForm::isLinked() const
{
   return m_ui.checkBox->isChecked();
}

inline void BandwidthForm::setName(const QString &name)
{
    m_ui.label->setText(name);
}

inline void BandwidthForm::setPeriod(const int period)
{
    m_ui.spinBox->setValue(period);
}

#endif // BANDWIDTHFORM_H
