#include "bandwidthform.h"

#include <QDebug>

#include "uavobject.h"
#include "smartsavebutton.h"

BandwidthForm::BandwidthForm(UAVObject &uavObject, QWidget *parent, const bool showLegend) throw(const std::invalid_argument&) :
        QWidget(parent),
        m_ui(),
        m_uavObject(uavObject)
//        m_smartSaveButton(NULL)
{
    //FIXME: add support for periodic logging

    // check for periodic UAVObject
    UAVObject::Metadata metaData = uavObject.getMetadata();
    if (metaData.flightTelemetryUpdateMode != UAVObject::UPDATEMODE_PERIODIC)
    {
        throw std::invalid_argument(uavObject.getName().toStdString() + std::string(" is not periodic"));
    }

    // check for write permission
    if (metaData.gcsAccess != UAVObject::ACCESS_READWRITE)
    {
        throw std::invalid_argument(uavObject.getName().toStdString() + std::string(" is not writeable"));
    }

    m_ui.setupUi(this);

    if(!showLegend)
    {
        // Remove legend
        QGridLayout *grid_layout = dynamic_cast<QGridLayout*>(layout());
        Q_ASSERT(grid_layout);
        for (int col = 0; col < grid_layout->columnCount(); col++)
        { // remove every item in first row
            QLayoutItem *item = grid_layout->itemAtPosition(0, col);
            if (!item) continue;
            // get widget from layout item
            QWidget *legend_widget = item->widget();
            if (!legend_widget) continue;
            // delete widget
            grid_layout->removeWidget(legend_widget);
            delete legend_widget;
        }
    }

    setName(uavObject.getName());
    setDescription(uavObject.getDescription());
    setPeriod(metaData.flightTelemetryUpdatePeriod);
    connect(m_ui.spinBox, SIGNAL(valueChanged(int)),
            this, SLOT(updateUAVObject(int)));
    connect(&uavObject, SIGNAL(objectUpdated(UAVObject*)),
            this, SLOT(updateWidgets(UAVObject*)));

    // setup save button
//    m_smartSaveButton = new smartSaveButton(NULL, m_ui.saveButton);
//    Q_ASSERT(m_smartSaveButton);
//    m_smartSaveButton->addObject(dynamic_cast<UAVDataObject*>(&uavObject));
//    connect(m_smartSaveButton, SIGNAL(beginOp()),
//            this, SLOT(disable()));
//    connect(m_smartSaveButton, SIGNAL(endOp()),
//            this, SLOT(enable()));
}

BandwidthForm::~BandwidthForm()
{
//    if (m_smartSaveButton)
//        delete m_smartSaveButton;
}

/// set description as tool tip
void BandwidthForm::setDescription(QString description)
{
    description.remove("@Ref", Qt::CaseInsensitive);
    // do a line break after approx. 40 characters
    int linebreak = 40;
    int lb_index = -1;
    while (true)
    {
        lb_index = description.indexOf(" ", linebreak);
        if(lb_index < 0)
            break;
        linebreak += 40;
        description.replace(lb_index, 1, '\n');
    }
    m_ui.label->setToolTip(description);
}

/// update telemetry period
void BandwidthForm::updateWidgets(UAVObject *object)
{
    if (!object) return;

    UAVObject::Metadata metaData = object->getMetadata();
    if (m_ui.spinBox->value() != metaData.flightTelemetryUpdatePeriod)
        setPeriod(metaData.flightTelemetryUpdatePeriod);
}

void BandwidthForm::updateUAVObject(int period)
{
    UAVObject::Metadata metaData = m_uavObject.getMetadata();
    metaData.flightTelemetryUpdatePeriod = period;
    m_uavObject.setMetadata(metaData);
    m_uavObject.updated();
}
