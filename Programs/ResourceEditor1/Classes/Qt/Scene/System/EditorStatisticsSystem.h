#pragma once

#include "Classes/Qt/Scene/System/EditorSceneSystem.h"
#include "Classes/Qt/Scene/SceneTypes.h"

#include <TArc/Core/FieldBinder.h>
#include <TArc/DataProcessing/SettingsNode.h>

#include <Entity/SceneSystem.h>
#include <Reflection/Reflection.h>

namespace DAVA
{
class Entity;
class Camera;
class RenderComponent;
class RenderObject;
}

class RECommandNotificationObject;
class EditorStatisticsSystemUIDelegate;
struct TrianglesData;

class RenderStatsSettings : public DAVA::TArc::SettingsNode
{
public:
    bool calculatePerFrame = true;

    DAVA_VIRTUAL_REFLECTION(RenderStatsSettings, DAVA::TArc::SettingsNode);
};

class EditorStatisticsSystem : public DAVA::SceneSystem, public EditorSceneSystem
{
    enum eStatisticsSystemFlag : DAVA::uint32
    {
        FLAG_TRIANGLES = 1 << 0,

        FLAG_NONE = 0
    };

public:
    static const DAVA::int32 INDEX_OF_ALL_LODS_TRIANGLES = 0;
    static const DAVA::int32 INDEX_OF_FIRST_LOD_TRIANGLES = 1;

    EditorStatisticsSystem(DAVA::Scene* scene);

    void RegisterEntity(DAVA::Entity* entity) override;
    void UnregisterEntity(DAVA::Entity* entity) override;

    void RegisterComponent(DAVA::Entity* entity, DAVA::Component* component) override;
    void UnregisterComponent(DAVA::Entity* entity, DAVA::Component* component) override;
    void PrepareForRemove() override;

    void Process(DAVA::float32 timeElapsed) override;

    const DAVA::Vector<DAVA::uint32>& GetTriangles(eEditorMode mode, bool allTriangles);

    void AddDelegate(EditorStatisticsSystemUIDelegate* uiDelegate);
    void RemoveDelegate(EditorStatisticsSystemUIDelegate* uiDelegate);

private:
    void CalculateTriangles();
    void ClipSelection(DAVA::Camera* camera, DAVA::Vector<DAVA::RenderObject*>& selection,
                       DAVA::Vector<DAVA::RenderObject*>& visibilityArray, DAVA::uint32 visibilityCriteria);

    //signals
    void EmitInvalidateUI(DAVA::uint32 flags);
    void DispatchSignals();
    //signals

    void ProcessCommand(const RECommandNotificationObject& commandNotification) override;

private:
    DAVA::Vector<TrianglesData> triangles;

    DAVA::Vector<EditorStatisticsSystemUIDelegate*> uiDelegates;
    DAVA::uint32 invalidateUIflag = FLAG_NONE;
    std::unique_ptr<DAVA::TArc::FieldBinder> binder;
    bool calculatePerFrame = true;
    bool initialized = false;
};

class EditorStatisticsSystemUIDelegate
{
public:
    virtual ~EditorStatisticsSystemUIDelegate() = default;

    virtual void UpdateTrianglesUI(EditorStatisticsSystem* forSystem){};
};
