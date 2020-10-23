#include "mainwindow.h"

#include "Classes/Qt/CubemapEditor/CubeMapTextureBrowser.h"
#include "Classes/Qt/CubemapEditor/CubemapUtils.h"
#include "Classes/Qt/DebugTools/VersionInfoWidget/VersionInfoWidget.h"
#include "Classes/Qt/ImageSplitterDialog/ImageSplitterDialog.h"
#include "Classes/Qt/MaterialEditor/MaterialEditor.h"
#include "Classes/Qt/QualitySwitcher/QualitySwitcher.h"
#include "Classes/Qt/RunActionEventWidget/RunActionEventWidget.h"
#include "Classes/Qt/Scene/Validation/SceneValidationDialog.h"
#include "Classes/Qt/TextureBrowser/TextureBrowser.h"
#include "Classes/Qt/TextureBrowser/TextureCache.h"
#include "Classes/Qt/Tools/BaseAddEntityDialog/BaseAddEntityDialog.h"
#include "Classes/Qt/Tools/DeveloperTools/DeveloperTools.h"
#include "Classes/Qt/Tools/HeightDeltaTool/HeightDeltaTool.h"
#include "Classes/Qt/Tools/PathDescriptor/PathDescriptor.h"
#include "Classes/Qt/Tools/ToolButtonWithWidget/ToolButtonWithWidget.h"
#include "Classes/Qt/Tools/ExportSceneDialog/ExportSceneDialog.h"
#include "Classes/Qt/Tools/LoggerOutput/ErrorDialogOutput.h"
#include "Classes/Qt/DockLandscapeEditorControls/LandscapeEditorShortcutManager.h"
#include "Classes/Application/FileSystemData.h"

#include <REPlatform/Commands/AddComponentCommand.h>
#include <REPlatform/Commands/CustomColorsCommands2.h>
#include <REPlatform/Commands/EntityAddCommand.h>
#include <REPlatform/Commands/HeightmapEditorCommands2.h>
#include <REPlatform/Commands/LandscapeToolsToggleCommand.h>
#include <REPlatform/Commands/RECommandNotificationObject.h>
#include <REPlatform/Commands/RemoveComponentCommand.h>
#include <REPlatform/Commands/TilemaskEditorCommands.h>
#include <REPlatform/Commands/WayEditCommands.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/Settings/GlobalSceneSettings.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/DataNodes/SpritesPackerModule.h>
#include <REPlatform/Deprecated/EditorConfig.h>
#include <REPlatform/Deprecated/SceneValidator.h>
#include <REPlatform/Global/Constants.h>
#include <REPlatform/Global/GlobalOperations.h>
#include <REPlatform/Global/StringConstants.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/SceneHelper.h>
#include <REPlatform/Scene/Systems/CameraSystem.h>
#include <REPlatform/Scene/Systems/CollisionSystem.h>
#include <REPlatform/Scene/Systems/CustomColorsSystem.h>
#include <REPlatform/Scene/Systems/EditorLightSystem.h>
#include <REPlatform/Scene/Systems/EditorMaterialSystem.h>
#include <REPlatform/Scene/Systems/EditorVegetationSystem.h>
#include <REPlatform/Scene/Systems/EditorVegetationSystem.h>
#include <REPlatform/Scene/Systems/EditorVegetationSystem.h>
#include <REPlatform/Scene/Systems/HeightmapEditorSystem.h>
#include <REPlatform/Scene/Systems/LandscapeEditorDrawSystem.h>
#include <REPlatform/Scene/Systems/PathSystem.h>
#include <REPlatform/Scene/Systems/RulerToolSystem.h>
#include <REPlatform/Scene/Systems/SelectionSystem.h>
#include <REPlatform/Scene/Systems/TilemaskEditorSystem.h>
#include <REPlatform/Scene/Systems/VisibilityCheckSystem.h>
#include <REPlatform/Scene/Systems/WayEditSystem.h>
#include <REPlatform/Scene/Utils/SceneSaver.h>
#include <REPlatform/Scene/Utils/Utils.h>

#include <TextureCompression/TextureConverter.h>
#include <Version/Version.h>

#include <QtTools/ConsoleWidget/LogWidget.h>
#include <QtTools/ConsoleWidget/LogModel.h>
#include <QtTools/ConsoleWidget/PointerSerializer.h>
#include <QtTools/ConsoleWidget/LoggerOutputObject.h>
#include <QtTools/FileDialogs/FindFileDialog.h>

#include <TArc/WindowSubSystem/Private/WaitDialog.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Core/Deprecated.h>
#include <TArc/Utils/Utils.h>

#include <Engine/Engine.h>
#include <Engine/PlatformApiQt.h>
#include <Engine/Qt/RenderWidget.h>
#include <Reflection/ReflectedType.h>
#include <Render/2D/Sprite.h>
#include <Render/Highlevel/LandscapeThumbnails.h>
#include <Scene3D/Components/ActionComponent.h>
#include <Scene3D/Components/TextComponent.h>
#include <Scene3D/Components/Waypoint/PathComponent.h>
#include <Scene3D/Components/Controller/WASDControllerComponent.h>
#include <Scene3D/Components/Controller/RotationControllerComponent.h>
#include <Scene3D/Systems/StaticOcclusionBuildSystem.h>
#include <Scene3D/Systems/StaticOcclusionSystem.h>
#include <Utils/StringFormat.h>

#include <QDesktopServices>
#include <QMessageBox>

namespace MainWindowDetails
{
bool IsSavingAllowed(const QString& warningTitle)
{
    DAVA::SceneData* data = DAVA::Deprecated::GetActiveDataNode<DAVA::SceneData>();
    DVASSERT(data);
    QString message;
    bool result = data->IsSavingAllowed(&message);
    if (result == false)
    {
        using namespace DAVA;

        ModalMessageParams params;
        params.buttons = ModalMessageParams::Ok;
        params.message = message;
        params.title = warningTitle;
        DAVA::Deprecated::ShowModalMessage(params);
    }

    return result;
}

DAVA::RefPtr<DAVA::SceneEditor2> GetCurrentScene()
{
    DAVA::SceneData* data = DAVA::Deprecated::GetActiveDataNode<DAVA::SceneData>();
    DAVA::RefPtr<DAVA::SceneEditor2> result;
    if (data)
    {
        result = data->GetScene();
    }
    return result;
}
}

