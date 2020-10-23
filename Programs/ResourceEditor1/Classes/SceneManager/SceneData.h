#pragma once

#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/Qt/Scene/System/CollisionSystem.h"
#include "Classes/Selection/SelectionSystem.h"

#include <TArc/DataProcessing/DataNode.h>
#include <TArc/DataProcessing/PropertiesHolder.h>
#include <TArc/DataProcessing/SettingsNode.h>

#include <Reflection/ReflectionRegistrator.h>
#include <Base/RefPtr.h>

#include <QString>

namespace DAVA
{
namespace TArc
{
class PropertiesItem;
} // namespace TArc
} // namespace DAVA

class SceneData : public DAVA::TArc::DataNode
{
public:
    // use this alias in constructions like Any::CanCast, Any::Cast, Any::CanGet and Any::Get
    using TSceneType = DAVA::RefPtr<SceneEditor2>;
    TSceneType GetScene();

    bool IsSceneChanged() const;
    DAVA::FilePath GetScenePath() const;
    DAVA::uint32 GetEnabledLandscapeTools() const;

    bool IsSavingAllowed(QString* message = nullptr) const;

    bool IsHUDVisible() const;

    static const char* scenePropertyName;
    static const char* sceneChangedPropertyName;
    static const char* scenePathPropertyName;
    static const char* sceneLandscapeToolsPropertyName;
    static const char* sceneHUDVisiblePropertyName;
    static const char* sceneCanUndoPropertyName;
    static const char* sceneUndoDescriptionPropertyName;
    static const char* sceneCanRedoPropertyName;
    static const char* sceneRedoDescriptionPropertyName;

    DAVA::TArc::PropertiesHolder* GetPropertiesRoot();

private:
    friend class SceneManagerModule;

    void CreatePropertiesRoot(DAVA::FileSystem* fs, const DAVA::FilePath& dirPath, const DAVA::FilePath& fileName);

    SceneEditor2* GetScenePtr() const;
    DAVA::RefPtr<SceneEditor2> scene;
    std::unique_ptr<DAVA::TArc::PropertiesHolder> propertiesRoot = nullptr;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SceneData, DAVA::TArc::DataNode)
    {
        DAVA::ReflectionRegistrator<SceneData>::Begin()
        .Field(scenePropertyName, &SceneData::scene)
        .Field(sceneChangedPropertyName, &SceneData::IsSceneChanged, nullptr)
        .Field(scenePathPropertyName, &SceneData::GetScenePath, nullptr)
        .Field(sceneLandscapeToolsPropertyName, &SceneData::GetEnabledLandscapeTools, nullptr)
        .Field("ScenePtr", &SceneData::GetScenePtr, nullptr)
        .Field(sceneHUDVisiblePropertyName, &SceneData::IsHUDVisible, nullptr)
        .Field(sceneCanUndoPropertyName, [](SceneData* data) { return data->scene->CanUndo(); }, nullptr)
        .Field(sceneUndoDescriptionPropertyName, [](SceneData* data) { return data->scene->GetUndoText(); }, nullptr)
        .Field(sceneCanRedoPropertyName, [](SceneData* data) { return data->scene->CanRedo(); }, nullptr)
        .Field(sceneRedoDescriptionPropertyName, [](SceneData* data) { return data->scene->GetRedoText(); }, nullptr)
        .End();
    }
};

class GlobalSceneSettings : public DAVA::TArc::SettingsNode
{
public:
    DAVA::float32 gridStep = 10.0f;
    DAVA::float32 gridSize = 600.0f;
    DAVA::float32 cameraSpeed0 = 35.0f;
    DAVA::float32 cameraSpeed1 = 100.0f;
    DAVA::float32 cameraSpeed2 = 250.0f;
    DAVA::float32 cameraSpeed3 = 400.0f;
    DAVA::float32 cameraFOV = 70.0f;
    DAVA::float32 cameraNear = 1.0f;
    DAVA::float32 cameraFar = 5000.0f;
    bool cameraUseDefaultSettings = true;
    DAVA::float32 heightOnLandscape = 2.0f;
    DAVA::float32 heightOnLandscapeStep = 0.5f;
    bool selectionSequent = false;
    bool selectionOnClick = false;
    SelectionSystemDrawMode selectionDrawMode = SelectionSystemDrawMode::SS_DRAW_DEFAULT;
    bool modificationByGizmoOnly = false;
    DAVA::float32 gizmoScale = 1.0f;
    DAVA::float32 debugBoxScale = 1.0f;
    DAVA::float32 debugBoxUserScale = 1.0f;
    DAVA::float32 debugBoxParticleScale = 1.0f;
    DAVA::float32 debugBoxWaypointScale = 1.0f;
    bool dragAndDropWithShift = false;
    bool autoSelectNewEntity = true;
    bool saveEmitters = false;
    bool saveStaticOcclusion = true;
    DAVA::uint32 defaultCustomColorIndex = 0;
    bool openLastScene = false;

    // Sound settings
    bool drawSoundObjects = false;
    DAVA::Color soundObjectBoxColor = DAVA::Color(0.0f, 0.8f, 0.4f, 0.2f);
    DAVA::Color soundObjectSphereColor = DAVA::Color(0.0f, 0.8f, 0.4f, 0.1f);

    // Scene grad settings
    DAVA::uint32 grabSizeWidth = 1280;
    DAVA::uint32 grabSizeHeight = 1024;

    DAVA_VIRTUAL_REFLECTION(GlobalSceneSettings, DAVA::TArc::SettingsNode);
};
