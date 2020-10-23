#include "Classes/Project/ProjectManagerData.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Selection/Selection.h"
#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/Qt/Scene/SceneSignals.h"
#include "Classes/Qt/Scene/System/EditorParticlesSystem.h"

#include "Classes/Deprecated/SceneValidator.h"
#include "Classes/Commands2/Base/RECommandStack.h"
#include "Classes/Commands2/Base/RECommandNotificationObject.h"
#include "Classes/Commands2/CustomColorsCommands2.h"
#include "Classes/Commands2/HeightmapEditorCommands2.h"
#include "Classes/Commands2/TilemaskEditorCommands.h"
#include "Classes/Commands2/LandscapeToolsToggleCommand.h"
#include "Classes/Utils/SceneExporter/SceneExporter.h"

#include "Classes/Qt/Scene/System/GridSystem.h"
#include "Classes/Qt/Scene/System/CameraSystem.h"
#include "Classes/Qt/Scene/System/CollisionSystem.h"
#include "Classes/Qt/Scene/System/HoodSystem.h"
#include "Classes/Qt/Scene/System/EditorLODSystem.h"
#include "Classes/Qt/Scene/System/EditorStatisticsSystem.h"
#include "Classes/Qt/Scene/System/VisibilityCheckSystem/VisibilityCheckSystem.h"
#include "Classes/Qt/Scene/System/EditorVegetationSystem.h"
#include "Classes/Qt/Scene/System/EditorSceneSystem.h"

#include <QtTools/ConsoleWidget/PointerSerializer.h>
#include <TArc/Utils/RenderContextGuard.h>

// framework
#include <Debug/DVAssert.h>
#include <Engine/Engine.h>
#include <Entity/ComponentUtils.h>
#include <Scene3D/Entity.h>
#include <Scene3D/SceneFileV2.h>
#include <Scene3D/Systems/RenderUpdateSystem.h>
#include <Scene3D/Systems/StaticOcclusionSystem.h>
#include <Scene3D/Systems/Controller/SnapToLandscapeControllerSystem.h>
#include <Scene3D/Components/VisibilityCheckComponent.h>
#include <Scene3D/Components/Waypoint/WaypointComponent.h>
#include <Render/Highlevel/RenderBatchArray.h>
#include <Render/Highlevel/RenderPass.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Command/Command.h>

namespace SceneEditorDetail
{
struct EmitterDescriptor
{
    EmitterDescriptor(DAVA::ParticleEmitter* emitter_, DAVA::ParticleLayer* layer, DAVA::FilePath path, DAVA::String name)
        : emitter(emitter_)
        , ownerLayer(layer)
        , yamlPath(path)
        , entityName(name)
    {
    }

    DAVA::ParticleEmitter* emitter = nullptr;
    DAVA::ParticleLayer* ownerLayer = nullptr;
    DAVA::FilePath yamlPath;
    DAVA::String entityName;
};

void CollectEmittersForSave(DAVA::ParticleEmitter* topLevelEmitter, DAVA::List<EmitterDescriptor>& emitters, const DAVA::String& entityName)
{
    DVASSERT(topLevelEmitter != nullptr);

    for (auto& layer : topLevelEmitter->layers)
    {
        if (nullptr != layer->innerEmitter)
        {
            CollectEmittersForSave(layer->innerEmitter->GetEmitter(), emitters, entityName);
            emitters.emplace_back(EmitterDescriptor(layer->innerEmitter->GetEmitter(), layer, layer->innerEmitter->GetEmitter()->configPath, entityName));
        }
    }

    emitters.emplace_back(topLevelEmitter, nullptr, topLevelEmitter->configPath, entityName);
}
}