QtMainWindow::QtMainWindow(DAVA::UI* tarcUI_, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , globalInvalidate(false)
    , developerTools(new DeveloperTools(this))
#if defined(__DAVAENGINE_MACOS__)
    , shortcutChecker(this)
#endif
    , tarcUI(tarcUI_)
{
    projectDataWrapper = DAVA::Deprecated::CreateDataWrapper(DAVA::ReflectedTypeDB::Get<DAVA::ProjectManagerData>());
    projectDataWrapper.SetListener(this);

    fieldBinderTagged.reset(DAVA::Deprecated::CreateFieldBinder());
    { // bind to changed data
        DAVA::FieldDescriptor sceneFieldDescr;
        sceneFieldDescr.type = DAVA::ReflectedTypeDB::Get<DAVA::SceneData>();
        sceneFieldDescr.fieldName = DAVA::FastName(DAVA::SceneData::scenePropertyName);

        DAVA::FieldDescriptor fsFieldDescr;
        fsFieldDescr.type = DAVA::ReflectedTypeDB::Get<FileSystemData>();
        fsFieldDescr.fieldName = DAVA::FastName("tag");

        fieldBinderTagged->BindField(sceneFieldDescr, DAVA::MakeFunction(this, &QtMainWindow::UpdateTagDependentActionsState));
        fieldBinderTagged->BindField(fsFieldDescr, DAVA::MakeFunction(this, &QtMainWindow::UpdateTagDependentActionsState));
    }

    errorLoggerOutput = new ErrorDialogOutput(tarcUI);
    DAVA::Logger::AddCustomOutput(errorLoggerOutput);

    new LandscapeEditorShortcutManager(this);
    PathDescriptor::InitializePathDescriptors();

    ui->setupUi(this);
    setObjectName("ResourceEditor"); //we need to support old names to save settings

    SetupTitle(DAVA::String());

    qApp->installEventFilter(this);

    SetupDocks();
    SetupMainMenu();
    SetupToolBars();
    SetupActions();

    // create tool windows
    new TextureBrowser(this);
    new MaterialEditor(this);

    connect(SceneSignals::Instance(), &SceneSignals::CommandExecuted, this, &QtMainWindow::SceneCommandExecuted);
    connect(SceneSignals::Instance(), &SceneSignals::Activated, this, &QtMainWindow::SceneActivated);
    connect(SceneSignals::Instance(), &SceneSignals::Deactivated, this, &QtMainWindow::SceneDeactivated);

    selectionWrapper = DAVA::Deprecated::CreateDataWrapper(DAVA::ReflectedTypeDB::Get<DAVA::SelectionData>());
    selectionWrapper.SetListener(this);

    LoadMaterialLightViewMode();

    EnableGlobalTimeout(globalInvalidate);

    EnableProjectActions(false);
    EnableSceneActions(false);
}

QtMainWindow::~QtMainWindow()
{
    errorLoggerOutput->Disable();
    errorLoggerOutput = nullptr; // will be deleted by DAVA::Logger;

    LogWidget* logWidget = qobject_cast<LogWidget*>(dockConsole->widget());
    QByteArray dataToSave = logWidget->Serialize();

    DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->logWidgetState = dataToSave;

    TextureBrowser::Instance()->Release();
    MaterialEditor::Instance()->Release();

    LandscapeEditorShortcutManager::Instance()->Release();
}

void QtMainWindow::OnRenderingInitialized()
{
    ui->landscapeEditorControlsPlaceholder->OnOpenGLInitialized();
    DAVA::RenderWidget* renderWidget = DAVA::PlatformApi::Qt::GetRenderWidget();
    renderWidget->resized.Connect(ui->statusBar, &StatusBar::OnSceneGeometryChaged);
}

void QtMainWindow::AfterInjectInit()
{
    SetupStatusBar();
}

void QtMainWindow::WaitStart(const QString& title, const QString& message, int min, int max)
{
    DVASSERT(waitDialog == nullptr);

    DAVA::WaitDialogParams params;
    params.message = message;
    params.min = min;
    params.max = max;
    params.needProgressBar = min != max;
    waitDialog = tarcUI->ShowWaitDialog(DAVA::mainWindowKey, params);
}

void QtMainWindow::WaitSetMessage(const QString& messsage)
{
    DVASSERT(waitDialog != nullptr);
    waitDialog->SetMessage(messsage);
}

void QtMainWindow::WaitSetValue(int value)
{
    DVASSERT(waitDialog != nullptr);
    waitDialog->SetProgressValue(value);
}

void QtMainWindow::WaitStop()
{
    waitDialog.reset();
}

bool QtMainWindow::eventFilter(QObject* obj, QEvent* event)
{
    QEvent::Type eventType = event->type();

#if defined(__DAVAENGINE_MACOS__)
    if (QEvent::ShortcutOverride == eventType && shortcutChecker.TryCallShortcut(static_cast<QKeyEvent*>(event)))
    {
        return true;
    }
#endif
    return QMainWindow::eventFilter(obj, event);
}

void QtMainWindow::SetupTitle(const DAVA::String& projectPath)
{
    DAVA::String title = DAVA::Version::CreateAppVersion("Resource Editor");
    if (!projectPath.empty())
    {
        title += " | Project - ";
        title += projectPath;
    }

    setWindowTitle(QString::fromStdString(title));
}

void QtMainWindow::SetupMainMenu()
{
    ui->Dock->addAction(ui->dockSceneInfo->toggleViewAction());
    ui->Dock->addAction(ui->dockParticleEditor->toggleViewAction());
    ui->Dock->addAction(ui->dockParticleEditorTimeLine->toggleViewAction());
    ui->Dock->addAction(ui->dockLODEditor->toggleViewAction());
    ui->Dock->addAction(ui->dockLandscapeEditorControls->toggleViewAction());

    ui->Dock->addAction(dockActionEvent->toggleViewAction());
    ui->Dock->addAction(dockConsole->toggleViewAction());
}

void QtMainWindow::SetupToolBars()
{
    QAction* actionMainToolBar = ui->mainToolBar->toggleViewAction();
    QAction* actionLandscapeToolbar = ui->landscapeToolBar->toggleViewAction();

    ui->Toolbars->addAction(actionMainToolBar);
    ui->Toolbars->addAction(actionLandscapeToolbar);
    ui->Toolbars->addAction(ui->sceneToolBar->toggleViewAction());
    ui->Toolbars->addAction(ui->cameraToolBar->toggleViewAction());

    // adding menu for material light view mode
    {
        QToolButton* setLightViewMode = new QToolButton();
        setLightViewMode->setMenu(ui->menuLightView);
        setLightViewMode->setPopupMode(QToolButton::InstantPopup);
        setLightViewMode->setDefaultAction(ui->actionSetLightViewMode);
        ui->mainToolBar->addWidget(setLightViewMode);
        setLightViewMode->setToolButtonStyle(Qt::ToolButtonIconOnly);
        setLightViewMode->setAutoRaise(false);
    }
}

void QtMainWindow::SetupStatusBar()
{
    QObject::connect(SceneSignals::Instance(), &SceneSignals::Activated, ui->statusBar, &StatusBar::SceneActivated);
    QObject::connect(SceneSignals::Instance(), &SceneSignals::Deactivated, ui->statusBar, &StatusBar::SceneDeactivated);

    QObject::connect(this, &QtMainWindow::GlobalInvalidateTimeout, ui->statusBar, &StatusBar::UpdateByTimer);

    DAVA::InsertionParams insertParams;
    insertParams.method = DAVA::InsertionParams::eInsertionMethod::BeforeItem;
    DAVA::ActionPlacementInfo placementInfo(DAVA::CreateStatusbarPoint(true, 0, insertParams));

    tarcUI->AddAction(DAVA::mainWindowKey, placementInfo, ui->actionShowEditorGizmo);
    tarcUI->AddAction(DAVA::mainWindowKey, placementInfo, ui->actionLightmapCanvas);
    tarcUI->AddAction(DAVA::mainWindowKey, placementInfo, ui->actionShowStaticOcclusion);
    tarcUI->AddAction(DAVA::mainWindowKey, placementInfo, ui->actionEnableVisibilitySystem);
    tarcUI->AddAction(DAVA::mainWindowKey, placementInfo, ui->actionEnableDisableShadows);
    tarcUI->AddAction(DAVA::mainWindowKey, placementInfo, ui->actionEnableSounds);
}

void QtMainWindow::SetupDocks()
{
    QObject::connect(this, SIGNAL(GlobalInvalidateTimeout()), ui->sceneInfo, SLOT(UpdateInfoByTimer()));

    // Run Action Event dock
    {
        dockActionEvent = new QDockWidget("Run Action Event", this);
        dockActionEvent->setWidget(new RunActionEventWidget());
        dockActionEvent->setObjectName(QString("dock_%1").arg(dockActionEvent->widget()->objectName()));
        addDockWidget(Qt::RightDockWidgetArea, dockActionEvent);
    }
    // Console dock
    {
        LogWidget* logWidget = new LogWidget();
        logWidget->SetConvertFunction(&PointerSerializer::CleanUpString);

        LoggerOutputObject* loggerOutput = new LoggerOutputObject(); //will be removed by DAVA::Logger
        connect(loggerOutput, &LoggerOutputObject::OutputReady, logWidget, &LogWidget::AddMessage, Qt::DirectConnection);

        connect(logWidget, &LogWidget::ItemClicked, this, &QtMainWindow::OnConsoleItemClicked);
        logWidget->Deserialize(DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->logWidgetState);
        dockConsole = new QDockWidget(logWidget->windowTitle(), this);
        dockConsole->setWidget(logWidget);
        dockConsole->setObjectName(QString("dock_%1").arg(dockConsole->widget()->objectName()));
        addDockWidget(Qt::RightDockWidgetArea, dockConsole);
    }
}

void QtMainWindow::SetupActions()
{
    QObject::connect(ui->actionAlbedo, SIGNAL(toggled(bool)), this, SLOT(OnMaterialLightViewChanged(bool)));
    QObject::connect(ui->actionAmbient, SIGNAL(toggled(bool)), this, SLOT(OnMaterialLightViewChanged(bool)));
    QObject::connect(ui->actionDiffuse, SIGNAL(toggled(bool)), this, SLOT(OnMaterialLightViewChanged(bool)));
    QObject::connect(ui->actionSpecular, SIGNAL(toggled(bool)), this, SLOT(OnMaterialLightViewChanged(bool)));

    OnEditorGizmoToggle(DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->gizmoEnabled);
    QObject::connect(ui->actionShowEditorGizmo, SIGNAL(toggled(bool)), this, SLOT(OnEditorGizmoToggle(bool)));

    QObject::connect(ui->actionLightmapCanvas, SIGNAL(toggled(bool)), this, SLOT(OnViewLightmapCanvas(bool)));
    QObject::connect(ui->actionShowStaticOcclusion, SIGNAL(toggled(bool)), this, SLOT(OnShowStaticOcclusionToggle(bool)));
    QObject::connect(ui->actionEnableVisibilitySystem, SIGNAL(triggered(bool)), this, SLOT(OnEnableVisibilitySystemToggle(bool)));

    QObject::connect(ui->actionRefreshVisibilitySystem, SIGNAL(triggered()), this, SLOT(OnRefreshVisibilitySystem()));
    QObject::connect(ui->actionFixCurrentFrame, SIGNAL(triggered()), this, SLOT(OnFixVisibilityFrame()));
    QObject::connect(ui->actionReleaseCurrentFrame, SIGNAL(triggered()), this, SLOT(OnReleaseVisibilityFrame()));

    QObject::connect(ui->actionEnableDisableShadows, &QAction::toggled, this, &QtMainWindow::OnEnableDisableShadows);

    EnableSounds(DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->enableSound);
    QObject::connect(ui->actionEnableSounds, &QAction::toggled, this, &QtMainWindow::EnableSounds);

    // quality
    QObject::connect(ui->actionCustomQuality, SIGNAL(triggered()), this, SLOT(OnCustomQuality()));

    // tools
    QObject::connect(ui->actionMaterialEditor, SIGNAL(triggered()), this, SLOT(OnMaterialEditor()));
    QObject::connect(ui->actionTextureConverter, SIGNAL(triggered()), this, SLOT(OnTextureBrowser()));
    QObject::connect(ui->actionEnableCameraLight, SIGNAL(triggered()), this, SLOT(OnSceneLightMode()));
    QObject::connect(ui->actionCubemapEditor, SIGNAL(triggered()), this, SLOT(OnCubemapEditor()));
    QObject::connect(ui->actionImageSplitter, SIGNAL(triggered()), this, SLOT(OnImageSplitter()));

    QObject::connect(ui->actionForceFirstLODonLandscape, SIGNAL(triggered(bool)), this, SLOT(OnForceFirstLod(bool)));
    QObject::connect(ui->actionShowNotPassableLandscape, SIGNAL(triggered()), this, SLOT(OnNotPassableTerrain()));
    QObject::connect(ui->actionCustomColorsEditor, SIGNAL(triggered()), this, SLOT(OnCustomColorsEditor()));
    QObject::connect(ui->actionHeightMapEditor, SIGNAL(triggered()), this, SLOT(OnHeightmapEditor()));
    QObject::connect(ui->actionTileMapEditor, SIGNAL(triggered()), this, SLOT(OnTilemaskEditor()));
    QObject::connect(ui->actionRulerTool, SIGNAL(triggered()), this, SLOT(OnRulerTool()));
    QObject::connect(ui->actionWayEditor, SIGNAL(triggered()), this, SLOT(OnWayEditor()));
    QObject::connect(ui->actionBakeWaypoints, SIGNAL(triggered()), this, SLOT(OnBakeWaypoints()));

    QObject::connect(ui->actionSaveHeightmapToPNG, SIGNAL(triggered()), this, SLOT(OnSaveHeightmapToImage()));
    QObject::connect(ui->actionSaveTiledTexture, SIGNAL(triggered()), this, SLOT(OnSaveTiledTexture()));
    QObject::connect(ui->actionConvertModifiedTextures, SIGNAL(triggered()), this, SLOT(OnConvertModifiedTextures()));
    QObject::connect(ui->actionBuildStaticOcclusion, SIGNAL(triggered()), this, SLOT(OnBuildStaticOcclusion()));
    QObject::connect(ui->actionInvalidateStaticOcclusion, SIGNAL(triggered()), this, SLOT(OnInavalidateStaticOcclusion()));

    connect(ui->actionHeightmap_Delta_Tool, SIGNAL(triggered()), this, SLOT(OnGenerateHeightDelta()));

    //Help
    QObject::connect(ui->actionHelp, SIGNAL(triggered()), this, SLOT(OnOpenHelp()));

    //Landscape editors toggled
    QObject::connect(SceneSignals::Instance(), &SceneSignals::LandscapeEditorToggled, this, &QtMainWindow::OnLandscapeEditorToggled);

    // Debug functions
    QObject::connect(ui->actionGridCopy, SIGNAL(triggered()), developerTools, SLOT(OnDebugFunctionsGridCopy()));
    {
#ifdef USER_VERSIONING_DEBUG_FEATURES
        QAction* act = ui->menuDebug_Functions->addAction("Edit version tags");
        connect(act, SIGNAL(triggered()), SLOT(DebugVersionInfo()));
#endif
    }

    connect(ui->actionImageSplitterForNormals, &QAction::triggered, developerTools, &DeveloperTools::OnImageSplitterNormals);
    connect(ui->actionReplaceTextureMipmap, &QAction::triggered, developerTools, &DeveloperTools::OnReplaceTextureMipmap);
    connect(ui->actionToggleUseInstancing, &QAction::triggered, developerTools, &DeveloperTools::OnToggleLandscapeInstancing);

    connect(ui->actionDumpTextures, &QAction::triggered, [] {
        DAVA::Texture::DumpTextures();
    });
    connect(ui->actionDumpSprites, &QAction::triggered, [] {
        DAVA::Sprite::DumpSprites();
    });

    connect(ui->actionCreateTestHardSkinnedObject, SIGNAL(triggered()), developerTools, SLOT(OnDebugCreateTestHardSkinnedObject()));
    connect(ui->actionCreateTestSoftSkinnedObject, SIGNAL(triggered()), developerTools, SLOT(OnDebugCreateTestSoftSkinnedObject()));

    QObject::connect(ui->actionSnapCameraToLandscape, SIGNAL(triggered(bool)), this, SLOT(OnSnapCameraToLandscape(bool)));

    QObject::connect(ui->actionValidateScene, SIGNAL(triggered()), this, SLOT(OnValidateScene()));
}

// ###################################################################################################
// Scene signals
// ###################################################################################################

void QtMainWindow::SceneActivated(DAVA::SceneEditor2* scene)
{
    EnableSceneActions(true);

    LoadViewState(scene);
    LoadWayEditorState(scene);
    LoadEditorLightState(scene);
    LoadLandscapeEditorState(scene);

    OnMaterialLightViewChanged(true);
    OnViewLightmapCanvas(true);

    ui->actionSnapCameraToLandscape->setChecked(false);
    if (nullptr != scene)
    {
        scene->SetHUDVisible(ui->actionShowEditorGizmo->isChecked());
        ui->actionSnapCameraToLandscape->setChecked(scene->GetSystem<DAVA::SceneCameraSystem>()->IsEditorCameraSnappedToLandscape());
    }
}

void QtMainWindow::SceneDeactivated(DAVA::SceneEditor2* scene)
{
    // block some actions, when there is no scene
    EnableSceneActions(false);
}

void QtMainWindow::EnableProjectActions(bool enable)
{
    ui->actionCubemapEditor->setEnabled(enable);
    ui->actionImageSplitter->setEnabled(enable);
}

void QtMainWindow::EnableSceneActions(bool enable)
{
    ui->dockLODEditor->setEnabled(enable);
    ui->dockSceneInfo->setEnabled(enable);

    ui->actionWayEditor->setEnabled(enable);
    ui->actionForceFirstLODonLandscape->setEnabled(enable);
    ui->actionEnableVisibilitySystem->setEnabled(enable);

    ui->actionEnableCameraLight->setEnabled(enable);

    ui->actionSetLightViewMode->setEnabled(enable);

    ui->actionSaveHeightmapToPNG->setEnabled(enable);
    ui->actionSaveTiledTexture->setEnabled(enable);

    ui->Edit->setEnabled(enable);
    ui->Scene->setEnabled(enable);
    ui->menuLightView->setEnabled(enable);

    ui->sceneToolBar->setEnabled(enable);
    ui->actionConvertModifiedTextures->setEnabled(enable);

    ui->actionSnapCameraToLandscape->setEnabled(enable);
    ui->actionHeightmap_Delta_Tool->setEnabled(enable);

    ui->actionValidateScene->setEnabled(enable);

    // Fix for menuBar rendering
    const auto isMenuBarEnabled = ui->menuBar->isEnabled();
    ui->menuBar->setEnabled(false);
    ui->menuBar->setEnabled(isMenuBarEnabled);
}

void QtMainWindow::UpdateTagDependentActionsState(const DAVA::Any& value)
{
    const DAVA::String& tag = DAVA::GetEngineContext()->fileSystem->GetFilenamesTag();
    bool enable = (tag.empty() == true) && (DAVA::Deprecated::GetActiveDataNode<DAVA::SceneData>() != nullptr);

    ui->actionTextureConverter->setEnabled(enable);
    ui->actionMaterialEditor->setEnabled(enable);
    ui->actionHeightMapEditor->setEnabled(enable);
    ui->actionTileMapEditor->setEnabled(enable);
    ui->actionShowNotPassableLandscape->setEnabled(enable);
    ui->actionRulerTool->setEnabled(enable);
    ui->actionCustomColorsEditor->setEnabled(enable);
}

void QtMainWindow::UpdateWayEditor(const DAVA::RECommandNotificationObject& commandNotification)
{
    if (commandNotification.MatchCommandTypes<DAVA::EnableWayEditCommand>())
    {
        DVASSERT(commandNotification.MatchCommandTypes<DAVA::DisableWayEditCommand>() == false);
        SetActionCheckedSilently(ui->actionWayEditor, commandNotification.IsRedo());
        ui->actionBakeWaypoints->setEnabled(commandNotification.IsRedo() == false);
    }
    else if (commandNotification.MatchCommandTypes<DAVA::DisableWayEditCommand>())
    {
        DVASSERT(commandNotification.MatchCommandTypes<DAVA::EnableWayEditCommand>() == false);
        SetActionCheckedSilently(ui->actionWayEditor, !commandNotification.IsRedo());
        ui->actionBakeWaypoints->setEnabled(commandNotification.IsRedo() == true);
    }
}

void QtMainWindow::SceneCommandExecuted(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification)
{
    if (scene == MainWindowDetails::GetCurrentScene())
    {
        UpdateWayEditor(commandNotification);

        DAVA::Vector<DAVA::Entity*> entities;
        commandNotification.ForEach<DAVA::AddComponentCommand>([&](const DAVA::AddComponentCommand* cmd) {
            entities.push_back(cmd->GetEntity());
        });

        commandNotification.ForEach<DAVA::RemoveComponentCommand>([&](const DAVA::RemoveComponentCommand* cmd) {
            entities.push_back(cmd->GetEntity());
        });

        for (DAVA::Entity* e : entities)
        {
            if (e->GetName() == DAVA::FastName(DAVA::ResourceEditor::EDITOR_DEBUG_CAMERA))
            {
                SetActionCheckedSilently(ui->actionSnapCameraToLandscape, scene->GetSystem<DAVA::SceneCameraSystem>()->IsEditorCameraSnappedToLandscape());
            }
        }
    }
}

// ###################################################################################################
// Mainwindow Qt actions
// ###################################################################################################
void QtMainWindow::OnEditorGizmoToggle(bool show)
{
    ui->actionShowEditorGizmo->setChecked(show);
    DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->gizmoEnabled = show;
    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        scene->SetHUDVisible(show);
    }
}

