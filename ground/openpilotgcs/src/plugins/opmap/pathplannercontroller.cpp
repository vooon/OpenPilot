#include "pathplannercontroller.h"

#include <math.h>

#include <QDebug>
#include <qmath.h>

#include <waypoint.h>
#include <waypointactive.h>

#include "extensionsystem/pluginmanager.h"
#include "uavobjectutilmanager.h"
#include "uavobjectmanager.h"
#include "uavobject.h"

#include "opmapcontrol/opmapcontrol.h"


PathPlannerController::PathPlannerController(QObject* parent) :
    QObject(parent),
    m_pluginManager(NULL),
    m_objectManager(NULL),
    m_waypoint(NULL)
{
    m_pluginManager = ExtensionSystem::PluginManager::instance();
    Q_ASSERT(m_pluginManager);
    m_objectManager = m_pluginManager->getObject<UAVObjectManager>();
    Q_ASSERT(m_objectManager);
    m_waypoint = Waypoint::GetInstance(m_objectManager);
    Q_ASSERT(m_waypoint != NULL);
    m_objUtilManager = m_pluginManager->getObject<UAVObjectUtilManager>();
    Q_ASSERT(m_objUtilManager);
}

PathPlannerController::~PathPlannerController()
{

}

void PathPlannerController::waypointAdded(mapcontrol::WayPointItem* wp)
{
    AddWaypoint(wp);
}

void PathPlannerController::waypointChanged(mapcontrol::WayPointItem* wp)
{
    Waypoint *obj = Waypoint::GetInstance(m_objectManager, wp->Number());
    UpdateWaypoint(wp, obj);
}

void PathPlannerController::waypointDeleted(mapcontrol::WayPointItem* wp)
{
    Waypoint *obj = Waypoint::GetInstance(m_objectManager, wp->Number());
    //m_objectManager->unregisterObject(obj);
    delete obj;
    obj = NULL;
    //WaypointUtil::DeleteWaypoint(wp, obj);
}

Waypoint* PathPlannerController::AddWaypoint(mapcontrol::WayPointItem* wp)
{
    Waypoint *obj(NULL);
    quint32 newInstId = m_objectManager->getNumInstances(m_waypoint->getObjID());

    if (wp->Number() < newInstId)
    {
        --newInstId;
        Q_ASSERT(newInstId == wp->Number());
        obj = Waypoint::GetInstance(m_objectManager, wp->Number());
        UpdateWaypoint(wp, obj);
    }

    else
    {
        obj = new Waypoint();
        quint32 newInstId = m_objectManager->getNumInstances(m_waypoint->getObjID());
        obj->initialize(newInstId, obj->getMetaObject());
        m_objectManager->registerObject(obj);
        UpdateWaypoint(wp, obj);
    }

    return obj;
}

void PathPlannerController::UpdateWaypoint(mapcontrol::WayPointItem* src, Waypoint* dest)
{
    Q_ASSERT(src);
    Q_ASSERT(dest);

    double home[3];
    bool isHomeSet = false;

    m_objUtilManager->getHomeLocation(isHomeSet, home);

    if (isHomeSet)
    {
        WaypointUtil::UpdateWaypoint(home, src, dest);
    }

}

void PathPlannerController::setWaypoints(const QList<mapcontrol::WayPointItem*> &waypoints)
{
    // TODO: Should we save this?
    QList<Waypoint*> uavwaypoints;

    foreach(mapcontrol::WayPointItem* wp, waypoints)
    {
        Waypoint *uavWaypoint = AddWaypoint(wp);
        uavwaypoints.append(uavWaypoint);
    }
}

//QList<mapcontrol::WayPointItem*>& PathPlannerController::getWaypoints()
//{
//    QList<mapcontrol::WayPointItem*> waypoints;
//
//    quint32 numUavWaypoints = m_objectManager->getNumInstances(m_waypoint->getObjID());
//    for(quint32 ix=0; ix < numUavWaypoints; ++ix)
//    {
//        Waypoint *obj = Waypoint::GetInstance(m_objectManager, ix);
//        mapcontrol::WayPointItem* waypoint = new mapcontrol::WayPointItem()
//    }
//
//}

void WaypointUtil::DeleteWaypoint(mapcontrol::WayPointItem* src, Waypoint* dest)
{
    Q_ASSERT(src);
    Q_ASSERT(dest);
}

void WaypointUtil::UpdateWaypoint(double home[3], mapcontrol::WayPointItem* src, Waypoint* dest)
{
    Q_ASSERT(src);
    Q_ASSERT(dest);
    Waypoint::DataFields waypoint = dest->getData();

    waypoint.Position[Waypoint::POSITION_NORTH] = WaypointUtil::latDist(src, home[0]);
    waypoint.Position[Waypoint::POSITION_EAST] = WaypointUtil::lngDist(src, home[1]);
    waypoint.Position[Waypoint::POSITION_DOWN] = home[2] - src->Altitude();

    //customData data = static_cast<customData>(src->customData());
    //waypoint.setVelocity(data.velocity);
    //struct customData
    //        {
    //                float velocity;
    //                int mode;
    //                float mode_params[4];
    //                int condition;
    //                float condition_params[4];
    //                int command;
    //                int jumpdestination;
    //                int errordestination;

    dest->setData(waypoint);
    dest->updated();
}

qreal WaypointUtil::latDist(mapcontrol::WayPointItem* a, double b, qreal metric)
{
    const qreal cmetric = metric ? metric : latMetric(a);
    return (a->Coord().Lat() - b) * cmetric;
}

//qreal WaypointUtil::latDist(mapcontrol::WayPointItem* a, mapcontrol::WayPointItem* b, qreal metric)
//{
//    const qreal cmetric = metric ? metric : latMetric(a);
//    return (a->Coord().Lat() - b->Coord().Lat()) * cmetric;
//}

qreal WaypointUtil::lngDist(mapcontrol::WayPointItem* a, double b, qreal metric)
{
    const qreal cmetric = metric ? metric : latMetric(a);
    return (a->Coord().Lng() - b) * cmetric;
}

//qreal WaypointUtil::lngDist(mapcontrol::WayPointItem* a, mapcontrol::WayPointItem* b, qreal metric)
//{
//    const qreal cmetric = metric ? metric : latMetric(a);
//    return (a->Coord().Lng() - b->Coord().Lng()) * cmetric;
//}

qreal WaypointUtil::latMetric(mapcontrol::WayPointItem* wp)
{
    const qreal torad = (qreal)M_PI / 180.0;
    const qreal a2 = 2 * wp->Coord().Lat() * torad;
    const qreal a4 = 4 * wp->Coord().Lat() * torad;
    const qreal result = 111132.954 -(559.822 * qCos(a2)) + (1.175 * qCos(a4));
    return result;
}

qreal WaypointUtil::lngMetric(mapcontrol::WayPointItem* wp)
{
    const qreal torad = (qreal)M_PI / 180.0;
    const qreal a = wp->Coord().Lat() * torad;
    const qreal result = torad * 6367449.0 * qCos(a);
    return result;
}
