#pragma once

#include "Classes/Commands2/Base/RECommand.h"
#include "Classes/Qt/Scene/SceneEditor2.h"

#include <Scene3D/Components/Waypoint/PathComponent.h>

class ToggleWayEditCommand : public RECommand
{
public:
    ToggleWayEditCommand(DAVA::uint32 id, const DAVA::String& description, SceneEditor2* scene);

protected:
    void EnableWayEdit(bool enable);

private:
    SceneEditor2* scene = nullptr;
};

class EnableWayEditCommand : public ToggleWayEditCommand
{
public:
    EnableWayEditCommand(SceneEditor2* scene);
    void Undo() override;
    void Redo() override;
};

class DisableWayEditCommand : public ToggleWayEditCommand
{
public:
    DisableWayEditCommand(SceneEditor2* scene);
    void Undo() override;
    void Redo() override;
};

class WaypointEditCommand : public RECommand
{
public:
    WaypointEditCommand(SceneEditor2* scene, DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint,
                        DAVA::uint32 id, const DAVA::String& description, bool isRemovingCommand);
    ~WaypointEditCommand() override;

    DAVA::Entity* GetEntity() const;

protected:
    void AddWaypoint();
    void RemoveWaypoint();

private:
    SceneEditor2* scene = nullptr;
    DAVA::PathComponent* path = nullptr;
    DAVA::PathComponent::Waypoint* waypoint = nullptr;

    DAVA::uint32 waypointIndex = 0;
    bool isWaypointAdded = false;
};

class AddWaypointCommand : public WaypointEditCommand
{
public:
    AddWaypointCommand(SceneEditor2* scene, DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint);

    void Redo() override;
    void Undo() override;
};

class RemoveWaypointCommand : public WaypointEditCommand
{
public:
    RemoveWaypointCommand(SceneEditor2* scene, DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint);

    void Redo() override;
    void Undo() override;
};

class EdgeEditCommand : public RECommand
{
public:
    EdgeEditCommand(SceneEditor2* scene, DAVA::PathComponent* path,
                    DAVA::PathComponent::Waypoint* waypoint, DAVA::PathComponent::Edge* edge,
                    DAVA::uint32 id, const DAVA::String& description, bool isRemovingCommand);
    ~EdgeEditCommand() override;

    DAVA::Entity* GetEntity() const;

protected:
    void AddEdge();
    void RemoveEdge();

private:
    SceneEditor2* scene = nullptr;
    DAVA::PathComponent* path = nullptr;
    DAVA::PathComponent::Waypoint* waypoint = nullptr;
    DAVA::PathComponent::Waypoint* destination = nullptr;
    DAVA::PathComponent::Edge* edge = nullptr;

    size_t edgeIndex = 0;
    bool isEdgeAdded = false;
};

class AddEdgeCommand : public EdgeEditCommand
{
public:
    AddEdgeCommand(SceneEditor2* scene, DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint, DAVA::PathComponent::Edge* edge);

    void Redo() override;
    void Undo() override;
};

class RemoveEdgeCommand : public EdgeEditCommand
{
public:
    RemoveEdgeCommand(SceneEditor2* scene, DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint, DAVA::PathComponent::Edge* edge);

    void Redo() override;
    void Undo() override;
};