void QtMainWindow::OnViewLightmapCanvas(bool show)
{
    bool showCanvas = ui->actionLightmapCanvas->isChecked();
    DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->materialShowLightmapCanvas = showCanvas;

    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        scene->GetSystem<DAVA::EditorMaterialSystem>()->SetLightmapCanvasVisible(showCanvas);
    }
}

void QtMainWindow::OnShowStaticOcclusionToggle(bool show)
{
    DAVA::Renderer::GetOptions()->SetOption(DAVA::RenderOptions::DEBUG_DRAW_STATIC_OCCLUSION, show);
}

void QtMainWindow::OnEnableVisibilitySystemToggle(bool enabled)
{
    SetVisibilityToolEnabledIfPossible(enabled);
}

void QtMainWindow::OnRefreshVisibilitySystem()
{
    MainWindowDetails::GetCurrentScene()->GetSystem<DAVA::VisibilityCheckSystem>()->Recalculate();
}

void QtMainWindow::OnFixVisibilityFrame()
{
    MainWindowDetails::GetCurrentScene()->GetSystem<DAVA::VisibilityCheckSystem>()->FixCurrentFrame();
}

void QtMainWindow::OnReleaseVisibilityFrame()
{
    MainWindowDetails::GetCurrentScene()->GetSystem<DAVA::VisibilityCheckSystem>()->ReleaseFixedFrame();
}