SceneEditor2::SceneEditor2()
    : Scene()
    , commandStack(new RECommandStack())
{
    DVASSERT(DAVA::Engine::Instance()->IsConsoleMode() == false);

    DAVA::ComponentMask emptyMask;

    EditorCommandNotify* notify = new EditorCommandNotify(this);
    commandStack->SetNotify(notify);
    SafeRelease(notify);

    DAVA::SceneSystem* gridSystem = new SceneGridSystem(this);
    AddSystem(gridSystem, emptyMask, SCENE_SYSTEM_REQUIRE_PROCESS, renderUpdateSystem);

    cameraSystem = new SceneCameraSystem(this);
    AddSystem(cameraSystem, DAVA::ComponentUtils::MakeMask<DAVA::CameraComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS | SCENE_SYSTEM_REQUIRE_INPUT, transformSystem);

    rotationSystem = new DAVA::RotationControllerSystem(this);
    AddSystem(rotationSystem, DAVA::ComponentUtils::MakeMask<DAVA::CameraComponent>() | DAVA::ComponentUtils::MakeMask<DAVA::RotationControllerComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS | SCENE_SYSTEM_REQUIRE_INPUT);

    DAVA::SceneSystem* snapToLandscapeSystem = new DAVA::SnapToLandscapeControllerSystem(this);
    AddSystem(snapToLandscapeSystem, DAVA::ComponentUtils::MakeMask<DAVA::CameraComponent>() | DAVA::ComponentUtils::MakeMask<DAVA::SnapToLandscapeControllerComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);

    wasdSystem = new DAVA::WASDControllerSystem(this);
    AddSystem(wasdSystem, DAVA::ComponentUtils::MakeMask<DAVA::CameraComponent>() | DAVA::ComponentUtils::MakeMask<DAVA::WASDControllerComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);

    collisionSystem = new SceneCollisionSystem(this);
    AddSystem(collisionSystem, emptyMask, SCENE_SYSTEM_REQUIRE_PROCESS | SCENE_SYSTEM_REQUIRE_INPUT, renderUpdateSystem);

    hoodSystem = new HoodSystem(this, cameraSystem);
    AddSystem(hoodSystem, emptyMask, SCENE_SYSTEM_REQUIRE_PROCESS | SCENE_SYSTEM_REQUIRE_INPUT, renderUpdateSystem);

    modifSystem = new EntityModificationSystem(this, collisionSystem, cameraSystem, hoodSystem);
    AddSystem(modifSystem, emptyMask, SCENE_SYSTEM_REQUIRE_INPUT, renderUpdateSystem);

    landscapeEditorDrawSystem = new LandscapeEditorDrawSystem(this);
    AddSystem(landscapeEditorDrawSystem, emptyMask, SCENE_SYSTEM_REQUIRE_PROCESS, renderUpdateSystem);
    landscapeEditorDrawSystem->EnableSystem();

    heightmapEditorSystem = new HeightmapEditorSystem(this);
    AddSystem(heightmapEditorSystem, emptyMask, SCENE_SYSTEM_REQUIRE_PROCESS | SCENE_SYSTEM_REQUIRE_INPUT, renderUpdateSystem);

    tilemaskEditorSystem = new TilemaskEditorSystem(this);
    AddSystem(tilemaskEditorSystem, emptyMask, SCENE_SYSTEM_REQUIRE_PROCESS | SCENE_SYSTEM_REQUIRE_INPUT, renderUpdateSystem);

    customColorsSystem = new CustomColorsSystem(this);
    AddSystem(customColorsSystem, emptyMask, SCENE_SYSTEM_REQUIRE_PROCESS | SCENE_SYSTEM_REQUIRE_INPUT, renderUpdateSystem);

    rulerToolSystem = new RulerToolSystem(this);
    AddSystem(rulerToolSystem, emptyMask, SCENE_SYSTEM_REQUIRE_PROCESS | SCENE_SYSTEM_REQUIRE_INPUT, renderUpdateSystem);

    structureSystem = new StructureSystem(this);
    AddSystem(structureSystem, emptyMask, SCENE_SYSTEM_REQUIRE_PROCESS, renderUpdateSystem);

    particlesSystem = new EditorParticlesSystem(this);
    AddSystem(particlesSystem, DAVA::ComponentUtils::MakeMask<DAVA::ParticleEffectComponent>(), 0, renderUpdateSystem);

    textDrawSystem = new TextDrawSystem(this, cameraSystem);
    AddSystem(textDrawSystem, emptyMask, 0, renderUpdateSystem);

    editorLightSystem = new EditorLightSystem(this);
    AddSystem(editorLightSystem, DAVA::ComponentUtils::MakeMask<DAVA::LightComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS, transformSystem);

    beastSystem = new BeastSystem(this);
    AddSystem(beastSystem, emptyMask);

    staticOcclusionBuildSystem = new DAVA::StaticOcclusionBuildSystem(this);
    AddSystem(staticOcclusionBuildSystem, DAVA::ComponentUtils::MakeMask<DAVA::StaticOcclusionComponent>() | DAVA::ComponentUtils::MakeMask<DAVA::TransformComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS, renderUpdateSystem);

    materialSystem = new EditorMaterialSystem(this);
    AddSystem(materialSystem, DAVA::ComponentUtils::MakeMask<DAVA::RenderComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS, renderUpdateSystem);

    wayEditSystem = new WayEditSystem(this);
    AddSystem(wayEditSystem, DAVA::ComponentUtils::MakeMask<DAVA::WaypointComponent>() | DAVA::ComponentUtils::MakeMask<DAVA::TransformComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS | SCENE_SYSTEM_REQUIRE_INPUT);
    structureSystem->AddDelegate(wayEditSystem);

    pathSystem = new PathSystem(this);
    AddSystem(pathSystem, DAVA::ComponentUtils::MakeMask<DAVA::PathComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);
    modifSystem->AddDelegate(pathSystem);
    modifSystem->AddDelegate(wayEditSystem);

    editorLODSystem = new EditorLODSystem(this);
    AddSystem(editorLODSystem, DAVA::ComponentUtils::MakeMask<DAVA::LodComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);

    editorStatisticsSystem = new EditorStatisticsSystem(this);
    AddSystem(editorStatisticsSystem, emptyMask, SCENE_SYSTEM_REQUIRE_PROCESS);

    visibilityCheckSystem = new VisibilityCheckSystem(this);
    AddSystem(visibilityCheckSystem, DAVA::ComponentUtils::MakeMask<DAVA::VisibilityCheckComponent>(), SCENE_SYSTEM_REQUIRE_PROCESS);

    editorVegetationSystem = new EditorVegetationSystem(this);
    AddSystem(editorVegetationSystem, DAVA::ComponentUtils::MakeMask<DAVA::RenderComponent>(), 0);

    SceneSignals::Instance()->EmitOpened(this);
}

