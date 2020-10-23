#include "Classes/SceneManager/SceneData.h"

DAVA::RefPtr<SceneEditor2> SceneData::GetScene()
{
    return scene;
}

bool SceneData::IsSceneChanged() const
{
    if (scene.Get() == nullptr)
    {
        return false;
    }

    return scene->IsChanged();
}

DAVA::FilePath SceneData::GetScenePath() const
{
    if (scene.Get() == nullptr)
    {
        return DAVA::FilePath();
    }

    return scene->GetScenePath();
}

DAVA::uint32 SceneData::GetEnabledLandscapeTools() const
{
    if (scene.Get() == nullptr)
    {
        return 0;
    }

    return scene->GetEnabledTools();
}

bool SceneData::IsSavingAllowed(QString* message /*= nullptr*/) const
{
    DVASSERT(scene.Get() != nullptr);
    QString warningMessage;
    if (scene->GetEnabledTools() != 0)
    {
        warningMessage = "Disable landscape editing before save!";
    }
    else if (scene->wayEditSystem->IsWayEditEnabled())
    {
        warningMessage = "Disable path editing before save!";
    }

    if (warningMessage.isEmpty())
    {
        return true;
    }
    if (message != nullptr)
    {
        *message = warningMessage;
    }
    return false;
}

SceneEditor2* SceneData::GetScenePtr() const
{
    return scene.Get();
}

bool SceneData::IsHUDVisible() const
{
    if (scene.Get() == nullptr)
    {
        return false;
    }

    return scene->IsHUDVisible();
}

DAVA::TArc::PropertiesHolder* SceneData::GetPropertiesRoot()
{
    return propertiesRoot.get();
}

void SceneData::CreatePropertiesRoot(DAVA::FileSystem* fs, const DAVA::FilePath& dirPath, const DAVA::FilePath& fileName)
{
    fs->CreateDirectory(dirPath, true);
    if (propertiesRoot.get() == nullptr)
    {
        propertiesRoot = std::make_unique<DAVA::TArc::PropertiesHolder>(fileName.GetFilename(), dirPath);
    }
    else
    {
        propertiesRoot = DAVA::TArc::PropertiesHolder::CopyWithNewPath(*propertiesRoot, fs, fileName.GetFilename(), dirPath);
    }
}

const char* SceneData::scenePropertyName = "Scene";
const char* SceneData::sceneChangedPropertyName = "IsSceneChanged";
const char* SceneData::scenePathPropertyName = "ScenePath";
const char* SceneData::sceneLandscapeToolsPropertyName = "EnabledLandscapeTools";
const char* SceneData::sceneHUDVisiblePropertyName = "sceneHUDVisiblePropertyName";
const char* SceneData::sceneCanUndoPropertyName = "canUndo";
const char* SceneData::sceneUndoDescriptionPropertyName = "undoDescription";
const char* SceneData::sceneCanRedoPropertyName = "canRedo";
const char* SceneData::sceneRedoDescriptionPropertyName = "redoDescription";