void QtMainWindow::OnEnableDisableShadows(bool enable)
{
    DAVA::Renderer::GetOptions()->SetOption(DAVA::RenderOptions::SHADOWVOLUME_DRAW, enable);
}

void QtMainWindow::EnableSounds(bool toEnable)
{
    ui->actionEnableSounds->setChecked(toEnable);
    DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->enableSound = toEnable;
    DAVA::SoundSystem::Instance()->Mute(!toEnable);
}

void QtMainWindow::OnMaterialEditor(DAVA::NMaterial* material)
{
    MaterialEditor* editor = MaterialEditor::Instance();
    editor->show();
    if (material != nullptr)
    {
        editor->SelectMaterial(material);
    }
}

void QtMainWindow::OnTextureBrowser()
{
    TextureBrowser::Instance()->show();

    DAVA::RefPtr<DAVA::SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    TextureBrowser::Instance()->sceneActivated(sceneEditor.Get());
}

void QtMainWindow::OnSceneLightMode()
{
    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        DAVA::EditorLightSystem* lightSystem = scene->GetSystem<DAVA::EditorLightSystem>();
        if (ui->actionEnableCameraLight->isChecked())
        {
            lightSystem->SetCameraLightEnabled(true);
        }
        else
        {
            lightSystem->SetCameraLightEnabled(false);
        }

        LoadEditorLightState(scene.Get());
    }
}