SceneEditor2::~SceneEditor2()
{
    DAVA::TArc::RenderContextGuard guard;
    commandStack.reset();
    RemoveSystems();

    SceneSignals::Instance()->EmitClosed(this);
}

DAVA::SceneFileV2::eError SceneEditor2::LoadScene(const DAVA::FilePath& path)
{
    DAVA::TArc::RenderContextGuard guard;
    DAVA::SceneFileV2::eError ret = Scene::LoadScene(path);
    if (ret == DAVA::SceneFileV2::ERROR_NO_ERROR)
    {
        for (DAVA::int32 i = 0, e = GetScene()->GetChildrenCount(); i < e; ++i)
        {
            structureSystem->CheckAndMarkSolid(GetScene()->GetChild(i));
        }
        curScenePath = path;
        isLoaded = true;
    }

    SceneValidator::ExtractEmptyRenderObjects(this);

    SceneValidator validator;
    ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
    if (data)
    {
        validator.SetPathForChecking(data->GetProjectPath());
    }
    validator.ValidateScene(this, path);

    SceneSignals::Instance()->EmitLoaded(this);

    return ret;
}

DAVA::SceneFileV2::eError SceneEditor2::SaveScene(const DAVA::FilePath& path, bool saveForGame /*= false*/)
{
    DAVA::TArc::RenderContextGuard guard;
    bool cameraLightState = false;
    if (editorLightSystem != nullptr)
    {
        cameraLightState = editorLightSystem->GetCameraLightEnabled();
        editorLightSystem->SetCameraLightEnabled(false);
    }

    DAVA::Vector<std::unique_ptr<DAVA::Command>> prepareForSaveCommands;
    prepareForSaveCommands.reserve(editorSystems.size());
    for (EditorSceneSystem* editorSceneSystem : editorSystems)
    {
        std::unique_ptr<DAVA::Command> cmd = editorSceneSystem->PrepareForSave(saveForGame);
        if (cmd != nullptr)
        {
            prepareForSaveCommands.push_back(std::move(cmd));
        }
    }

    std::for_each(prepareForSaveCommands.begin(), prepareForSaveCommands.end(), [](std::unique_ptr<DAVA::Command>& cmd)
                  {
                      cmd->Redo();
                  });

    ExtractEditorEntities();

    DAVA::ScopedPtr<DAVA::Texture> tilemaskTexture(nullptr);
    bool needToRestoreTilemask = false;
    if (landscapeEditorDrawSystem)
    { //dirty magic to work with new saving of materials and FBO landscape texture
        tilemaskTexture = SafeRetain(landscapeEditorDrawSystem->GetTileMaskTexture());

        needToRestoreTilemask = landscapeEditorDrawSystem->SaveTileMaskTexture();
        landscapeEditorDrawSystem->ResetTileMaskTexture();
    }

    DAVA::SceneFileV2::eError err = Scene::SaveScene(path, saveForGame);
    if (DAVA::SceneFileV2::ERROR_NO_ERROR == err)
    {
        curScenePath = path;
        isLoaded = true;

        // mark current position in command stack as clean
        commandStack->SetClean();
    }

    if (needToRestoreTilemask)
    {
        landscapeEditorDrawSystem->SetTileMaskTexture(tilemaskTexture);
    }

    std::for_each(prepareForSaveCommands.rbegin(), prepareForSaveCommands.rend(), [](std::unique_ptr<DAVA::Command>& cmd)
                  {
                      cmd->Undo();
                  });

    InjectEditorEntities();

    if (editorLightSystem != nullptr)
    {
        editorLightSystem->SetCameraLightEnabled(cameraLightState);
    }

    SceneSignals::Instance()->EmitSaved(this);

    return err;
}

