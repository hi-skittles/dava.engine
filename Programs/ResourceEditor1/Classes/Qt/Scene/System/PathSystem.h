#pragma once

#include "Classes/Qt/Scene/System/EditorSceneSystem.h"
#include "Classes/Qt/Scene/System/ModifSystem.h"

#include <Scene3D/Components/Waypoint/PathComponent.h>
#include <Scene3D/Components/Waypoint/EdgeComponent.h>
#include <Scene3D/Entity.h>
#include <Entity/SceneSystem.h>
#include <Base/FastName.h>

static const DAVA::uint32 WAYPOINTS_DRAW_LIFTING = 1;

class RECommandNotificationObject;
class SceneEditor2;
class PathSystem : public DAVA::SceneSystem, public EntityModificationSystemDelegate, public EditorSceneSystem
{
public:
    PathSystem(DAVA::Scene* scene);
    ~PathSystem() override;

    void EnablePathEdit(bool enable);
    bool IsPathEditEnabled() const;

    void AddEntity(DAVA::Entity* entity) override;
    void RemoveEntity(DAVA::Entity* entity) override;
    void PrepareForRemove() override;

    void Process(DAVA::float32 timeElapsed) override;

    DAVA::Entity* GetCurrrentPath() const;
    const DAVA::Vector<DAVA::Entity*>& GetPathes() const;

    void WillClone(DAVA::Entity* originalEntity) override;
    void DidCloned(DAVA::Entity* originalEntity, DAVA::Entity* newEntity) override;

    void OnWaypointAdded(DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint);
    void OnWaypointRemoved(DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint);
    void OnEdgeAdded(DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint, DAVA::PathComponent::Edge* edge);
    void OnEdgeRemoved(DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint, DAVA::PathComponent::Edge* edge);
    void OnWaypointDeleted(DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint);
    void OnEdgeDeleted(DAVA::PathComponent* path, DAVA::PathComponent::Waypoint* waypoint, DAVA::PathComponent::Edge* edge);

protected:
    void Draw() override;

    void DrawInEditableMode();
    void DrawInViewOnlyMode();
    void DrawArrow(const DAVA::Vector3& start, const DAVA::Vector3& finish, const DAVA::Color& color);

    void InitPathComponent(DAVA::PathComponent* component);

    DAVA::FastName GeneratePathName() const;
    const DAVA::Color& GetNextPathColor() const;

    void ExpandPathEntity(DAVA::Entity* entity);
    void CollapsePathEntity(DAVA::Entity* entity, DAVA::FastName pathName);

    DAVA::Vector<DAVA::Entity*> pathes;
    DAVA::Set<std::pair<DAVA::Entity*, DAVA::FastName>> entitiesForCollapse;
    DAVA::Set<DAVA::Entity*> entitiesForExpand;
    DAVA::UnorderedMap<DAVA::PathComponent::Waypoint*, DAVA::RefPtr<DAVA::Entity>> entityCache;
    DAVA::UnorderedMap<DAVA::PathComponent::Edge*, DAVA::EdgeComponent*> edgeComponentCache;

    SelectableGroup currentSelection;
    DAVA::Entity* currentPath;

    bool isEditingEnabled;
};

inline const DAVA::Vector<DAVA::Entity*>& PathSystem::GetPathes() const
{
    return pathes;
}

inline DAVA::Entity* PathSystem::GetCurrrentPath() const
{
    return currentPath;
}

inline bool PathSystem::IsPathEditEnabled() const
{
    return isEditingEnabled;
}