void QtMainWindow::OnCubemapEditor()
{
    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();

    CubeMapTextureBrowser dlg(scene.Get(), dynamic_cast<QWidget*>(parent()));
    dlg.exec();
}

void QtMainWindow::OnImageSplitter()
{
    ImageSplitterDialog dlg(this);
    dlg.exec();
}

void QtMainWindow::OnOpenHelp()
{
    DAVA::FilePath docsPath = DAVA::ResourceEditor::DOCUMENTATION_PATH + "index.html";
    QString docsFile = QString::fromStdString("file:///" + docsPath.GetAbsolutePathname());
    QDesktopServices::openUrl(QUrl(docsFile));
}

// ###################################################################################################
// Mainwindow load state functions
// ###################################################################################################

void QtMainWindow::LoadViewState(DAVA::SceneEditor2* scene)
{
    if (nullptr != scene)
    {
        ui->actionLightmapCanvas->setChecked(DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->materialShowLightmapCanvas);

        auto options = DAVA::Renderer::GetOptions();
        ui->actionEnableDisableShadows->setChecked(options->IsOptionEnabled(DAVA::RenderOptions::SHADOWVOLUME_DRAW));
    }
}

void QtMainWindow::LoadWayEditorState(DAVA::SceneEditor2* scene)
{
    using namespace DAVA;
    if (nullptr != scene)
    {
        ui->actionWayEditor->setChecked(scene->GetSystem<DAVA::WayEditSystem>()->IsWayEditEnabled());
    }
}

void QtMainWindow::LoadEditorLightState(DAVA::SceneEditor2* scene)
{
    if (nullptr != scene)
    {
        ui->actionEnableCameraLight->setChecked(scene->GetSystem<DAVA::EditorLightSystem>()->GetCameraLightEnabled());
    }
}

void QtMainWindow::LoadMaterialLightViewMode()
{
    int curViewMode = DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->materialLightViewMode;

    ui->actionAlbedo->setChecked((bool)(curViewMode & DAVA::CommonInternalSettings::LIGHTVIEW_ALBEDO));
    ui->actionAmbient->setChecked((bool)(curViewMode & DAVA::CommonInternalSettings::LIGHTVIEW_AMBIENT));
    ui->actionSpecular->setChecked((bool)(curViewMode & DAVA::CommonInternalSettings::LIGHTVIEW_SPECULAR));
    ui->actionDiffuse->setChecked((bool)(curViewMode & DAVA::CommonInternalSettings::LIGHTVIEW_DIFFUSE));
}

void QtMainWindow::LoadLandscapeEditorState(DAVA::SceneEditor2* scene)
{
    OnLandscapeEditorToggled(scene);
}

void QtMainWindow::OnSaveHeightmapToImage()
{
    if (MainWindowDetails::IsSavingAllowed("Save heightmap to Image") == false)
    {
        return;
    }

    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();

    DAVA::Landscape* landscape = FindLandscape(scene.Get());
    QString titleString = "Saving is not allowed";

    if (!landscape)
    {
        QMessageBox::warning(this, titleString, "There is no landscape in scene!");
        return;
    }
    if (!landscape->GetHeightmap()->Size())
    {
        QMessageBox::warning(this, titleString, "There is no heightmap in landscape!");
        return;
    }

    DAVA::Heightmap* heightmap = landscape->GetHeightmap();
    DAVA::FilePath heightmapPath = landscape->GetHeightmapPathname();

    DAVA::FileDialogParams params;
    params.dir = heightmapPath.GetAbsolutePathname().c_str();
    params.filters = PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter;
    params.title = "Save heightmap as";

    QString selectedPath = DAVA::Deprecated::GetUI()->GetSaveFileName(DAVA::mainWindowKey, params);
    if (selectedPath.isEmpty())
        return;

    DAVA::FilePath requestedPngPath = DAVA::FilePath(selectedPath.toStdString());
    heightmap->SaveToImage(requestedPngPath);
}

void QtMainWindow::OnSaveTiledTexture()
{
    if (MainWindowDetails::IsSavingAllowed("Save tiled texture") == false)
    {
        return;
    }

    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    DAVA::LandscapeEditorDrawSystem::eErrorType varifLandscapeError = scene->GetSystem<DAVA::LandscapeEditorDrawSystem>()->VerifyLandscape();
    if (varifLandscapeError != DAVA::LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS)
    {
        DAVA::Logger::Error(DAVA::LandscapeEditorDrawSystem::GetDescriptionByError(varifLandscapeError).c_str());
        return;
    }

    DAVA::Landscape* landscape = FindLandscape(scene.Get());
    if (nullptr != landscape)
    {
        DAVA::LandscapeThumbnails::Create(landscape, MakeFunction(this, &QtMainWindow::OnTiledTextureRetreived));
    }
}