void SceneEditor2::AddSystem(DAVA::SceneSystem* sceneSystem, const DAVA::ComponentMask& componentMask, DAVA::uint32 processFlags, DAVA::SceneSystem* insertBeforeSceneForProcess, DAVA::SceneSystem* insertBeforeSceneForInput, DAVA::SceneSystem* insertBeforeSceneForFixedProcess)
{
    Scene::AddSystem(sceneSystem, componentMask, processFlags, insertBeforeSceneForProcess, insertBeforeSceneForInput);

    EditorSceneSystem* editorSystem = dynamic_cast<EditorSceneSystem*>(sceneSystem);
    if (editorSystem != nullptr)
    {
        editorSystems.push_back(editorSystem);
        if (dynamic_cast<LandscapeEditorSystem*>(sceneSystem) != nullptr)
        {
            landscapeEditorSystems.push_back(editorSystem);
        }
    }
}

void SceneEditor2::RemoveSystem(DAVA::SceneSystem* sceneSystem)
{
    EditorSceneSystem* editorSystem = dynamic_cast<EditorSceneSystem*>(sceneSystem);
    if (editorSystem != nullptr)
    {
        DAVA::FindAndRemoveExchangingWithLast(editorSystems, editorSystem);
        if (dynamic_cast<LandscapeEditorSystem*>(sceneSystem) != nullptr)
        {
            DAVA::FindAndRemoveExchangingWithLast(landscapeEditorSystems, editorSystem);
        }
    }

    Scene::RemoveSystem(sceneSystem);
}

bool SceneEditor2::AcquireInputLock(EditorSceneSystem* system)
{
    if (inputLockedByThis != nullptr && inputLockedByThis != system)
    {
        return false;
    }

    inputLockedByThis = system;
    return true;
}

void SceneEditor2::ReleaseInputLock(EditorSceneSystem* system)
{
    if (inputLockedByThis == system)
    {
        inputLockedByThis = nullptr;
    }
}

void SceneEditor2::ExtractEditorEntities()
{
    DVASSERT(editorEntities.size() == 0);

    DAVA::Vector<DAVA::Entity*> allEntities;
    GetChildNodes(allEntities);

    DAVA::size_type count = allEntities.size();
    for (DAVA::size_type i = 0; i < count; ++i)
    {
        if (allEntities[i]->GetName().find("editor.") != DAVA::String::npos)
        {
            allEntities[i]->Retain();
            editorEntities.push_back(allEntities[i]);

            allEntities[i]->GetParent()->RemoveNode(allEntities[i]);
        }
    }
}

void SceneEditor2::InjectEditorEntities()
{
    for (DAVA::int32 i = static_cast<DAVA::int32>(editorEntities.size()) - 1; i >= 0; i--)
    {
        AddEditorEntity(editorEntities[i]);
        editorEntities[i]->Release();
    }
    editorEntities.clear();
}

DAVA::SceneFileV2::eError SceneEditor2::SaveScene()
{
    return SaveScene(curScenePath);
}