DAVA_VIRTUAL_REFLECTION_IMPL(GlobalSceneSettings)
{
    DAVA::ReflectionRegistrator<GlobalSceneSettings>::Begin()[DAVA::M::DisplayName("Scene"), DAVA::M::SettingsSortKey(90)]
    .ConstructorByPointer()
    .Field("openLastScene", &GlobalSceneSettings::openLastScene)[DAVA::M::DisplayName("Open last opened scene on launch")]
    .Field("dragAndDropWithShift", &GlobalSceneSettings::dragAndDropWithShift)[DAVA::M::DisplayName("Drag'n'Drop with shift")]
    .Field("saveEmitters", &GlobalSceneSettings::saveEmitters)[DAVA::M::DisplayName("Save Emitters")]
    .Field("saveStaticOcclusion", &GlobalSceneSettings::saveStaticOcclusion)[DAVA::M::DisplayName("Save static occlusion")]
    .Field("defaultCustomColorIndex", &GlobalSceneSettings::defaultCustomColorIndex)[DAVA::M::DisplayName("Default custom color index")]
    .Field("selectionDrawMode", &GlobalSceneSettings::selectionDrawMode)[DAVA::M::DisplayName("Selection draw mode"), DAVA::M::FlagsT<SelectionSystemDrawMode>()]
    .Field("gridStep", &GlobalSceneSettings::gridStep)[DAVA::M::DisplayName("Step"), DAVA::M::Group("Grid")]
    .Field("gridSize", &GlobalSceneSettings::gridSize)[DAVA::M::DisplayName("Size"), DAVA::M::Group("Grid")]
    .Field("cameraSpeed0", &GlobalSceneSettings::cameraSpeed0)[DAVA::M::DisplayName("Speed 0"), DAVA::M::Group("Camera")]
    .Field("cameraSpeed1", &GlobalSceneSettings::cameraSpeed1)[DAVA::M::DisplayName("Speed 1"), DAVA::M::Group("Camera")]
    .Field("cameraSpeed2", &GlobalSceneSettings::cameraSpeed2)[DAVA::M::DisplayName("Speed 2"), DAVA::M::Group("Camera")]
    .Field("cameraSpeed3", &GlobalSceneSettings::cameraSpeed3)[DAVA::M::DisplayName("Speed 3"), DAVA::M::Group("Camera")]
    .Field("cameraFOV", &GlobalSceneSettings::cameraFOV)[DAVA::M::DisplayName("Camera FOV"), DAVA::M::Group("Camera")]
    .Field("cameraNear", &GlobalSceneSettings::cameraNear)[DAVA::M::DisplayName("Camera near"), DAVA::M::Group("Camera")]
    .Field("cameraFar", &GlobalSceneSettings::cameraFar)[DAVA::M::DisplayName("Camera far"), DAVA::M::Group("Camera")]
    .Field("cameraRestoreFullParameters", &GlobalSceneSettings::cameraUseDefaultSettings)[DAVA::M::DisplayName("Use default settings for editor.camera"), DAVA::M::Group("Camera")]
    .Field("heightOnLandscape", &GlobalSceneSettings::heightOnLandscape)[DAVA::M::DisplayName("Height"), DAVA::M::Group("Snap camera to landscape")]
    .Field("heightOnLandscapeStep", &GlobalSceneSettings::heightOnLandscapeStep)[DAVA::M::DisplayName("Height step"), DAVA::M::Group("Snap camera to landscape")]
    .Field("selectionSequent", &GlobalSceneSettings::selectionSequent)[DAVA::M::DisplayName("Select Sequent"), DAVA::M::Group("Selection")]
    .Field("selectionOnClick", &GlobalSceneSettings::selectionOnClick)[DAVA::M::DisplayName("Select on click"), DAVA::M::Group("Selection")]
    .Field("autoSelectNewEntity", &GlobalSceneSettings::autoSelectNewEntity)[DAVA::M::DisplayName("Autoselect new entity"), DAVA::M::Group("Selection")]
    .Field("modificationByGizmoOnly", &GlobalSceneSettings::modificationByGizmoOnly)[DAVA::M::DisplayName("Modify by Gizmo only"), DAVA::M::Group("Modification Gyzmo")]
    .Field("gizmoScale", &GlobalSceneSettings::gizmoScale)[DAVA::M::DisplayName("Gizmo scale"), DAVA::M::Group("Modification Gyzmo")]
    .Field("debugBoxScale", &GlobalSceneSettings::debugBoxScale)[DAVA::M::DisplayName("Box scale"), DAVA::M::Group("Debug draw")]
    .Field("debugBoxUserScale", &GlobalSceneSettings::debugBoxUserScale)[DAVA::M::DisplayName("Box user scale"), DAVA::M::Group("Debug draw")]
    .Field("debugBoxParticleScale", &GlobalSceneSettings::debugBoxParticleScale)[DAVA::M::DisplayName("Box particle scale"), DAVA::M::Group("Debug draw")]
    .Field("debugBoxWaypointScale", &GlobalSceneSettings::debugBoxWaypointScale)[DAVA::M::DisplayName("Box waypoint scale"), DAVA::M::Group("Debug draw")]
    .Field("drawSoundObjects", &GlobalSceneSettings::drawSoundObjects)[DAVA::M::DisplayName("Draw sound objects"), DAVA::M::Group("Sound")]
    .Field("soundObjectBoxColor", &GlobalSceneSettings::soundObjectBoxColor)[DAVA::M::DisplayName("Sound box color"), DAVA::M::Group("Sound")]
    .Field("soundObjectSphereColor", &GlobalSceneSettings::soundObjectSphereColor)[DAVA::M::DisplayName("Sound sphere color"), DAVA::M::Group("Sound")]
    .Field("grabSizeWidth", &GlobalSceneSettings::grabSizeWidth)[DAVA::M::DisplayName("Texture width"), DAVA::M::Group("Grab scene")]
    .Field("grabSizeHeight", &GlobalSceneSettings::grabSizeHeight)[DAVA::M::DisplayName("Texture height"), DAVA::M::Group("Grab scene")]
    .End();
}