void QtMainWindow::OnTiledTextureRetreived(DAVA::Landscape* landscape, DAVA::Texture* landscapeTexture)
{
    DAVA::FilePath pathToSave = landscape->GetMaterial()->GetEffectiveTexture(DAVA::Landscape::TEXTURE_COLOR)->GetPathname();
    if (pathToSave.IsEmpty())
    {
        DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
        DVASSERT(data != nullptr);

        DAVA::FileDialogParams params;
        params.dir = data->GetDataSource3DPath().GetAbsolutePathname().c_str();
        params.title = "Save landscape texture as";
        params.filters = PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter;
        QString selectedPath = DAVA::Deprecated::GetUI()->GetSaveFileName(DAVA::mainWindowKey, params);

        if (selectedPath.isEmpty())
        {
            return;
        }

        pathToSave = DAVA::FilePath(selectedPath.toStdString());
    }
    else
    {
        pathToSave.ReplaceExtension(".thumbnail.png");
    }

    DAVA::SaveTextureToFile(landscapeTexture, pathToSave);
}

void QtMainWindow::OnConvertModifiedTextures()
{
    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (!scene)
    {
        return;
    }

    WaitStart("Conversion of modified textures.", "Checking for modified textures.", 0, 0);
    DAVA::Map<DAVA::Texture*, DAVA::Vector<DAVA::eGPUFamily>> textures;
    int filesToUpdate = DAVA::SceneHelper::EnumerateModifiedTextures(scene.Get(), textures);

    if (filesToUpdate == 0)
    {
        WaitStop();
        return;
    }

    DAVA::TextureConverter::eConvertQuality quality = DAVA::Deprecated::GetDataNode<DAVA::GeneralSettings>()->compressionQuality;

    int convretedNumber = 0;
    waitDialog->SetRange(convretedNumber, filesToUpdate);
    WaitSetValue(convretedNumber);
    for (DAVA::Map<DAVA::Texture*, DAVA::Vector<DAVA::eGPUFamily>>::iterator it = textures.begin(); it != textures.end(); ++it)
    {
        DAVA::TextureDescriptor* descriptor = it->first->GetDescriptor();

        if (nullptr == descriptor)
        {
            continue;
        }

        const DAVA::Vector<DAVA::eGPUFamily>& updatedGPUs = it->second;
        WaitSetMessage(descriptor->GetSourceTexturePathname().GetAbsolutePathname().c_str());
        for (DAVA::eGPUFamily gpu : updatedGPUs)
        {
            DAVA::TextureConverter::ConvertTexture(*descriptor, gpu, true, quality);

            DAVA::TexturesMap texturesMap = DAVA::Texture::GetTextureMap();
            DAVA::TexturesMap::iterator found = texturesMap.find(FILEPATH_MAP_KEY(descriptor->pathname));
            if (found != texturesMap.end())
            {
                DAVA::Texture* tex = found->second;
                tex->Reload();
            }

            WaitSetValue(++convretedNumber);
        }
    }

    WaitStop();
}

void QtMainWindow::OnGlobalInvalidateTimeout()
{
    emit GlobalInvalidateTimeout();
    if (globalInvalidate)
    {
        StartGlobalInvalidateTimer();
    }
}

void QtMainWindow::EnableGlobalTimeout(bool enable)
{
    if (globalInvalidate != enable)
    {
        globalInvalidate = enable;

        if (globalInvalidate)
        {
            StartGlobalInvalidateTimer();
        }
    }
}

void QtMainWindow::StartGlobalInvalidateTimer()
{
    QTimer::singleShot(GLOBAL_INVALIDATE_TIMER_DELTA, this, SLOT(OnGlobalInvalidateTimeout()));
}

void QtMainWindow::OnLandscapeEditorToggled(DAVA::SceneEditor2* scene)
{
    if (scene != MainWindowDetails::GetCurrentScene())
    {
        return;
    }

    ui->actionCustomColorsEditor->setChecked(false);
    ui->actionHeightMapEditor->setChecked(false);
    ui->actionRulerTool->setChecked(false);
    ui->actionTileMapEditor->setChecked(false);
    ui->actionShowNotPassableLandscape->setChecked(false);

    DAVA::int32 tools = scene->GetEnabledTools();

    bool anyEditorEnabled = false;
    if (tools & DAVA::SceneEditor2::LANDSCAPE_TOOL_CUSTOM_COLOR)
    {
        ui->actionCustomColorsEditor->setChecked(true);
        anyEditorEnabled = true;
    }
    if (tools & DAVA::SceneEditor2::LANDSCAPE_TOOL_HEIGHTMAP_EDITOR)
    {
        ui->actionHeightMapEditor->setChecked(true);
        anyEditorEnabled = true;
    }
    if (tools & DAVA::SceneEditor2::LANDSCAPE_TOOL_RULER)
    {
        ui->actionRulerTool->setChecked(true);
        anyEditorEnabled = true;
    }
    if (tools & DAVA::SceneEditor2::LANDSCAPE_TOOL_TILEMAP_EDITOR)
    {
        ui->actionTileMapEditor->setChecked(true);
        anyEditorEnabled = true;
    }
    if (tools & DAVA::SceneEditor2::LANDSCAPE_TOOL_NOT_PASSABLE_TERRAIN)
    {
        ui->actionShowNotPassableLandscape->setChecked(true);
        anyEditorEnabled = true;
    }

    if (anyEditorEnabled)
    {
        SetVisibilityToolEnabledIfPossible(false);
    }
    ui->actionForceFirstLODonLandscape->setChecked(anyEditorEnabled);
    OnForceFirstLod(anyEditorEnabled);

    UpdateLandscapeRenderMode();
}

void QtMainWindow::OnCustomColorsEditor()
{
    DAVA::RefPtr<DAVA::SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (!sceneEditor)
    {
        return;
    }

    if (sceneEditor->GetSystem<DAVA::CustomColorsSystem>()->IsLandscapeEditingEnabled())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DAVA::DisableCustomColorsCommand(sceneEditor.Get(), true)));
    }
    else if (CanEnableLandscapeEditor())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DAVA::EnableCustomColorsCommand(sceneEditor.Get(), true)));
    }
    OnLandscapeEditorToggled(sceneEditor.Get());
}

void QtMainWindow::OnHeightmapEditor()
{
    DAVA::RefPtr<DAVA::SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (!sceneEditor)
    {
        return;
    }

    if (sceneEditor->GetSystem<DAVA::HeightmapEditorSystem>()->IsLandscapeEditingEnabled())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DAVA::DisableHeightmapEditorCommand(sceneEditor.Get())));
    }
    else if (CanEnableLandscapeEditor())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DAVA::EnableHeightmapEditorCommand(sceneEditor.Get())));
    }
    OnLandscapeEditorToggled(sceneEditor.Get());
}

