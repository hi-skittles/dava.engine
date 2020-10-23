#pragma once

#include <QObject>
#include <Classes/Commands2/Base/RECommandNotificationObject.h>
#include "UI/UIEvent.h"
#include "Scene3D/Scene.h"
#include "Base/StaticSingleton.h"

#include "Command/Command.h"

#include "Scene/System/ModifSystem.h"
#include "Scene/System/LandscapeEditorDrawSystem.h"
#include "Scene/System/HeightmapEditorSystem.h"
#include "Scene/System/TilemaskEditorSystem.h"
#include "Scene/System/CustomColorsSystem.h"
#include "Scene/System/RulerToolSystem.h"
#include "Scene/System/StructureSystem.h"
#include "Scene/System/EditorLightSystem.h"
#include "Scene/System/TextDrawSystem.h"
#include "Scene/System/BeastSystem.h"
#include "Scene/System/EditorMaterialSystem.h"
#include "Scene/System/WayEditSystem.h"
#include "Scene/System/PathSystem.h"

#include "Scene3D/Systems/StaticOcclusionBuildSystem.h"
#include "Scene3D/Systems/Controller/RotationControllerSystem.h"
#include "Scene3D/Systems/Controller/WASDControllerSystem.h"

#include "Utils/SceneExporter/SceneExporter.h"

#include "Commands2/Base/CommandNotify.h"
#include "Commands2/RECommandIDs.h"

class RECommandNotificationObject;
class SceneCameraSystem;
class SceneCollisionSystem;
class HoodSystem;
class EditorLODSystem;
class EditorStatisticsSystem;
class EditorVegetationSystem;
class EditorParticlesSystem;
class FogSettingsChangedReceiver;
class VisibilityCheckSystem;
class RECommandStack;
class EditorSceneSystem;
class EditorSlotSystem;

namespace DAVA
{
namespace TArc
{
class PropertiesHolder;
class ContextAccessor;
}
}

class SceneEditor2 : public DAVA::Scene
{
public:
    enum LandscapeTools : DAVA::uint32
    {
        LANDSCAPE_TOOL_CUSTOM_COLOR = 1 << 0,
        LANDSCAPE_TOOL_HEIGHTMAP_EDITOR = 1 << 1,
        LANDSCAPE_TOOL_TILEMAP_EDITOR = 1 << 2,
        LANDSCAPE_TOOL_RULER = 1 << 3,
        LANDSCAPE_TOOL_NOT_PASSABLE_TERRAIN = 1 << 4,

        LANDSCAPE_TOOLS_ALL = LANDSCAPE_TOOL_CUSTOM_COLOR | LANDSCAPE_TOOL_HEIGHTMAP_EDITOR | LANDSCAPE_TOOL_TILEMAP_EDITOR |
        LANDSCAPE_TOOL_RULER |
        LANDSCAPE_TOOL_NOT_PASSABLE_TERRAIN
    };

    SceneEditor2();
    ~SceneEditor2() override;

    // editor systems
    SceneCameraSystem* cameraSystem = nullptr;
    SceneCollisionSystem* collisionSystem = nullptr;
    HoodSystem* hoodSystem = nullptr;
    EntityModificationSystem* modifSystem = nullptr;
    LandscapeEditorDrawSystem* landscapeEditorDrawSystem = nullptr;
    HeightmapEditorSystem* heightmapEditorSystem = nullptr;
    TilemaskEditorSystem* tilemaskEditorSystem = nullptr;
    CustomColorsSystem* customColorsSystem = nullptr;
    RulerToolSystem* rulerToolSystem = nullptr;
    StructureSystem* structureSystem = nullptr;
    EditorParticlesSystem* particlesSystem = nullptr;
    EditorLightSystem* editorLightSystem = nullptr;
    TextDrawSystem* textDrawSystem = nullptr;
    BeastSystem* beastSystem = nullptr;
    DAVA::StaticOcclusionBuildSystem* staticOcclusionBuildSystem = nullptr;
    EditorMaterialSystem* materialSystem = nullptr;
    EditorLODSystem* editorLODSystem = nullptr;
    EditorStatisticsSystem* editorStatisticsSystem = nullptr;
    VisibilityCheckSystem* visibilityCheckSystem = nullptr;
    EditorVegetationSystem* editorVegetationSystem = nullptr;
    DAVA::WASDControllerSystem* wasdSystem = nullptr;
    DAVA::RotationControllerSystem* rotationSystem = nullptr;
    WayEditSystem* wayEditSystem = nullptr;
    PathSystem* pathSystem = nullptr;

    //to manage editor systems adding/deleting
    void AddSystem(DAVA::SceneSystem* sceneSystem,
                   const DAVA::ComponentMask& componentMask,
                   DAVA::uint32 processFlags = 0,
                   DAVA::SceneSystem* insertBeforeSceneForProcess = nullptr,
                   DAVA::SceneSystem* insertBeforeSceneForInput = nullptr,
                   DAVA::SceneSystem* insertBeforeSceneForFixedProcess = nullptr) override;