bool SceneEditor2::Export(const SceneExporter::Params& exportingParams)
{
    DAVA::ScopedPtr<SceneEditor2> clonedScene(CreateCopyForExport());
    if (clonedScene)
    {
        SceneExporter exporter;
        exporter.SetExportingParams(exportingParams);

        const DAVA::FilePath& scenePathname = GetScenePath();
        DAVA::FilePath newScenePathname = exportingParams.outputs[0].dataFolder + scenePathname.GetRelativePathname(exportingParams.dataSourceFolder);
        DAVA::FileSystem::Instance()->CreateDirectory(newScenePathname.GetDirectory(), true);

        DAVA::Vector<SceneExporter::ExportedObjectCollection> exportedObjects;
        exportedObjects.resize(SceneExporter::OBJECT_COUNT);
        bool sceneExported = exporter.ExportScene(clonedScene, scenePathname, newScenePathname, exportedObjects);

        bool objectExported = true;
        for (const SceneExporter::ExportedObjectCollection& collection : exportedObjects)
        {
            if (collection.empty() == false)
            {
                objectExported = exporter.ExportObjects(collection) && objectExported;
            }
        }

        return (sceneExported && objectExported);
    }

    return false;
}

void SceneEditor2::SaveEmitters(const DAVA::Function<DAVA::FilePath(const DAVA::String&, const DAVA::String&)>& getEmitterPathFn)
{
    DAVA::List<DAVA::Entity*> effectEntities;
    GetChildEntitiesWithComponent(effectEntities, DAVA::Type::Instance<DAVA::ParticleEffectComponent>());
    if (effectEntities.empty())
    {
        return;
    }

    DAVA::List<SceneEditorDetail::EmitterDescriptor> emittersForSave;
    for (DAVA::Entity* entityWithEffect : effectEntities)
    {
        const DAVA::String entityName = entityWithEffect->GetName().c_str();
        DAVA::ParticleEffectComponent* effect = GetEffectComponent(entityWithEffect);
        for (DAVA::int32 i = 0, sz = effect->GetEmittersCount(); i < sz; ++i)
        {
            SceneEditorDetail::CollectEmittersForSave(effect->GetEmitterInstance(i)->GetEmitter(), emittersForSave, entityName);
        }
    }

    for (SceneEditorDetail::EmitterDescriptor& descriptor : emittersForSave)
    {
        DAVA::ParticleEmitter* emitter = descriptor.emitter;
        const DAVA::String& entityName = descriptor.entityName;

        DAVA::FilePath yamlPathForSaving = descriptor.yamlPath;
        if (yamlPathForSaving.IsEmpty())
        {
            yamlPathForSaving = getEmitterPathFn(entityName, emitter->name.c_str());
        }

        if (!yamlPathForSaving.IsEmpty())
        {
            if (nullptr != descriptor.ownerLayer)
            {
                descriptor.ownerLayer->innerEmitterPath = yamlPathForSaving;
            }
            emitter->SaveToYaml(yamlPathForSaving);
        }
    }
}

const DAVA::FilePath& SceneEditor2::GetScenePath() const
{
    return curScenePath;
}

void SceneEditor2::SetScenePath(const DAVA::FilePath& newScenePath)
{
    curScenePath = newScenePath;
}

bool SceneEditor2::CanUndo() const
{
    return commandStack->CanUndo();
}

bool SceneEditor2::CanRedo() const
{
    return commandStack->CanRedo();
}

DAVA::String SceneEditor2::GetUndoText() const
{
    const DAVA::Command* undoCommand = commandStack->GetUndoCommand();
    if (undoCommand != nullptr)
    {
        return undoCommand->GetDescription();
    }
    return DAVA::String();
}

DAVA::String SceneEditor2::GetRedoText() const
{
    const DAVA::Command* redoCommand = commandStack->GetRedoCommand();
    if (redoCommand != nullptr)
    {
        return redoCommand->GetDescription();
    }
    return DAVA::String();
}

void SceneEditor2::Undo()
{
    if (commandStack->CanUndo())
    {
        commandStack->Undo();
    }
}

void SceneEditor2::Redo()
{
    if (commandStack->CanRedo())
    {
        commandStack->Redo();
    }
}

void SceneEditor2::BeginBatch(const DAVA::String& text, DAVA::uint32 commandsCount /*= 1*/)
{
    commandStack->BeginBatch(text, commandsCount);
}

void SceneEditor2::EndBatch()
{
    commandStack->EndBatch();
}