void QtMainWindow::OnRulerTool()
{
    DAVA::RefPtr<DAVA::SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (!sceneEditor)
    {
        return;
    }

    if (sceneEditor->GetSystem<DAVA::RulerToolSystem>()->IsLandscapeEditingEnabled())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DAVA::DisableRulerToolCommand(sceneEditor.Get())));
    }
    else if (CanEnableLandscapeEditor())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DAVA::EnableRulerToolCommand(sceneEditor.Get())));
    }
    OnLandscapeEditorToggled(sceneEditor.Get());
}

void QtMainWindow::OnTilemaskEditor()
{
    DAVA::RefPtr<DAVA::SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (!sceneEditor)
    {
        return;
    }

    if (sceneEditor->GetSystem<DAVA::TilemaskEditorSystem>()->IsLandscapeEditingEnabled())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DAVA::DisableTilemaskEditorCommand(sceneEditor.Get())));
    }
    else if (CanEnableLandscapeEditor())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DAVA::EnableTilemaskEditorCommand(sceneEditor.Get())));
    }
    OnLandscapeEditorToggled(sceneEditor.Get());
}

void QtMainWindow::OnForceFirstLod(bool enabled)
{
    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() == nullptr)
    {
        ui->actionForceFirstLODonLandscape->setChecked(false);
        return;
    }

    auto landscape = FindLandscape(scene.Get());
    if (landscape == nullptr)
    {
        ui->actionForceFirstLODonLandscape->setChecked(false);
        return;
    }

    landscape->SetForceMaxSubdiv(enabled);
    scene->GetSystem<DAVA::VisibilityCheckSystem>()->Recalculate();
}

void QtMainWindow::OnNotPassableTerrain()
{
    DAVA::RefPtr<DAVA::SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (!sceneEditor)
    {
        return;
    }

    if (sceneEditor->GetSystem<DAVA::LandscapeEditorDrawSystem>()->IsNotPassableTerrainEnabled())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DAVA::DisableNotPassableCommand(sceneEditor.Get())));
    }
    else if (CanEnableLandscapeEditor())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DAVA::EnableNotPassableCommand(sceneEditor.Get())));
    }
    OnLandscapeEditorToggled(sceneEditor.Get());
}

void QtMainWindow::OnWayEditor()
{
    DAVA::RefPtr<DAVA::SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (!sceneEditor)
    {
        return;
    }

    if (sceneEditor->GetSystem<DAVA::PathSystem>()->IsPathEditEnabled())
    {
        sceneEditor->Exec(std::make_unique<DAVA::DisableWayEditCommand>(sceneEditor.Get()));
    }
    else
    {
        DAVA::int32 toolsEnabled = sceneEditor->GetEnabledTools();
        if (toolsEnabled)
        {
            DAVA::Logger::Error("Landscape tools should be disabled prior to enabling WayEditor");
            ui->actionWayEditor->setChecked(false);
        }
        else
        {
            sceneEditor->Exec(std::make_unique<DAVA::EnableWayEditCommand>(sceneEditor.Get()));
        }
    }
}

void QtMainWindow::OnBakeWaypoints()
{
    MainWindowDetails::GetCurrentScene()->GetSystem<DAVA::PathSystem>()->BakeWaypoints();
}

void QtMainWindow::OnBuildStaticOcclusion()
{
    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (!scene)
        return;

    DAVA::WaitDialogParams params;
    params.cancelable = true;
    params.message = "Please wait while building static occlusion.";
    params.needProgressBar = true;
    params.min = 0;
    params.max = 100;

    std::unique_ptr<DAVA::WaitHandle> waitHandle = DAVA::Deprecated::GetUI()->ShowWaitDialog(DAVA::mainWindowKey, params);

    bool sceneWasChanged = true;
    DAVA::StaticOcclusionBuildSystem* soBuildSystem = scene->GetSystem<DAVA::StaticOcclusionBuildSystem>();
    soBuildSystem->Build();
    while (soBuildSystem->IsInBuild())
    {
        if (waitHandle->WasCanceled())
        {
            soBuildSystem->Cancel();
            sceneWasChanged = false;
        }
        else
        {
            waitHandle->SetProgressValue(soBuildSystem->GetBuildStatus());
            waitHandle->SetMessage(QString::fromStdString(soBuildSystem->GetBuildStatusInfo()));
        }
    }

    if (sceneWasChanged)
    {
        scene->MarkAsChanged();

        if (DAVA::Deprecated::GetDataNode<DAVA::GlobalSceneSettings>()->saveStaticOcclusion)
        {
            DAVA::Deprecated::GetInvoker()->Invoke(DAVA::SaveCurrentScene.ID);
        }
    }
}

void QtMainWindow::OnInavalidateStaticOcclusion()
{
    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (!scene)
        return;
    scene->staticOcclusionSystem->InvalidateOcclusion();
    scene->MarkAsChanged();
}

void QtMainWindow::OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields)
{
    if (projectDataWrapper == wrapper)
    {
        DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();

        // empty fields mean that ProjectManagerData was just added
        // this means that there is a first call
        if (data != nullptr && fields.empty())
        {
            const DAVA::SpritesPackerModule* spritesPacker = data->GetSpritesModules();
            QObject::connect(spritesPacker, &DAVA::SpritesPackerModule::SpritesReloaded, ui->sceneInfo, &SceneInfo::SpritesReloaded);
        }

        if (data != nullptr && !data->GetProjectPath().IsEmpty())
        {
            EnableProjectActions(true);
            SetupTitle(data->GetProjectPath().GetAbsolutePathname());
        }
        else
        {
            EnableProjectActions(false);
            SetupTitle(DAVA::String());
        }
    }
}

void QtMainWindow::OnMaterialLightViewChanged(bool)
{
    int newMode = DAVA::CommonInternalSettings::LIGHTVIEW_NOTHING;

    if (ui->actionAlbedo->isChecked())
        newMode |= DAVA::CommonInternalSettings::LIGHTVIEW_ALBEDO;
    if (ui->actionDiffuse->isChecked())
        newMode |= DAVA::CommonInternalSettings::LIGHTVIEW_DIFFUSE;
    if (ui->actionAmbient->isChecked())
        newMode |= DAVA::CommonInternalSettings::LIGHTVIEW_AMBIENT;
    if (ui->actionSpecular->isChecked())
        newMode |= DAVA::CommonInternalSettings::LIGHTVIEW_SPECULAR;

    DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->materialLightViewMode = static_cast<DAVA::CommonInternalSettings::MaterialLightViewMode>(newMode);
    if (MainWindowDetails::GetCurrentScene().Get() != nullptr)
    {
        MainWindowDetails::GetCurrentScene()->GetSystem<DAVA::EditorMaterialSystem>()->SetLightViewMode(newMode);
    }
}

void QtMainWindow::OnCustomQuality()
{
    QualitySwitcher::ShowDialog();
}