    void RemoveSystem(DAVA::SceneSystem* sceneSystem) override;

    template <typename T>
    T* LookupEditorSystem();

    bool AcquireInputLock(EditorSceneSystem* system);
    void ReleaseInputLock(EditorSceneSystem* system);

    // save/load
    DAVA::SceneFileV2::eError LoadScene(const DAVA::FilePath& path) override;
    DAVA::SceneFileV2::eError SaveScene(const DAVA::FilePath& pathname, bool saveForGame = false) override;
    DAVA::SceneFileV2::eError SaveScene();
    bool Export(const SceneExporter::Params& exportingParams);

    void SaveEmitters(const DAVA::Function<DAVA::FilePath(const DAVA::String& /*entityName*/, const DAVA::String& /*emitterName*/)>& getEmitterPathFn);

    const DAVA::FilePath& GetScenePath() const;
    void SetScenePath(const DAVA::FilePath& newScenePath);

    // commands
    bool CanUndo() const;
    bool CanRedo() const;

    DAVA::String GetUndoText() const;
    DAVA::String GetRedoText() const;

    void Undo();
    void Redo();

    void BeginBatch(const DAVA::String& text, DAVA::uint32 commandsCount = 1);
    void EndBatch();

    void Exec(std::unique_ptr<DAVA::Command>&& command);
    void RemoveCommands(DAVA::uint32 commandId);

    void ClearAllCommands();
    const RECommandStack* GetCommandStack() const;

    // checks whether the scene changed since the last save
    bool IsLoaded() const;
    bool IsChanged() const;
    void SetChanged();

    // enable/disable drawing custom HUD
    void SetHUDVisible(bool visible);
    bool IsHUDVisible() const;

    // DAVA events
    void Update(float timeElapsed) override;
    void Draw() override;

    // this function should be called each time UI3Dview changes its position
    // viewport rect is used to calc. ray from camera to any 2d point on this viewport
    void SetViewportRect(const DAVA::Rect& newViewportRect);

    //Insert entity to begin of scene hierarchy to display editor entities at one place on top og scene tree
    void AddEditorEntity(Entity* editorEntity);

    const DAVA::RenderStats& GetRenderStats() const;

    void EnableToolsInstantly(DAVA::int32 toolFlags);
    void DisableToolsInstantly(DAVA::int32 toolFlags, bool saveChanges = true);
    bool IsToolsEnabled(DAVA::int32 toolFlags);
    DAVA::int32 GetEnabledTools();

    SceneEditor2* CreateCopyForExport(); //Need to prevent changes of original scene
    DAVA::Entity* Clone(DAVA::Entity* dstNode /* = NULL */) override;

    void Activate() override;
    void Deactivate() override;

    void EnableEditorSystems();
    void LoadSystemsLocalProperties(DAVA::TArc::PropertiesHolder* holder, DAVA::TArc::ContextAccessor* accessor);
    void SaveSystemsLocalProperties(DAVA::TArc::PropertiesHolder* holder);

    DAVA::uint32 GetFramesCount() const;
    void ResetFramesCount();

    DAVA_DEPRECATED(void MarkAsChanged()); // for old material & particle editors

protected:
    bool isLoaded = false;
    bool isHUDVisible = true;

    DAVA::FilePath curScenePath;
    std::unique_ptr<RECommandStack> commandStack;
    DAVA::RenderStats renderStats;

    DAVA::Vector<EditorSceneSystem*> editorSystems;
    DAVA::Vector<EditorSceneSystem*> landscapeEditorSystems;
    EditorSceneSystem* inputLockedByThis = nullptr;
    DAVA::Vector<DAVA::Entity*> editorEntities;

    void AccumulateDependentCommands(REDependentCommandsHolder& holder);
    void EditorCommandProcess(const RECommandNotificationObject& commandNotification);

    void ExtractEditorEntities();
    void InjectEditorEntities();

    void RemoveSystems();

    void Setup3DDrawing();

    DAVA::uint32 framesCount = 0;

private:
    friend struct EditorCommandNotify;

    class EditorCommandNotify : public CommandNotify
    {
    public:
        EditorCommandNotify(SceneEditor2* _editor);

        void AccumulateDependentCommands(REDependentCommandsHolder& holder) override;
        void Notify(const RECommandNotificationObject& commandNotification) override;

    private:
        SceneEditor2* editor = nullptr;
    };

    DAVA_VIRTUAL_REFLECTION(SceneEditor2, DAVA::Scene);
};

template <typename T>
T* SceneEditor2::LookupEditorSystem()
{
    for (EditorSceneSystem* system : editorSystems)
    {
        if (dynamic_cast<T*>(system) != nullptr)
        {
            return static_cast<T*>(system);
        }
    }

    return nullptr;
}

Q_DECLARE_METATYPE(SceneEditor2*)

void LookAtSelection(SceneEditor2* scene);
void RemoveSelection(SceneEditor2* scene);
void LockTransform(SceneEditor2* scene);
void UnlockTransform(SceneEditor2* scene);
