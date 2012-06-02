#include "pathplannercontroller.h"

#include <QDebug>

#include <waypoint.h>
#include <waypointactive.h>

#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"

#include "opmapcontrol/opmapcontrol.h"


PathPlannerController::PathPlannerController(QObject* parent) : QObject(parent), m_pluginManager(NULL), m_objectManager(NULL), m_waypoint(NULL)
{
    m_pluginManager = ExtensionSystem::PluginManager::instance();
    Q_ASSERT(m_pluginManager);
    m_objectManager = m_pluginManager->getObject<UAVObjectManager>();
    Q_ASSERT(m_objectManager);
    m_waypoint = Waypoint::GetInstance(m_objectManager);
    Q_ASSERT(m_waypoint != NULL);
}

PathPlannerController::~PathPlannerController()
{

}

void PathPlannerController::waypointAdded(mapcontrol::WayPointItem* wp)
{
    quint32 newInstId = m_objectManager->getNumInstances(m_waypoint->getObjID());

    if (wp->Number() < newInstId)
    {
        --newInstId;
        Q_ASSERT(newInstId == wp->Number());
        Waypoint *obj = Waypoint::GetInstance(m_objectManager, wp->Number());
        WaypointUtil::UpdateWaypoint(wp, obj);
    }

    else
    {
        Waypoint *obj = new Waypoint();
        quint32 newInstId = m_objectManager->getNumInstances(m_waypoint->getObjID());
        obj->initialize(newInstId, obj->getMetaObject());
        m_objectManager->registerObject(obj);
        WaypointUtil::UpdateWaypoint(wp, obj);
    }
}

void PathPlannerController::waypointChanged(mapcontrol::WayPointItem* wp)
{
    Waypoint *obj = Waypoint::GetInstance(m_objectManager, wp->Number());
    WaypointUtil::UpdateWaypoint(wp, obj);
}

void PathPlannerController::waypointDeleted(mapcontrol::WayPointItem* wp)
{
    Waypoint *obj = Waypoint::GetInstance(m_objectManager, wp->Number());
    //m_objectManager->unregisterObject(obj);
    delete obj;
    obj = NULL;
    //WaypointUtil::DeleteWaypoint(wp, obj);
}

void WaypointUtil::DeleteWaypoint(mapcontrol::WayPointItem* src, Waypoint* dest)
{
    Q_ASSERT(src);
    Q_ASSERT(dest);
}

void WaypointUtil::UpdateWaypoint(mapcontrol::WayPointItem* src, Waypoint* dest)
{
    Q_ASSERT(src);
    Q_ASSERT(dest);
    Waypoint::DataFields waypoint = dest->getData();

    waypoint.Position[Waypoint::POSITION_NORTH] = src->Coord().Lat();
    waypoint.Position[Waypoint::POSITION_EAST] = src->Coord().Lng();
    waypoint.Position[Waypoint::POSITION_DOWN] = src->Altitude();

    dest->setData(waypoint);
    dest->updated();
    qDebug() << "Set data for instance " << dest->getInstID();
}