bool QtMainWindow::CanEnableLandscapeEditor() const
{
    using namespace DAVA;

    RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() == nullptr)
        return false;

    if (scene->GetSystem<DAVA::PathSystem>()->IsPathEditEnabled())
    {
        Logger::Error("WayEditor should be disabled prior to enabling landscape tools");
        return false;
    }

    FileSystem* fs = GetEngineContext()->fileSystem;
    if (fs->GetFilenamesTag().empty() == false)
    {
        Logger::Error("Could not enable editing of landscape due to enabled tags at file system");
        return false;
    }

    LandscapeEditorDrawSystem* landDrawSystem = scene->GetSystem<LandscapeEditorDrawSystem>();
    if (landDrawSystem == nullptr)
    {
        Logger::Error("Could not enable editing of landscape without landscape draw system");
        return false;
    }

    LandscapeEditorDrawSystem::eErrorType verificationResult = landDrawSystem->VerifyLandscape();
    if (verificationResult != LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS)
    {
        Logger::Error(LandscapeEditorDrawSystem::GetDescriptionByError(verificationResult).c_str());
        return false;
    }

    return LoadAppropriateTextureFormat();
}

bool QtMainWindow::LoadAppropriateTextureFormat() const
{
    DAVA::CommonInternalSettings* settings = DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>();
    DAVA::eGPUFamily gpuFormat = settings->textureViewGPU;
    if (gpuFormat != DAVA::GPU_ORIGIN)
    {
        DAVA::ModalMessageParams params;
        params.title = "Inappropriate texture format";
        params.message = "Landscape editing is only allowed in original texture format.\nDo you want to reload textures in original format?";
        params.icon = DAVA::ModalMessageParams::Question;
        params.buttons = DAVA::ModalMessageParams::Yes | DAVA::ModalMessageParams::No;
        params.defaultButton = DAVA::ModalMessageParams::No;
        DAVA::ModalMessageParams::Button answer = DAVA::Deprecated::GetUI()->ShowModalMessage(DAVA::mainWindowKey, params);
        if (answer == DAVA::ModalMessageParams::No)
        {
            return false;
        }

        DAVA::Deprecated::GetInvoker()->Invoke(DAVA::ReloadAllTextures.ID, DAVA::eGPUFamily::GPU_ORIGIN);
    }

    return settings->textureViewGPU == DAVA::GPU_ORIGIN;
}

void QtMainWindow::DebugVersionInfo()
{
    if (!versionInfoWidget)
    {
        versionInfoWidget = new VersionInfoWidget(this);
        versionInfoWidget->setWindowFlags(Qt::Window);
        versionInfoWidget->setAttribute(Qt::WA_DeleteOnClose);
    }

    versionInfoWidget->show();
}

void QtMainWindow::OnConsoleItemClicked(const QString& data)
{
    PointerSerializer conv(data.toStdString());
    if (conv.CanConvert<DAVA::Entity*>())
    {
        DAVA::RefPtr<DAVA::SceneEditor2> currentScene = MainWindowDetails::GetCurrentScene();
        if (currentScene.Get() != nullptr)
        {
            auto vec = conv.GetPointers<DAVA::Entity*>();
            if (!vec.empty())
            {
                DAVA::SelectableGroup objects;
                DAVA::Vector<DAVA::Entity*> allEntities;
                currentScene->GetChildNodes(allEntities);
                for (auto entity : vec)
                {
                    if (std::find(allEntities.begin(), allEntities.end(), entity) != allEntities.end())
                    {
                        objects.Add(entity, currentScene->GetSystem<DAVA::SceneCollisionSystem>()->GetUntransformedBoundingBox(entity));
                    }
                }

                if (!objects.IsEmpty())
                {
                    currentScene->GetSystem<DAVA::SelectionSystem>()->SetSelection(objects);
                    currentScene->GetSystem<DAVA::SceneCameraSystem>()->LookAt(objects.GetIntegralBoundingBox());
                }
            }
        }
    }
}

void QtMainWindow::OnGenerateHeightDelta()
{
    HeightDeltaTool* w = new HeightDeltaTool(this);
    w->setWindowFlags(Qt::Window);
    w->setAttribute(Qt::WA_DeleteOnClose);

    w->show();
}

void QtMainWindow::OnSnapCameraToLandscape(bool snap)
{
    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (!scene)
        return;

    bool toggleProcessed = false;
    DAVA::SceneCameraSystem* system = scene->GetSystem<DAVA::SceneCameraSystem>();
    if (system)
    {
        toggleProcessed = system->SnapEditorCameraToLandscape(snap);
    }

    if (toggleProcessed == false)
    {
        ui->actionSnapCameraToLandscape->setChecked(!snap);
    }
}

void QtMainWindow::SetActionCheckedSilently(QAction* action, bool checked)
{
    DVASSERT(action);

    bool b = action->blockSignals(true);
    action->setChecked(checked);
    action->blockSignals(b);
}

bool QtMainWindow::SetVisibilityToolEnabledIfPossible(bool enabled)
{
    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    DVASSERT(scene.Get() != nullptr, "Switching visibility tool requires an opened scene");

    DAVA::int32 enabledTools = scene->GetEnabledTools();
    if (enabled && (enabledTools != 0))
    {
        DAVA::Logger::Error("Please disable Landscape editing tools before enabling Visibility Check System");
        enabled = false;
    }

    DAVA::Renderer::GetOptions()->SetOption(DAVA::RenderOptions::DEBUG_ENABLE_VISIBILITY_SYSTEM, enabled);

    if (enabled)
    {
        ui->actionForceFirstLODonLandscape->setChecked(true);
        OnForceFirstLod(true);
    }

    ui->actionEnableVisibilitySystem->setChecked(enabled);
    UpdateLandscapeRenderMode();

    return enabled;
}

void QtMainWindow::UpdateLandscapeRenderMode()
{
    DAVA::RefPtr<DAVA::SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    DAVA::Landscape* landscape = FindLandscape(scene.Get());
    if (landscape != nullptr)
    {
        bool visibiilityEnabled = DAVA::Renderer::GetOptions()->IsOptionEnabled(DAVA::RenderOptions::DEBUG_ENABLE_VISIBILITY_SYSTEM);
        bool anyToolEnabled = scene->GetEnabledTools() != 0;
        bool enableInstancing = anyToolEnabled || !visibiilityEnabled;

        if (anyToolEnabled)
        {
            DVASSERT(visibiilityEnabled == false);
        }
        if (visibiilityEnabled)
        {
            DVASSERT(anyToolEnabled == false);
        }

        DAVA::Landscape::RenderMode newRenderMode = (enableInstancing && rhi::DeviceCaps().isInstancingSupported) ?
        DAVA::Landscape::RenderMode::RENDERMODE_INSTANCING_MORPHING :
        DAVA::Landscape::RenderMode::RENDERMODE_NO_INSTANCING;

        landscape->SetRenderMode(newRenderMode);
    }
}

bool QtMainWindow::ParticlesArePacking() const
{
    DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
    DVASSERT(data != nullptr);
    return data->GetSpritesModules()->IsRunning();
}

void QtMainWindow::OnValidateScene()
{
    DAVA::RefPtr<DAVA::SceneEditor2> currentScene = MainWindowDetails::GetCurrentScene();
    DVASSERT(currentScene.Get() != nullptr);

    SceneValidationDialog dlg(currentScene.Get());
    dlg.exec();
}