void SceneEditor2::Exec(std::unique_ptr<DAVA::Command>&& command)
{
    if (command)
    {
        commandStack->Exec(std::move(command));
    }
}

void SceneEditor2::RemoveCommands(DAVA::uint32 commandId)
{
    commandStack->RemoveCommands(commandId);
}

void SceneEditor2::ClearAllCommands()
{
    commandStack->Clear();
}

const RECommandStack* SceneEditor2::GetCommandStack() const
{
    return commandStack.get();
}

bool SceneEditor2::IsLoaded() const
{
    return isLoaded;
}

void SceneEditor2::SetHUDVisible(bool visible)
{
    isHUDVisible = visible;
    hoodSystem->LockAxis(!visible);
}

bool SceneEditor2::IsHUDVisible() const
{
    return isHUDVisible;
}

bool SceneEditor2::IsChanged() const
{
    return !commandStack->IsClean();
}

void SceneEditor2::SetChanged()
{
    commandStack->SetChanged();
}

void SceneEditor2::Update(float timeElapsed)
{
    ++framesCount;

    Scene::Update(timeElapsed);

    renderStats = DAVA::Renderer::GetRenderStats();
}

void SceneEditor2::SetViewportRect(const DAVA::Rect& newViewportRect)
{
    cameraSystem->SetViewportRect(newViewportRect);
}

void SceneEditor2::Draw()
{
    Scene::Draw();

    if (isHUDVisible)
    {
        for (EditorSceneSystem* system : editorSystems)
        {
            system->Draw();
        }
    }
    else
    {
        for (EditorSceneSystem* system : landscapeEditorSystems)
        {
            system->Draw();
        }
    }
}

void SceneEditor2::AccumulateDependentCommands(REDependentCommandsHolder& holder)
{
    if (holder.GetMasterCommandInfo().IsEmpty())
    {
        return;
    }

    for (EditorSceneSystem* system : editorSystems)
    {
        system->AccumulateDependentCommands(holder);
    }
}

void SceneEditor2::EditorCommandProcess(const RECommandNotificationObject& commandNotification)
{
    if (commandNotification.IsEmpty())
    {
        return;
    }

    for (EditorSceneSystem* system : editorSystems)
    {
        system->ProcessCommand(commandNotification);
    }
}

void SceneEditor2::AddEditorEntity(Entity* editorEntity)
{
    if (GetChildrenCount())
    {
        InsertBeforeNode(editorEntity, GetChild(0));
    }
    else
    {
        AddNode(editorEntity);
    }
}

SceneEditor2::EditorCommandNotify::EditorCommandNotify(SceneEditor2* _editor)
    : editor(_editor)
{
}

void SceneEditor2::EditorCommandNotify::AccumulateDependentCommands(REDependentCommandsHolder& holder)
{
    if (nullptr != editor)
    {
        editor->AccumulateDependentCommands(holder);
    }
}

void SceneEditor2::EditorCommandNotify::Notify(const RECommandNotificationObject& commandNotification)
{
    if (nullptr != editor)
    {
        editor->EditorCommandProcess(commandNotification);
        SceneSignals::Instance()->EmitCommandExecuted(editor, commandNotification);
    }
}

const DAVA::RenderStats& SceneEditor2::GetRenderStats() const
{
    return renderStats;
}

void SceneEditor2::EnableToolsInstantly(DAVA::int32 toolFlags)
{
    if (toolFlags & LANDSCAPE_TOOL_CUSTOM_COLOR)
    {
        EnableCustomColorsCommand(this, true).Redo();
    }

    if (toolFlags & LANDSCAPE_TOOL_HEIGHTMAP_EDITOR)
    {
        EnableHeightmapEditorCommand(this).Redo();
    }

    if (toolFlags & LANDSCAPE_TOOL_TILEMAP_EDITOR)
    {
        EnableTilemaskEditorCommand(this).Redo();
    }

    if (toolFlags & LANDSCAPE_TOOL_RULER)
    {
        EnableRulerToolCommand(this).Redo();
    }

    if (toolFlags & LANDSCAPE_TOOL_NOT_PASSABLE_TERRAIN)
    {
        EnableNotPassableCommand(this).Redo();
    }
}

