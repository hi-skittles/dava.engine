#pragma once

#include "Classes/SceneManager/Private/SceneRenderWidget.h"

#include <TArc/Core/ControllerModule.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Utils/QtConnections.h>
#include <TArc/Models/RecentMenuItems.h>

#include <Base/BaseTypes.h>

namespace DAVA
{
class FilePath;
class SceneEditor2;
class SceneData;
}
class FileSystemCache;

class SceneManagerModule : public DAVA::ControllerModule, private SceneRenderWidget::IWidgetDelegate
{
public:
    SceneManagerModule();
    ~SceneManagerModule() override;

protected:
    void OnRenderSystemInitialized(DAVA::Window* w) override;
    bool CanWindowBeClosedSilently(const DAVA::WindowKey& key, DAVA::String& requestWindowText) override;
    bool ControlWindowClosing(const DAVA::WindowKey& key, QCloseEvent* event) override;
    bool SaveOnWindowClose(const DAVA::WindowKey& key) override;
    void RestoreOnWindowClose(const DAVA::WindowKey& key) override;

    void OnContextCreated(DAVA::DataContext* context) override;
    void OnContextDeleted(DAVA::DataContext* context) override;
    void OnContextWillBeChanged(DAVA::DataContext* current, DAVA::DataContext* newOne) override;
    void OnContextWasChanged(DAVA::DataContext* current, DAVA::DataContext* oldOne) override;
    void OnWindowClosed(const DAVA::WindowKey& key) override;

    void PostInit() override;

private:
    void CreateModuleControls(DAVA::UI* ui);
    void CreateModuleActions(DAVA::UI* ui);
    void RegisterOperations();

    /// Action and operation handlers
    void CreateFirstScene();
    void CreateNewScene();
    void OpenScene();
    void OpenSceneQuckly();
    void OpenSceneByPath(const DAVA::FilePath& scenePath);
    void AddSceneByPath(const DAVA::FilePath& scenePath);
    void SaveScene();
    void SaveScene(bool saveAs);
    void SaveSceneToFolder(bool compressedTextures);
    void ExportScene();
    void CloseAllScenes(bool needSavingReqiest);
    void ReloadAllTextures(DAVA::eGPUFamily gpu);
    void ReloadTextures(DAVA::Vector<DAVA::Texture*> textures);

    /// Fields value handlers
    void OnProjectPathChanged(const DAVA::Any& projectPath);

    /// IWidgetDelegate
    bool OnCloseSceneRequest(DAVA::uint64 id) override;
    void OnDragEnter(QObject* target, QDragEnterEvent* event) override;
    void OnDragMove(QObject* target, QDragMoveEvent* event) override;
    void OnDrop(QObject* target, QDropEvent* event) override;

    /// Helpers
    bool CanCloseScene(DAVA::SceneData* data);
    DAVA::RefPtr<DAVA::SceneEditor2> OpenSceneImpl(const DAVA::FilePath& scenePath);

    /// This method try to scene at "scenePath" place.
    /// If "scenePath" is empty, method try to save scene at current scene file.
    /// If current scene path is empty (for example this is completely new scene), method will call FileSaveDialog
    /// return true if scene was saved
    /// Preconditions:
    ///     "scenePath" - should be a file
    bool SaveSceneImpl(DAVA::RefPtr<DAVA::SceneEditor2> scene, const DAVA::FilePath& scenePath = DAVA::FilePath());
    DAVA::FilePath GetSceneSavePath(const DAVA::RefPtr<DAVA::SceneEditor2>& scene);

    void GetPropertiesFilePath(const DAVA::FilePath& scenePath, DAVA::FilePath& path,
                               DAVA::FilePath& fileName, bool sceneIsTemp = false);
    void CreateSceneProperties(DAVA::SceneData* const data, bool sceneIsTemp = false);

    /// scene->SaveEmitters() would call this function if emitter to save didn't have path
    DAVA::FilePath SaveEmitterFallback(const DAVA::String& entityName, const DAVA::String& emitterName);
    bool IsSceneCompatible(const DAVA::FilePath& scenePath);

    bool SaveTileMaskInAllScenes();
    bool SaveTileMaskInScene(DAVA::RefPtr<DAVA::SceneEditor2> scene);

    bool CloseSceneImpl(DAVA::uint64 id, bool needSavingRequest);
    void RestartParticles();
    bool IsSavingAllowed(DAVA::SceneData* sceneData);
    void DefaultDragHandler(QObject* target, QDropEvent* event);
    bool IsValidMimeData(QDropEvent* event);
    void DeleteSelection();
    void MoveToSelection();

    bool SaveToFolderAvailable() const;

private:
    DAVA::QtConnections connections;
    DAVA::uint32 newSceneCounter = 0;

    std::unique_ptr<DAVA::FieldBinder> fieldBinder;
    std::unique_ptr<DAVA::RecentMenuItems> recentItems;

    std::unique_ptr<FileSystemCache> sceneFilesCache;
    DAVA::FilePath cachedPath;

    QPointer<SceneRenderWidget> renderWidget;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SceneManagerModule, DAVA::ControllerModule)
    {
        DAVA::ReflectionRegistrator<SceneManagerModule>::Begin()
        .ConstructorByPointer()
        .Field("saveToFolderAvailable", &SceneManagerModule::SaveToFolderAvailable, nullptr)
        .End();
    }
};
