#pragma once

#include "ui_mainwindow.h"


#include <REPlatform/Scene/SceneEditor2.h>

#include <TArc/Models/RecentMenuItems.h>
#include <TArc/DataProcessing/DataListener.h>
#include <TArc/DataProcessing/DataWrapper.h>
#include <TArc/WindowSubSystem/UI.h>

#include <TArc/Utils/ShortcutChecker.h>
#include <TArc/Utils/QtDelayedExecutor.h>

#include <QMainWindow>
#include <QDockWidget>
#include <QPointer>

class RECommandNotificationObject;
class DeveloperTools;
class VersionInfoWidget;
class DeviceListController;
class ErrorDialogOutput;

namespace DAVA
{
class RenderWidget;
class FieldBinder;
}

class QtMainWindow : public QMainWindow, private DAVA::DataListener
{
    Q_OBJECT

    static const int GLOBAL_INVALIDATE_TIMER_DELTA = 1000;

signals:
    void GlobalInvalidateTimeout();

public:
    explicit QtMainWindow(DAVA::UI* tarcUI, QWidget* parent = 0);
    ~QtMainWindow();

    void OnRenderingInitialized();
    void AfterInjectInit();

    void WaitStart(const QString& title, const QString& message, int min, int max);
    void WaitSetMessage(const QString& messsage);
    void WaitSetValue(int value);
    void WaitStop();

    void EnableGlobalTimeout(bool enable);

    bool ParticlesArePacking() const;

    // qt actions slots
public slots:
    void OnEditorGizmoToggle(bool show);
    void OnViewLightmapCanvas(bool show);
    void OnShowStaticOcclusionToggle(bool show);
    void OnEnableVisibilitySystemToggle(bool enabled);
    void OnRefreshVisibilitySystem();
    void OnFixVisibilityFrame();
    void OnReleaseVisibilityFrame();

    void OnEnableDisableShadows(bool enable);

    void EnableSounds(bool enable);

    void OnMaterialEditor(DAVA::NMaterial* material = nullptr);
    void OnTextureBrowser();
    void OnSceneLightMode();

    void OnCubemapEditor();
    void OnImageSplitter();

    void OnOpenHelp();

    void OnSaveHeightmapToImage();
    void OnSaveTiledTexture();
    void OnTiledTextureRetreived(DAVA::Landscape* landscape, DAVA::Texture* landscapeTexture);

    void OnConvertModifiedTextures();

    void OnBuildStaticOcclusion();
    void OnInavalidateStaticOcclusion();

    void OnLandscapeEditorToggled(DAVA::SceneEditor2* scene);
    void OnForceFirstLod(bool);
    void OnCustomColorsEditor();
    void OnHeightmapEditor();
    void OnRulerTool();
    void OnTilemaskEditor();
    void OnNotPassableTerrain();
    void OnWayEditor();
    void OnBakeWaypoints();

    void OnMaterialLightViewChanged(bool);
    void OnCustomQuality();

    void OnGenerateHeightDelta();

    void OnSnapCameraToLandscape(bool);

    void SetupTitle(const DAVA::String& projectPath);

    bool SetVisibilityToolEnabledIfPossible(bool);
    void UpdateLandscapeRenderMode();

    void OnValidateScene();

protected:
    bool eventFilter(QObject* object, QEvent* event) override;
    void SetupMainMenu();
    void SetupToolBars();
    void SetupStatusBar();
    void SetupDocks();
    void SetupActions();

    void StartGlobalInvalidateTimer();

    static void SetActionCheckedSilently(QAction* action, bool checked);

private slots:
    void SceneCommandExecuted(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification);
    void SceneActivated(DAVA::SceneEditor2* scene);
    void SceneDeactivated(DAVA::SceneEditor2* scene);

    void OnGlobalInvalidateTimeout();

    void DebugVersionInfo();
    void OnConsoleItemClicked(const QString& data);

private:
    std::unique_ptr<Ui::MainWindow> ui;
    QPointer<QDockWidget> dockActionEvent;
    QPointer<QDockWidget> dockConsole;

    bool globalInvalidate;

    QComboBox* objectTypesWidget;

    void EnableSceneActions(bool enable);
    void EnableProjectActions(bool enable);
    void UpdateWayEditor(const DAVA::RECommandNotificationObject& commandNotification);

    void LoadViewState(DAVA::SceneEditor2* scene);
    void LoadWayEditorState(DAVA::SceneEditor2* scene);
    void LoadEditorLightState(DAVA::SceneEditor2* scene);
    void LoadLandscapeEditorState(DAVA::SceneEditor2* scene);
    void LoadMaterialLightViewMode();

    // Landscape editor specific
    // TODO: remove later -->
    bool CanEnableLandscapeEditor() const;
    bool LoadAppropriateTextureFormat() const;
    // <--

    void OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields) override;

    //Need for any debug functionality
    QPointer<DeveloperTools> developerTools;
    QPointer<VersionInfoWidget> versionInfoWidget;

    ErrorDialogOutput* errorLoggerOutput = nullptr;

#if defined(__DAVAENGINE_MACOS__)
    DAVA::ShortcutChecker shortcutChecker;
#endif

    DAVA::UI* tarcUI = nullptr;
    std::unique_ptr<DAVA::WaitHandle> waitDialog;
    DAVA::DataWrapper projectDataWrapper;
    DAVA::DataWrapper selectionWrapper;

    void UpdateTagDependentActionsState(const DAVA::Any& value);
    std::unique_ptr<DAVA::FieldBinder> fieldBinderTagged;

    DAVA::QtDelayedExecutor delayedExecutor;
};