void SceneEditor2::DisableToolsInstantly(DAVA::int32 toolFlags, bool saveChanges /*= true*/)
{
    if (toolFlags & LANDSCAPE_TOOL_CUSTOM_COLOR)
    {
        EnableCustomColorsCommand(this, saveChanges).Undo();
    }

    if (toolFlags & LANDSCAPE_TOOL_HEIGHTMAP_EDITOR)
    {
        EnableHeightmapEditorCommand(this).Undo();
    }

    if (toolFlags & LANDSCAPE_TOOL_TILEMAP_EDITOR)
    {
        EnableTilemaskEditorCommand(this).Undo();
    }

    if (toolFlags & LANDSCAPE_TOOL_RULER)
    {
        EnableRulerToolCommand(this).Undo();
    }

    if (toolFlags & LANDSCAPE_TOOL_NOT_PASSABLE_TERRAIN)
    {
        EnableNotPassableCommand(this).Undo();
    }
}

bool SceneEditor2::IsToolsEnabled(DAVA::int32 toolFlags)
{
    bool res = false;

    if (toolFlags & LANDSCAPE_TOOL_CUSTOM_COLOR)
    {
        res |= customColorsSystem->IsLandscapeEditingEnabled();
    }

    if (toolFlags & LANDSCAPE_TOOL_HEIGHTMAP_EDITOR)
    {
        res |= heightmapEditorSystem->IsLandscapeEditingEnabled();
    }

    if (toolFlags & LANDSCAPE_TOOL_TILEMAP_EDITOR)
    {
        res |= tilemaskEditorSystem->IsLandscapeEditingEnabled();
    }

    if (toolFlags & LANDSCAPE_TOOL_RULER)
    {
        res |= rulerToolSystem->IsLandscapeEditingEnabled();
    }

    if (toolFlags & LANDSCAPE_TOOL_NOT_PASSABLE_TERRAIN)
    {
        res |= landscapeEditorDrawSystem->IsNotPassableTerrainEnabled();
    }

    return res;
}

DAVA::int32 SceneEditor2::GetEnabledTools()
{
    DAVA::int32 toolFlags = 0;

    if (customColorsSystem->IsLandscapeEditingEnabled())
    {
        toolFlags |= LANDSCAPE_TOOL_CUSTOM_COLOR;
    }

    if (heightmapEditorSystem->IsLandscapeEditingEnabled())
    {
        toolFlags |= LANDSCAPE_TOOL_HEIGHTMAP_EDITOR;
    }

    if (tilemaskEditorSystem->IsLandscapeEditingEnabled())
    {
        toolFlags |= LANDSCAPE_TOOL_TILEMAP_EDITOR;
    }

    if (rulerToolSystem->IsLandscapeEditingEnabled())
    {
        toolFlags |= LANDSCAPE_TOOL_RULER;
    }

    if (landscapeEditorDrawSystem->IsNotPassableTerrainEnabled())
    {
        toolFlags |= LANDSCAPE_TOOL_NOT_PASSABLE_TERRAIN;
    }

    return toolFlags;
}

DAVA::Entity* SceneEditor2::Clone(Entity* dstNode /*= NULL*/)
{
    if (!dstNode)
    {
        DVASSERT(DAVA::IsPointerToExactClass<SceneEditor2>(this), "Can clone only SceneEditor2");
        dstNode = new SceneEditor2();
    }

    return Scene::Clone(dstNode);
}

SceneEditor2* SceneEditor2::CreateCopyForExport()
{
    auto originalPath = curScenePath;
    auto tempName = DAVA::Format(".tmp_%llu.sc2", static_cast<DAVA::uint64>(time(nullptr)) ^ static_cast<DAVA::uint64>(reinterpret_cast<DAVA::pointer_size>(this)));

    SceneEditor2* ret = nullptr;
    DAVA::FilePath tmpScenePath = DAVA::FilePath::CreateWithNewExtension(curScenePath, tempName);
    if (DAVA::SceneFileV2::ERROR_NO_ERROR == SaveScene(tmpScenePath))
    {
        SceneEditor2* sceneCopy = new SceneEditor2();
        if (DAVA::SceneFileV2::ERROR_NO_ERROR == sceneCopy->LoadScene(tmpScenePath))
        {
            sceneCopy->RemoveSystems();
            ret = sceneCopy;
        }
        else
        {
            SafeRelease(sceneCopy);
        }

        DAVA::FileSystem::Instance()->DeleteFile(tmpScenePath);
    }

    curScenePath = originalPath; // because SaveScene overwrites curScenePath
    SceneSignals::Instance()->EmitUpdated(this);

    return ret;
}

