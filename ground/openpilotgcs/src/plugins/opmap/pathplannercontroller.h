#ifndef PATHPLANNERCONTROLLER_H
#define PATHPLANNERCONTROLLER_H

#include <QObject>
#include <waypoint.h>
//#include <waypointactive.h>


namespace ExtensionSystem
{
    class PluginManager;
}

namespace mapcontrol
{
    class WayPointItem;
}

class UAVObjectManager;


namespace WaypointUtil
{
    void UpdateWaypoint(mapcontrol::WayPointItem* src, Waypoint* dest);
    void DeleteWaypoint(mapcontrol::WayPointItem* src, Waypoint* dest);
}

class PathPlannerController : public QObject
{
    Q_OBJECT
public:
    PathPlannerController(QObject *parent = 0);
    ~PathPlannerController();

public slots:
    void waypointAdded(mapcontrol::WayPointItem* wp);
    void waypointChanged(mapcontrol::WayPointItem* wp);
    void waypointDeleted(mapcontrol::WayPointItem* wp);

signals:

private:
    ExtensionSystem::PluginManager * m_pluginManager;
    UAVObjectManager * m_objectManager;
    Waypoint* m_waypoint;
};


#endif // PATHPLANNERCONTROLLER_H
