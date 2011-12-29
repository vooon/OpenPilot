#include "configbandwidth.h"

#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "bandwidthform.h"

ConfigBandwidth::ConfigBandwidth(QWidget *parent) :
        ConfigTaskWidget(parent),
        ui()
{
    ui.setupUi(this);
    setupButtons(ui.updateButton, ui.saveButton);

    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    Q_ASSERT(pm);
    UAVObjectManager *objectManager = pm->getObject<UAVObjectManager>();
    Q_ASSERT(objectManager);

    QList< QList<UAVObject*> > objListList = objectManager->getObjects();
    foreach (QList<UAVObject*> objList, objListList)
        foreach (UAVObject *obj, objList)
                addUAVObject(obj);
    connect(objectManager, SIGNAL(newObject(UAVObject*)),
            this, SLOT(addUAVObject(UAVObject*)));
    connect(objectManager, SIGNAL(newInstance(UAVObject*)),
            this, SLOT(addUAVObject(UAVObject*)));

    enableControls(false);
}

ConfigBandwidth::~ConfigBandwidth()
{
}

void ConfigBandwidth::addUAVObject(UAVObject *object)
{
    if (!object) return;

    try
    {
        BandwidthForm *form = new BandwidthForm(*object, this, ui.bandwidthsAreaWidgetContents->layout()->count() == 0);
        Q_ASSERT(form);
        ui.bandwidthsAreaWidgetContents->layout()->addWidget(form);
        //TODO: ConfigTaskWidget::addUAVOBject
    }
    catch(const std::invalid_argument &ia)
    {
        qDebug() << ia.what();
    }
}

void ConfigBandwidth::enableControls(bool enable)
{
    ConfigTaskWidget::enableControls(enable);
    ui.bandwidthsArea->setEnabled(enable);
}

void ConfigBandwidth::removeUAVObject(UAVObject *object)
{
    //TODO
}
