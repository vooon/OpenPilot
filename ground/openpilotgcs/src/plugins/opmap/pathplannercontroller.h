#ifndef PATHPLANNERCONTROLLER_H
#define PATHPLANNERCONTROLLER_H

#include <QObject>
#include <qmath.h>
#include <waypoint.h>
#include <waypointactive.h>


namespace ExtensionSystem
{
    class PluginManager;
}

namespace mapcontrol
{
    class WayPointItem;
}

class UAVObjectManager;
class UAVObjectUtilManager;


class PathPlannerController : public QObject
{
    Q_OBJECT
public:
    PathPlannerController(QObject *parent = 0);
    ~PathPlannerController();

    void setWaypoints(const QList<mapcontrol::WayPointItem*> &waypoints);
    //QList<mapcontrol::WayPointItem*>& getWaypoints();

    //void Save();
    //QList<mapcontrol::WayPointItem*> Load();

public slots:
    void waypointAdded(mapcontrol::WayPointItem* wp);
    void waypointChanged(mapcontrol::WayPointItem* wp);
    void waypointDeleted(mapcontrol::WayPointItem* wp);

signals:

private:
    Waypoint* AddWaypoint(mapcontrol::WayPointItem* wp);
    void UpdateWaypoint(mapcontrol::WayPointItem* src, Waypoint* dest);

    ExtensionSystem::PluginManager * m_pluginManager;
    UAVObjectManager * m_objectManager;
    UAVObjectUtilManager* m_objUtilManager;
    Waypoint* m_waypoint;
    double m_home[3];
};


namespace WaypointUtil
{
    void UpdateWaypoint(double home[3], mapcontrol::WayPointItem* src, Waypoint* dest);
    void DeleteWaypoint(mapcontrol::WayPointItem* src, Waypoint* dest);
    qreal latDist(mapcontrol::WayPointItem* a, double b, qreal metric=0.0);
    //qreal latDist(mapcontrol::WayPointItem* a, mapcontrol::WayPointItem* b, qreal metric=0.0);
    qreal lngDist(mapcontrol::WayPointItem* a, double b, qreal metric=0.0);
    //qreal lngDist(mapcontrol::WayPointItem* a, mapcontrol::WayPointItem* b, qreal metric=0.0);
    qreal latMetric(mapcontrol::WayPointItem* wp);
    qreal lngMetric(mapcontrol::WayPointItem* wp);
}


#endif // PATHPLANNERCONTROLLER_H
