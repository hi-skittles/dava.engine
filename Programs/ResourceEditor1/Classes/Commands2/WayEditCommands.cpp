#include "Classes/Commands2/WayEditCommands.h"
#include "Classes/Commands2/RECommandIDs.h"
#include "Classes/Selection/Selection.h"

#include <Debug/DVAssert.h>

ToggleWayEditCommand::ToggleWayEditCommand(DAVA::uint32 id, const DAVA::String& description, SceneEditor2* scene_)
    : RECommand(id, description)
    , scene(scene_)
{
}

void ToggleWayEditCommand::EnableWayEdit(bool enable)
{
    bool isEditorEnabled = scene->pathSystem->IsPathEditEnabled();
    DVASSERT(isEditorEnabled != enable);

    bool wasLocked = Selection::Lock();
    scene->pathSystem->EnablePathEdit(enable);
    if (wasLocked == false)
    {
        Selection::Unlock();
    }
}

EnableWayEditCommand::EnableWayEditCommand(SceneEditor2* scene)
    : ToggleWayEditCommand(CMDID_ENABLE_WAYEDIT, "Enable waypoint edit mode", scene)
{
}

void EnableWayEditCommand::Undo()
{
    EnableWayEdit(false);
}

void EnableWayEditCommand::Redo()
{
    EnableWayEdit(true);
}

DisableWayEditCommand::DisableWayEditCommand(SceneEditor2* scene)
    : ToggleWayEditCommand(CMDID_DISABLE_WAYEDIT, "Disable waypoint edit mode", scene)
{
}

void DisableWayEditCommand::Undo()
{
    EnableWayEdit(true);
}

void DisableWayEditCommand::Redo()
{
    EnableWayEdit(false);
}

WaypointEditCommand::WaypointEditCommand(SceneEditor2* scene_, DAVA::PathComponent* path_, DAVA::PathComponent::Waypoint* waypoint_,
                                         DAVA::uint32 id, const DAVA::String& description, bool isRemovingCommand)
    : RECommand(id, description)
    , scene(scene_)
    , path(path_)
    , waypoint(waypoint_)
{
    DVASSERT(path);
    DVASSERT(waypoint);

    isWaypointAdded = false;
    const DAVA::Vector<DAVA::PathComponent::Waypoint*>& points = path->GetPoints();
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (points[i] == waypoint)
        {
            isWaypointAdded = true;
            waypointIndex = static_cast<DAVA::uint32>(i);
            break;
        }
    }

    DVASSERT(isWaypointAdded == isRemovingCommand);
    if (isWaypointAdded == false)
    {
        waypointIndex = static_cast<DAVA::uint32>(points.size());
    }
}

WaypointEditCommand::~WaypointEditCommand()
{
    if (isWaypointAdded == false)
    {
        scene->pathSystem->OnWaypointDeleted(path, waypoint);
        SafeDelete(waypoint);
    }
}

DAVA::Entity* WaypointEditCommand::GetEntity() const
{
    return path->GetEntity();
}

void WaypointEditCommand::AddWaypoint()
{
    DVASSERT(isWaypointAdded == false);

    path->InsertPoint(waypoint, waypointIndex);
    scene->pathSystem->OnWaypointAdded(path, waypoint);
    isWaypointAdded = true;
}

void WaypointEditCommand::RemoveWaypoint()
{
    DVASSERT(isWaypointAdded == true);
    path->ExtractPoint(waypoint);
    scene->pathSystem->OnWaypointRemoved(path, waypoint);
    isWaypointAdded = false;
}

AddWaypointCommand::AddWaypointCommand(SceneEditor2* scene, DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint)
    : WaypointEditCommand(scene, path, waypoint, CMDID_ADD_WAYPOINT, "Add waypoint", false)
{
}

void AddWaypointCommand::Redo()
{
    AddWaypoint();
}

void AddWaypointCommand::Undo()
{
    RemoveWaypoint();
}

RemoveWaypointCommand::RemoveWaypointCommand(SceneEditor2* scene, DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint)
    : WaypointEditCommand(scene, path, waypoint, CMDID_REMOVE_WAYPOINT, "Remove waypoint", true)
{
}

void RemoveWaypointCommand::Redo()
{
    RemoveWaypoint();
}

void RemoveWaypointCommand::Undo()
{
    AddWaypoint();
}

EdgeEditCommand::EdgeEditCommand(SceneEditor2* scene_, DAVA::PathComponent* path_, DAVA::PathComponent::Waypoint* waypoint_,
                                 DAVA::PathComponent::Edge* edge_, DAVA::uint32 id, const DAVA::String& description, bool isRemovingCommand)
    : RECommand(id, description)
    , scene(scene_)
    , path(path_)
    , waypoint(waypoint_)
    , edge(edge_)
{
    DVASSERT(edge->destination != nullptr);
    DVASSERT(edge->destination != waypoint);
    destination = edge->destination;

    isEdgeAdded = false;
    for (size_t i = 0; i < waypoint->edges.size(); ++i)
    {
        if (waypoint->edges[i] == edge)
        {
            isEdgeAdded = true;
            edgeIndex = i;
            break;
        }
    }

    DVASSERT(isEdgeAdded == isRemovingCommand);
    if (isEdgeAdded == false)
    {
        edgeIndex = waypoint->edges.size();
    }
}

EdgeEditCommand::~EdgeEditCommand()
{
    if (isEdgeAdded == false)
    {
        scene->pathSystem->OnEdgeDeleted(path, waypoint, edge);
        SafeDelete(edge);
    }
}

DAVA::Entity* EdgeEditCommand::GetEntity() const
{
    return path->GetEntity();
}

void EdgeEditCommand::AddEdge()
{
    DVASSERT(isEdgeAdded == false);

#if defined(__DAVAENGINE_DEBUG__)
    bool destinationFound = false;
    for (DAVA::PathComponent::Waypoint* wp : path->GetPoints())
    {
        if (wp == destination)
        {
            destinationFound = true;
            break;
        }
    }
    DVASSERT(destinationFound);
#endif

    edge->destination = destination;
    if (edgeIndex < waypoint->edges.size())
    {
        waypoint->edges.insert(waypoint->edges.begin() + edgeIndex, edge);
    }
    else
    {
        waypoint->edges.push_back(edge);
    }

    scene->pathSystem->OnEdgeAdded(path, waypoint, edge);
    isEdgeAdded = true;
}

void EdgeEditCommand::RemoveEdge()
{
    DVASSERT(isEdgeAdded == true);
    DVASSERT(waypoint->edges[edgeIndex] == edge);
    DVASSERT(edge->destination == destination);

    waypoint->edges.erase(waypoint->edges.begin() + edgeIndex);
    scene->pathSystem->OnEdgeRemoved(path, waypoint, edge);
    isEdgeAdded = false;
}

AddEdgeCommand::AddEdgeCommand(SceneEditor2* scene, DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint, DAVA::PathComponent::Edge* edge)
    : EdgeEditCommand(scene, path, waypoint, edge, CMDID_ADD_EDGE, "Add edge", false)
{
}

void AddEdgeCommand::Redo()
{
    AddEdge();
}

void AddEdgeCommand::Undo()
{
    RemoveEdge();
}

RemoveEdgeCommand::RemoveEdgeCommand(SceneEditor2* scene, DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint, DAVA::PathComponent::Edge* edge)
    : EdgeEditCommand(scene, path, waypoint, edge, CMDID_REMOVE_EDGE, "Remove edge", true)
{
}

void RemoveEdgeCommand::Redo()
{
    RemoveEdge();
}

void RemoveEdgeCommand::Undo()
{
    AddEdge();
}
