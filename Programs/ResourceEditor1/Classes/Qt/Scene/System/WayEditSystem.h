#pragma once

#include "Classes/Selection/SelectableGroup.h"
#include "Scene/SceneTypes.h"
#include "Classes/Qt/Scene/System/EditorSceneSystem.h"

#include "SystemDelegates.h"

// framework
#include "UI/UIEvent.h"
#include "Entity/SceneSystem.h"
#include "Render/RenderHelper.h"
#include "Scene3D/Components/Waypoint/EdgeComponent.h"

// editor systems
#include "Scene/System/CollisionSystem.h"

// delegate
#include "Scene/System/StructureSystem.h"
#include "Scene/System/ModifSystem.h"

class RECommandNotificationObject;
class SceneEditor2;

class WayEditSystem : public DAVA::SceneSystem,
                      public EntityModificationSystemDelegate,
                      public StructureSystemDelegate,
                      public SelectionSystemDelegate,
                      public EditorSceneSystem

{
    friend class SceneEditor2;

public:
    WayEditSystem(DAVA::Scene* scene);
    ~WayEditSystem() = default;

    void EnableWayEdit(bool enable);
    bool IsWayEditEnabled() const;

    void Process(DAVA::float32 timeElapsed) override;
    bool Input(DAVA::UIEvent* event) override;

    void AddEntity(DAVA::Entity* entity) override;
    void RemoveEntity(DAVA::Entity* entity) override;
    void PrepareForRemove() override;

    bool HasCustomClonedAddading(DAVA::Entity* entityToClone) const override;
    void PerformAdding(DAVA::Entity* sourceEntity, DAVA::Entity* clonedEntity) override;

    bool HasCustomRemovingForEntity(DAVA::Entity* entityToRemove) const override;
    void PerformRemoving(DAVA::Entity* entityToRemove) override;

protected:
    void Draw() override;
    void ProcessCommand(const RECommandNotificationObject& commandNotification) override;

    void DefineAddOrRemoveEdges(const DAVA::Vector<DAVA::PathComponent::Waypoint*>& srcPoints, DAVA::PathComponent::Waypoint* dstPoint,
                                DAVA::Vector<DAVA::PathComponent::Waypoint*>& toAddEdge, DAVA::Vector<DAVA::PathComponent::Waypoint*>& toRemoveEdge);
    void AddEdges(DAVA::PathComponent* path, const DAVA::Vector<DAVA::PathComponent::Waypoint*>& srcPoints, DAVA::PathComponent::Waypoint* nextWaypoint);
    void RemoveEdges(DAVA::PathComponent* path, const DAVA::Vector<DAVA::PathComponent::Waypoint*>& srcPoints, DAVA::PathComponent::Waypoint* nextWaypoint);

    void ResetSelection();
    void ProcessSelection(const SelectableGroup& selection);
    void UpdateSelectionMask();
    void FilterSelection(DAVA::PathComponent* path, const DAVA::Vector<DAVA::PathComponent::Waypoint*>& srcPoints, DAVA::Vector<DAVA::PathComponent::Waypoint*>& validPoints);

    bool AllowPerformSelectionHavingCurrent(const SelectableGroup& currentSelection) override;
    bool AllowChangeSelectionReplacingCurrent(const SelectableGroup& currentSelection, const SelectableGroup& newSelection) override;

    SceneEditor2* GetSceneEditor() const;

private:
    SelectableGroup currentSelection;
    DAVA::Vector<DAVA::PathComponent::Waypoint*> selectedWaypoints;
    DAVA::Vector<DAVA::PathComponent::Waypoint*> prevSelectedWaypoints;
    DAVA::Vector<DAVA::Entity*> waypointEntities;
    DAVA::PathComponent::Waypoint* underCursorPathEntity = nullptr;
    bool inCloneState = false;
    bool isEnabled = false;
};
