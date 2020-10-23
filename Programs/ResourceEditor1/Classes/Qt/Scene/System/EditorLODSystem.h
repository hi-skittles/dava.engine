#pragma once

#include "Entity/SceneSystem.h"
#include "Scene3D/Lod/LodComponent.h"
#include "Commands2/CreatePlaneLODCommandHelper.h"
#include "Scene/SceneTypes.h"
#include "Classes/Qt/Scene/System/EditorSceneSystem.h"

namespace DAVA
{
class Entity;
class RenderObject;
}

class RECommandNotificationObject;
class SelectableGroup;

struct ForceValues
{
    enum eApplyFlag : DAVA::uint32
    {
        APPLY_DISTANCE = 1 << 0,
        APPLY_LAYER = 1 << 1,

        APPLY_NONE = 0,
        APPLY_ALL = APPLY_DISTANCE | APPLY_LAYER,

        APPLY_DEFAULT = APPLY_DISTANCE
    };

    ForceValues(DAVA::float32 distance_ = DAVA::LodComponent::INVALID_DISTANCE,
                DAVA::int32 layer_ = DAVA::LodComponent::INVALID_LOD_LAYER,
                eApplyFlag flag_ = APPLY_DEFAULT)
        : distance(distance_)
        , layer(layer_)
        , flag(flag_)
    {
    }

    DAVA::float32 distance;
    DAVA::int32 layer;
    eApplyFlag flag;
};

class SceneEditor2;
class EditorLODSystem;
class LODComponentHolder
{
    friend class EditorLODSystem;

public:
    LODComponentHolder();

    DAVA::int32 GetMaxLODLayer() const;
    DAVA::uint32 GetLODLayersCount() const;

    const DAVA::Vector<DAVA::float32>& GetDistances() const;
    const DAVA::Vector<bool>& GetMultiple() const;

protected:
    void BindToSystem(EditorLODSystem* system, SceneEditor2* scene);

    void SummarizeValues();
    void PropagateValues();

    void ApplyForce(const ForceValues& force);
    bool DeleteLOD(DAVA::int32 layer);
    bool CopyLod(DAVA::int32 from, DAVA::int32 to);

    DAVA::Vector<DAVA::LodComponent*> lodComponents;
    DAVA::Vector<DAVA::float32> distances;
    DAVA::Vector<bool> isMultiple;
    DAVA::Vector<bool> isChanged;

    DAVA::int32 maxLodLayerIndex = DAVA::LodComponent::INVALID_LOD_LAYER;

    EditorLODSystem* system = nullptr;
    SceneEditor2* scene = nullptr;
};

class EditorLODSystemUIDelegate;
class EditorLODSystem : public DAVA::SceneSystem, public EditorSceneSystem
{
    friend class SceneEditor2;

    enum eLODSystemFlag : DAVA::uint32
    {
        FLAG_MODE = 1 << 0,
        FLAG_FORCE = 1 << 1,
        FLAG_DISTANCE = 1 << 2,
        FLAG_ACTION = 1 << 3,

        FLAG_NONE = 0,
        FLAG_ALL = FLAG_MODE | FLAG_FORCE | FLAG_DISTANCE | FLAG_ACTION
    };

public:
    static const DAVA::int32 LAST_LOD_LAYER = 0x7fffffff;
    static const DAVA::float32 LOD_DISTANCE_INFINITY;

    EditorLODSystem(DAVA::Scene* scene);
    ~EditorLODSystem() override;

    void AddEntity(DAVA::Entity* entity) override;
    void RemoveEntity(DAVA::Entity* entity) override;
    void AddComponent(DAVA::Entity* entity, DAVA::Component* component) override;
    void RemoveComponent(DAVA::Entity* entity, DAVA::Component* component) override;
    void PrepareForRemove() override;

    void Process(DAVA::float32 timeElapsed) override;

    void SceneDidLoaded() override;

    eEditorMode GetMode() const;
    void SetMode(eEditorMode mode);

    bool GetRecursive() const;
    void SetRecursive(bool recursive);

    //actions
    bool CanDeleteLOD() const;
    bool CanCreateLOD() const;

    void DeleteLOD(DAVA::int32 layer);

    void CreatePlaneLOD(DAVA::int32 fromLayer, DAVA::uint32 textureSize, const DAVA::FilePath& texturePath);
    void CopyLastLODToFirst();
    //end of actions

    const ForceValues& GetForceValues() const;
    void SetForceValues(const ForceValues& values);

    const LODComponentHolder* GetActiveLODData() const;

    void SetLODDistances(const DAVA::Vector<DAVA::float32>& distances);

    //scene signals
    void SelectionChanged(const SelectableGroup& selection);

    void AddDelegate(EditorLODSystemUIDelegate* uiDelegate);
    void RemoveDelegate(EditorLODSystemUIDelegate* uiDelegate);

    DAVA::FilePath GetPathForPlaneEntity() const;

    static bool IsFitModeEnabled(const DAVA::Vector<DAVA::float32>& distances);

protected:
    void ProcessCommand(const RECommandNotificationObject& commandNotification) override;

private:
    void RecalculateData();
    //actions
    void CopyLOD(DAVA::int32 fromLayer, DAVA::int32 toLayer);

    //signals
    void EmitInvalidateUI(DAVA::uint32 flags);
    void DispatchSignals();
    //signals

    void ProcessAddedEntities();
    void ProcessPlaneLODs();

    LODComponentHolder lodData[eEditorMode::MODE_COUNT];
    LODComponentHolder* activeLodData = nullptr;
    ForceValues forceValues;
    eEditorMode mode = eEditorMode::MODE_DEFAULT;

    DAVA::Vector<CreatePlaneLODCommandHelper::RequestPointer> planeLODRequests;

    bool generateCommands = false;

    DAVA::Vector<EditorLODSystemUIDelegate*> uiDelegates;
    DAVA::uint32 invalidateUIFlag = FLAG_NONE;

    DAVA::Vector<std::pair<DAVA::Entity*, DAVA::LodComponent*>> componentsToAdd;

    bool recursive = false;
    DAVA::Array<bool, eEditorMode::MODE_COUNT> pendingSummarizeValues;
};

class EditorLODSystemUIDelegate
{
public:
    virtual ~EditorLODSystemUIDelegate() = default;

    virtual void UpdateModeUI(EditorLODSystem* forSystem, const eEditorMode mode, bool recursive)
    {
    }
    virtual void UpdateForceUI(EditorLODSystem* forSystem, const ForceValues& forceValues)
    {
    }
    virtual void UpdateDistanceUI(EditorLODSystem* forSystem, const LODComponentHolder* lodData)
    {
    }
    virtual void UpdateActionUI(EditorLODSystem* forSystem)
    {
    }
};