void SceneEditor2::RemoveSystems()
{
    if (editorLightSystem)
    {
        editorLightSystem->SetCameraLightEnabled(false);
        editorLightSystem = nullptr;
    }

    if (landscapeEditorDrawSystem != nullptr)
    {
        landscapeEditorDrawSystem->DisableSystem();
        landscapeEditorDrawSystem = nullptr;
    }

    structureSystem = nullptr;
    collisionSystem = nullptr;
    materialSystem = nullptr;
    visibilityCheckSystem = nullptr;
    cameraSystem = nullptr;
    editorLODSystem = nullptr;
    particlesSystem = nullptr;
    hoodSystem = nullptr;
    pathSystem = nullptr;
    textDrawSystem = nullptr;
    tilemaskEditorSystem = nullptr;
    wayEditSystem = nullptr;

    DAVA::Vector<EditorSceneSystem*> localEditorSystems = editorSystems;
    for (EditorSceneSystem* system : localEditorSystems)
    {
        DAVA::SceneSystem* sceneSystem = dynamic_cast<DAVA::SceneSystem*>(system);
        DVASSERT(sceneSystem != nullptr);

        RemoveSystem(sceneSystem);
        DAVA::SafeDelete(system);
    }
}

void SceneEditor2::MarkAsChanged()
{
    commandStack->SetChanged();
}

void SceneEditor2::Setup3DDrawing()
{
    if (drawCamera)
    {
        drawCamera->SetupDynamicParameters(false);
    }
}

void SceneEditor2::Activate()
{
    SceneSignals::Instance()->EmitActivated(this);
    Scene::Activate();
}

void SceneEditor2::Deactivate()
{
    Scene::Deactivate();
    SceneSignals::Instance()->EmitDeactivated(this);
}

void SceneEditor2::EnableEditorSystems()
{
    for (EditorSceneSystem* system : editorSystems)
    {
        system->EnableSystem();
    }
}

void SceneEditor2::SaveSystemsLocalProperties(DAVA::TArc::PropertiesHolder* holder)
{
    for (EditorSceneSystem* system : editorSystems)
    {
        DVASSERT(system != nullptr);
        system->SaveLocalProperties(holder);
    }
}

void SceneEditor2::LoadSystemsLocalProperties(DAVA::TArc::PropertiesHolder* holder, DAVA::TArc::ContextAccessor* accessor)
{
    for (EditorSceneSystem* system : editorSystems)
    {
        DVASSERT(system != nullptr);
        system->LoadLocalProperties(holder, accessor);
    }
}

DAVA::uint32 SceneEditor2::GetFramesCount() const
{
    return framesCount;
}

void SceneEditor2::ResetFramesCount()
{
    framesCount = 0;
}

void LookAtSelection(SceneEditor2* scene)
{
    if (scene != nullptr)
    {
        scene->cameraSystem->MoveToSelection();
    }
}

void RemoveSelection(SceneEditor2* scene)
{
    if (scene == nullptr)
        return;

    const SelectableGroup& selection = Selection::GetSelection();

    SelectableGroup objectsToRemove;
    for (const auto& item : selection.GetContent())
    {
        if (item.CanBeCastedTo<DAVA::Entity>())
        {
            DAVA::Entity* entity = item.AsEntity();
            if (entity->GetLocked() || entity->GetNotRemovable())
            {
                //Don't remove entity
                continue;
            }

            DAVA::Camera* camera = DAVA::GetCamera(entity);
            if (camera != nullptr && camera == scene->GetCurrentCamera())
            {
                //Don't remove current camera
                continue;
            }
        }

        objectsToRemove.Add(item.GetContainedObject(), item.GetBoundingBox());
    }

    if (objectsToRemove.IsEmpty() == false)
    {
        scene->structureSystem->Remove(objectsToRemove);
    }
}

void LockTransform(SceneEditor2* scene)
{
    if (scene != nullptr)
    {
        scene->modifSystem->LockTransform(Selection::GetSelection(), true);
    }
}

void UnlockTransform(SceneEditor2* scene)
{
    if (scene != nullptr)
    {
        scene->modifSystem->LockTransform(Selection::GetSelection(), false);
    }
}

DAVA_VIRTUAL_REFLECTION_IMPL(SceneEditor2)
{
    DAVA::ReflectionRegistrator<SceneEditor2>::Begin()
    .End();
}
