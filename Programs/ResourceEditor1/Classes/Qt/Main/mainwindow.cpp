#include "mainwindow.h"

#include "Classes/Qt/CubemapEditor/CubeMapTextureBrowser.h"
#include "Classes/Qt/CubemapEditor/CubemapUtils.h"
#include "Classes/Qt/DebugTools/VersionInfoWidget/VersionInfoWidget.h"
#include "Classes/Qt/ImageSplitterDialog/ImageSplitterDialog.h"
#include "Classes/Qt/Main/QtUtils.h"
#include "Classes/Qt/MaterialEditor/MaterialEditor.h"
#include "Classes/Qt/QualitySwitcher/QualitySwitcher.h"
#include "Classes/Qt/RunActionEventWidget/RunActionEventWidget.h"
#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/Qt/Scene/SceneHelper.h"
#include "Classes/Qt/Scene/System/EditorVegetationSystem.h"
#include "Classes/Qt/Scene/System/VisibilityCheckSystem/VisibilityCheckSystem.h"
#include "Classes/Qt/Scene/System/EditorVegetationSystem.h"
#include "Classes/Qt/Scene/Validation/SceneValidationDialog.h"
#include "Classes/Qt/SpritesPacker/SpritesPackerModule.h"
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
#include "Classes/Project/ProjectManagerData.h"
#include "Classes/Application/REGlobal.h"
#include "Classes/Application/REGlobalOperationsData.h"
#include "Classes/Application/RESettings.h"
#include "Classes/Application/FileSystemData.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/Selection/Selection.h"
#include "Classes/Selection/SelectionData.h"

#include "Deprecated/EditorConfig.h"
#include "Deprecated/SceneValidator.h"

#include "Utils/SceneSaver/SceneSaver.h"
#include "Commands2/Base/RECommandStack.h"
#include "Commands2/Base/RECommandBatch.h"
#include "Commands2/Base/RECommandNotificationObject.h"
#include "Commands2/AddComponentCommand.h"
#include "Commands2/CustomColorsCommands2.h"
#include "Commands2/EntityAddCommand.h"
#include "Commands2/HeightmapEditorCommands2.h"
#include "Commands2/RemoveComponentCommand.h"
#include "Commands2/TilemaskEditorCommands.h"
#include "Commands2/LandscapeToolsToggleCommand.h"
#include "Commands2/WayEditCommands.h"

#include "SceneProcessing/SceneProcessor.h"

#include "Constants.h"
#include "StringConstants.h"

#include <TextureCompression/TextureConverter.h>
#include <Version/Version.h>

#include <QtTools/ConsoleWidget/LogWidget.h>
#include <QtTools/ConsoleWidget/LogModel.h>
#include <QtTools/ConsoleWidget/PointerSerializer.h>
#include <QtTools/ConsoleWidget/LoggerOutputObject.h>
#include <QtTools/FileDialogs/FileDialog.h>
#include <QtTools/FileDialogs/FindFileDialog.h>

#include <TArc/WindowSubSystem/Private/WaitDialog.h>
#include <TArc/Core/FieldBinder.h>
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
#include <Scene3D/Systems/StaticOcclusionSystem.h>
#include <Scene/System/EditorVegetationSystem.h>
#include <Utils/StringFormat.h>

#include <QActionGroup>
#include <QColorDialog>
#include <QDesktopServices>
#include <QKeySequence>
#include <QMessageBox>
#include <QMetaObject>
#include <QMetaType>
#include <QList>

#define CHECK_GLOBAL_OPERATIONS(retValue) \
    if (globalOperations == nullptr) \
    {\
        DAVA::Logger::Error("GlobalOperationsProxy call after MainWindow was destroyed"); \
        return (retValue);\
    }

namespace MainWindowDetails
{
class GlobalOperationsProxy : public GlobalOperations
{
public:
    GlobalOperationsProxy(GlobalOperations* globalOperations_)
        : globalOperations(globalOperations_)
    {
    }

    void Reset()
    {
        globalOperations = nullptr;
    }

    void CallAction(ID id, DAVA::Any&& args) override
    {
        CHECK_GLOBAL_OPERATIONS(void());
        globalOperations->CallAction(id, std::move(args));
    }

    QWidget* GetGlobalParentWidget() const override
    {
        CHECK_GLOBAL_OPERATIONS(nullptr);
        return globalOperations->GetGlobalParentWidget();
    }

    void ShowWaitDialog(const DAVA::String& tittle, const DAVA::String& message, DAVA::uint32 min = 0, DAVA::uint32 max = 100) override
    {
        CHECK_GLOBAL_OPERATIONS(void());
        globalOperations->ShowWaitDialog(tittle, message, min, max);
    }

    void HideWaitDialog() override
    {
        CHECK_GLOBAL_OPERATIONS(void());
        globalOperations->HideWaitDialog();
    }

    void ForEachScene(const DAVA::Function<void(SceneEditor2*)>& functor) override
    {
        CHECK_GLOBAL_OPERATIONS(void());
        globalOperations->ForEachScene(functor);
    }

private:
    GlobalOperations* globalOperations;
};

bool IsSavingAllowed(const QString& warningTitle)
{
    SceneData* data = REGlobal::GetActiveDataNode<SceneData>();
    DVASSERT(data);
    QString message;
    bool result = data->IsSavingAllowed(&message);
    if (result == false)
    {
        using namespace DAVA::TArc;

        ModalMessageParams params;
        params.buttons = ModalMessageParams::Ok;
        params.message = message;
        params.title = warningTitle;
        REGlobal::ShowModalMessage(params);
    }

    return result;
}

DAVA::RefPtr<SceneEditor2> GetCurrentScene()
{
    SceneData* data = REGlobal::GetActiveDataNode<SceneData>();
    DAVA::RefPtr<SceneEditor2> result;
    if (data)
    {
        result = data->GetScene();
    }
    return result;
}
}

QtMainWindow::QtMainWindow(DAVA::TArc::UI* tarcUI_, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , globalInvalidate(false)
    , modificationWidget(nullptr)
    , developerTools(new DeveloperTools(this))
#if defined(__DAVAENGINE_MACOS__)
    , shortcutChecker(this)
#endif
    , tarcUI(tarcUI_)
{
    projectDataWrapper = REGlobal::CreateDataWrapper(DAVA::ReflectedTypeDB::Get<ProjectManagerData>());
    projectDataWrapper.SetListener(this);

    fieldBinderTagged.reset(REGlobal::CreateFieldBinder());
    { // bind to changed data
        DAVA::TArc::FieldDescriptor sceneFieldDescr;
        sceneFieldDescr.type = DAVA::ReflectedTypeDB::Get<SceneData>();
        sceneFieldDescr.fieldName = DAVA::FastName(SceneData::scenePropertyName);

        DAVA::TArc::FieldDescriptor fsFieldDescr;
        fsFieldDescr.type = DAVA::ReflectedTypeDB::Get<FileSystemData>();
        fsFieldDescr.fieldName = DAVA::FastName("tag");

        fieldBinderTagged->BindField(sceneFieldDescr, DAVA::MakeFunction(this, &QtMainWindow::UpdateTagDependentActionsState));
        fieldBinderTagged->BindField(fsFieldDescr, DAVA::MakeFunction(this, &QtMainWindow::UpdateTagDependentActionsState));
    }

    globalOperations.reset(new MainWindowDetails::GlobalOperationsProxy(this));

    DAVA::TArc::DataContext* globalContext = REGlobal::GetGlobalContext();
    std::unique_ptr<REGlobalOperationsData> globalData = std::make_unique<REGlobalOperationsData>();
    globalData->SetGlobalOperations(globalOperations);
    globalContext->CreateData(std::move(globalData));

    errorLoggerOutput = new ErrorDialogOutput(tarcUI);
    DAVA::Logger::AddCustomOutput(errorLoggerOutput);

    new LandscapeEditorShortcutManager(this);
    PathDescriptor::InitializePathDescriptors();

    ui->setupUi(this);
    setObjectName("ResourceEditor"); //we need to support old names to save settings

    SetupWidget();
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
    connect(SceneSignals::Instance(), &SceneSignals::EditorLightEnabled, this, &QtMainWindow::EditorLightEnabled);

    selectionWrapper = REGlobal::CreateDataWrapper(DAVA::ReflectedTypeDB::Get<SelectionData>());
    selectionWrapper.SetListener(this);

    LoadMaterialLightViewMode();

    EnableGlobalTimeout(globalInvalidate);

    EnableProjectActions(false);
    EnableSceneActions(false);

    SynchronizeStateWithUI();
}

QtMainWindow::~QtMainWindow()
{
    errorLoggerOutput->Disable();
    errorLoggerOutput = nullptr; // will be deleted by DAVA::Logger;

    LogWidget* logWidget = qobject_cast<LogWidget*>(dockConsole->widget());
    QByteArray dataToSave = logWidget->Serialize();

    REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->logWidgetState = dataToSave;

    TextureBrowser::Instance()->Release();
    MaterialEditor::Instance()->Release();

    LandscapeEditorShortcutManager::Instance()->Release();

    std::static_pointer_cast<MainWindowDetails::GlobalOperationsProxy>(globalOperations)->Reset();
    globalOperations.reset();
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

void QtMainWindow::SetupWidget()
{
    ui->scrollAreaWidgetContents->Init(globalOperations);
}

void QtMainWindow::WaitStart(const QString& title, const QString& message, int min, int max)
{
    DVASSERT(waitDialog == nullptr);

    DAVA::TArc::WaitDialogParams params;
    params.message = message;
    params.min = min;
    params.max = max;
    params.needProgressBar = min != max;
    waitDialog = tarcUI->ShowWaitDialog(DAVA::TArc::mainWindowKey, params);
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
    QAction* actionModifToolBar = ui->modificationToolBar->toggleViewAction();
    QAction* actionLandscapeToolbar = ui->landscapeToolBar->toggleViewAction();

    ui->Toolbars->addAction(actionMainToolBar);
    ui->Toolbars->addAction(actionModifToolBar);
    ui->Toolbars->addAction(actionLandscapeToolbar);
    ui->Toolbars->addAction(ui->sceneToolBar->toggleViewAction());
    ui->Toolbars->addAction(ui->cameraToolBar->toggleViewAction());

    // modification widget
    modificationWidget = new ModificationWidget(nullptr);
    ui->modificationToolBar->insertWidget(ui->actionModifyReset, modificationWidget);

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

    DAVA::TArc::InsertionParams insertParams;
    insertParams.method = DAVA::TArc::InsertionParams::eInsertionMethod::BeforeItem;
    DAVA::TArc::ActionPlacementInfo placementInfo(DAVA::TArc::CreateStatusbarPoint(true, 0, insertParams));

    tarcUI->AddAction(DAVA::TArc::mainWindowKey, placementInfo, ui->actionShowEditorGizmo);
    tarcUI->AddAction(DAVA::TArc::mainWindowKey, placementInfo, ui->actionLightmapCanvas);
    tarcUI->AddAction(DAVA::TArc::mainWindowKey, placementInfo, ui->actionShowStaticOcclusion);
    tarcUI->AddAction(DAVA::TArc::mainWindowKey, placementInfo, ui->actionEnableVisibilitySystem);
    tarcUI->AddAction(DAVA::TArc::mainWindowKey, placementInfo, ui->actionEnableDisableShadows);
    tarcUI->AddAction(DAVA::TArc::mainWindowKey, placementInfo, ui->actionEnableSounds);
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
        logWidget->Deserialize(REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->logWidgetState);
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

    OnEditorGizmoToggle(REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->gizmoEnabled);
    QObject::connect(ui->actionShowEditorGizmo, SIGNAL(toggled(bool)), this, SLOT(OnEditorGizmoToggle(bool)));

    QObject::connect(ui->actionLightmapCanvas, SIGNAL(toggled(bool)), this, SLOT(OnViewLightmapCanvas(bool)));
    QObject::connect(ui->actionShowStaticOcclusion, SIGNAL(toggled(bool)), this, SLOT(OnShowStaticOcclusionToggle(bool)));
    QObject::connect(ui->actionEnableVisibilitySystem, SIGNAL(triggered(bool)), this, SLOT(OnEnableVisibilitySystemToggle(bool)));

    QObject::connect(ui->actionRefreshVisibilitySystem, SIGNAL(triggered()), this, SLOT(OnRefreshVisibilitySystem()));
    QObject::connect(ui->actionFixCurrentFrame, SIGNAL(triggered()), this, SLOT(OnFixVisibilityFrame()));
    QObject::connect(ui->actionReleaseCurrentFrame, SIGNAL(triggered()), this, SLOT(OnReleaseVisibilityFrame()));

    QObject::connect(ui->actionEnableDisableShadows, &QAction::toggled, this, &QtMainWindow::OnEnableDisableShadows);

    EnableSounds(REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->enableSound);
    QObject::connect(ui->actionEnableSounds, &QAction::toggled, this, &QtMainWindow::EnableSounds);

    // quality
    QObject::connect(ui->actionCustomQuality, SIGNAL(triggered()), this, SLOT(OnCustomQuality()));

    // scene modifications
    QObject::connect(ui->actionModifySelect, SIGNAL(triggered()), this, SLOT(OnSelectMode()));
    QObject::connect(ui->actionModifyMove, SIGNAL(triggered()), this, SLOT(OnMoveMode()));
    QObject::connect(ui->actionModifyRotate, SIGNAL(triggered()), this, SLOT(OnRotateMode()));
    QObject::connect(ui->actionModifyScale, SIGNAL(triggered()), this, SLOT(OnScaleMode()));
    QObject::connect(ui->actionPivotCenter, SIGNAL(triggered()), this, SLOT(OnPivotCenterMode()));
    QObject::connect(ui->actionPivotCommon, SIGNAL(triggered()), this, SLOT(OnPivotCommonMode()));
    QObject::connect(ui->actionManualModifMode, SIGNAL(triggered()), this, SLOT(OnManualModifMode()));
    QObject::connect(ui->actionModifyReset, SIGNAL(triggered()), this, SLOT(OnResetTransform()));
    QObject::connect(ui->actionLockTransform, SIGNAL(triggered()), this, SLOT(OnLockTransform()));
    QObject::connect(ui->actionUnlockTransform, SIGNAL(triggered()), this, SLOT(OnUnlockTransform()));
    QObject::connect(ui->actionCenterPivotPoint, SIGNAL(triggered()), this, SLOT(OnCenterPivotPoint()));
    QObject::connect(ui->actionZeroPivotPoint, SIGNAL(triggered()), this, SLOT(OnZeroPivotPoint()));

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

    QObject::connect(ui->actionSaveHeightmapToPNG, SIGNAL(triggered()), this, SLOT(OnSaveHeightmapToImage()));
    QObject::connect(ui->actionSaveTiledTexture, SIGNAL(triggered()), this, SLOT(OnSaveTiledTexture()));
    QObject::connect(ui->actionConvertModifiedTextures, SIGNAL(triggered()), this, SLOT(OnConvertModifiedTextures()));
    QObject::connect(ui->actionBuildStaticOcclusion, SIGNAL(triggered()), this, SLOT(OnBuildStaticOcclusion()));
    QObject::connect(ui->actionInvalidateStaticOcclusion, SIGNAL(triggered()), this, SLOT(OnInavalidateStaticOcclusion()));

    connect(ui->actionHeightmap_Delta_Tool, SIGNAL(triggered()), this, SLOT(OnGenerateHeightDelta()));

    //Help
    QObject::connect(ui->actionHelp, SIGNAL(triggered()), this, SLOT(OnOpenHelp()));

    //Landscape editors toggled
    QObject::connect(SceneSignals::Instance(), SIGNAL(LandscapeEditorToggled(SceneEditor2*)),
                     this, SLOT(OnLandscapeEditorToggled(SceneEditor2*)));

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

    QObject::connect(ui->actionBatchProcess, SIGNAL(triggered(bool)), this, SLOT(OnBatchProcessScene()));

    QObject::connect(ui->actionSnapCameraToLandscape, SIGNAL(triggered(bool)), this, SLOT(OnSnapCameraToLandscape(bool)));

    QObject::connect(ui->actionValidateScene, SIGNAL(triggered()), this, SLOT(OnValidateScene()));
}

// ###################################################################################################
// Scene signals
// ###################################################################################################

void QtMainWindow::SceneActivated(SceneEditor2* scene)
{
    EnableSceneActions(true);

    LoadViewState(scene);
    LoadModificationState(scene);
    LoadEditorLightState(scene);
    LoadLandscapeEditorState(scene);

    OnMaterialLightViewChanged(true);
    OnViewLightmapCanvas(true);

    UpdateModificationActionsState();

    ui->actionSnapCameraToLandscape->setChecked(false);
    if (nullptr != scene)
    {
        scene->SetHUDVisible(ui->actionShowEditorGizmo->isChecked());

        if (scene->cameraSystem)
            ui->actionSnapCameraToLandscape->setChecked(scene->cameraSystem->IsEditorCameraSnappedToLandscape());
    }
}

void QtMainWindow::SceneDeactivated(SceneEditor2* scene)
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

    ui->actionModifySelect->setEnabled(enable);
    ui->actionModifyMove->setEnabled(enable);
    ui->actionModifyReset->setEnabled(enable);
    ui->actionModifyRotate->setEnabled(enable);
    ui->actionModifyScale->setEnabled(enable);
    ui->actionPivotCenter->setEnabled(enable);
    ui->actionPivotCommon->setEnabled(enable);
    ui->actionCenterPivotPoint->setEnabled(enable);
    ui->actionZeroPivotPoint->setEnabled(enable);
    ui->actionManualModifMode->setEnabled(enable);

    if (modificationWidget)
        modificationWidget->setEnabled(enable);

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
    bool enable = (tag.empty() == true) && (REGlobal::GetActiveDataNode<SceneData>() != nullptr);

    ui->actionTextureConverter->setEnabled(enable);
    ui->actionMaterialEditor->setEnabled(enable);
    ui->actionHeightMapEditor->setEnabled(enable);
    ui->actionTileMapEditor->setEnabled(enable);
    ui->actionShowNotPassableLandscape->setEnabled(enable);
    ui->actionRulerTool->setEnabled(enable);
    ui->actionCustomColorsEditor->setEnabled(enable);
}

void QtMainWindow::UpdateModificationActionsState()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    bool isMultiple = (scene.Get() != nullptr) && (scene->modifSystem->GetTransformableSelection().GetSize() > 1);

    // modificationWidget determines inside, if values could be modified and enables/disables itself
    modificationWidget->ReloadValues();
    bool canModify = modificationWidget->isEnabled();

    ui->actionModifyReset->setEnabled(canModify);
    ui->actionCenterPivotPoint->setEnabled(canModify && !isMultiple);
    ui->actionZeroPivotPoint->setEnabled(canModify && !isMultiple);
}

void QtMainWindow::UpdateWayEditor(const RECommandNotificationObject& commandNotification)
{
    if (commandNotification.MatchCommandID(CMDID_ENABLE_WAYEDIT))
    {
        DVASSERT(commandNotification.MatchCommandID(CMDID_DISABLE_WAYEDIT) == false);
        SetActionCheckedSilently(ui->actionWayEditor, commandNotification.redo);
    }
    else if (commandNotification.MatchCommandID(CMDID_DISABLE_WAYEDIT))
    {
        DVASSERT(commandNotification.MatchCommandID(CMDID_ENABLE_WAYEDIT) == false);
        SetActionCheckedSilently(ui->actionWayEditor, !commandNotification.redo);
    }
}

void QtMainWindow::SceneCommandExecuted(SceneEditor2* scene, const RECommandNotificationObject& commandNotification)
{
    if (scene == MainWindowDetails::GetCurrentScene())
    {
        UpdateModificationActionsState();
        UpdateWayEditor(commandNotification);

        auto updateCameraState = [this, scene](const RECommand* command)
        {
            DAVA::Entity* entity = nullptr;
            if (command->GetID() == CMDID_COMPONENT_ADD)
            {
                const AddComponentCommand* addCommand = static_cast<const AddComponentCommand*>(command);
                entity = addCommand->GetEntity();
            }
            else if (command->GetID() == CMDID_COMPONENT_REMOVE)
            {
                const RemoveComponentCommand* removeCommand = static_cast<const RemoveComponentCommand*>(command);
                entity = removeCommand->GetEntity();
            }
            if (entity != nullptr && entity->GetName() == ResourceEditor::EDITOR_DEBUG_CAMERA)
            {
                SetActionCheckedSilently(ui->actionSnapCameraToLandscape, scene->cameraSystem->IsEditorCameraSnappedToLandscape());
                return true;
            }
            return false;
        };

        if (commandNotification.batch != nullptr)
        {
            for (DAVA::uint32 i = 0, count = commandNotification.batch->Size(); i < count; ++i)
            {
                if (updateCameraState(commandNotification.batch->GetCommand(i)))
                {
                    break;
                }
            }
        }
        else
        {
            updateCameraState(commandNotification.command);
        }
    }
}

// ###################################################################################################
// Mainwindow Qt actions
// ###################################################################################################
void QtMainWindow::OnEditorGizmoToggle(bool show)
{
    ui->actionShowEditorGizmo->setChecked(show);
    REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->gizmoEnabled = show;
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        scene->SetHUDVisible(show);
    }
}

void QtMainWindow::OnViewLightmapCanvas(bool show)
{
    bool showCanvas = ui->actionLightmapCanvas->isChecked();
    REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->materialShowLightmapCanvas = showCanvas;

    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        scene->materialSystem->SetLightmapCanvasVisible(showCanvas);
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
    MainWindowDetails::GetCurrentScene()->visibilityCheckSystem->Recalculate();
}

void QtMainWindow::OnFixVisibilityFrame()
{
    MainWindowDetails::GetCurrentScene()->visibilityCheckSystem->FixCurrentFrame();
}

void QtMainWindow::OnReleaseVisibilityFrame()
{
    MainWindowDetails::GetCurrentScene()->visibilityCheckSystem->ReleaseFixedFrame();
}

void QtMainWindow::OnEnableDisableShadows(bool enable)
{
    DAVA::Renderer::GetOptions()->SetOption(DAVA::RenderOptions::SHADOWVOLUME_DRAW, enable);
}

void QtMainWindow::EnableSounds(bool toEnable)
{
    ui->actionEnableSounds->setChecked(toEnable);
    REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->enableSound = toEnable;
    DAVA::SoundSystem::Instance()->Mute(!toEnable);
}

void QtMainWindow::OnSelectMode()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        scene->modifSystem->SetTransformType(Selectable::TransformType::Disabled);
        LoadModificationState(scene.Get());
    }
}

void QtMainWindow::OnMoveMode()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        scene->modifSystem->SetTransformType(Selectable::TransformType::Translation);
        LoadModificationState(scene.Get());
    }
}

void QtMainWindow::OnRotateMode()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        scene->modifSystem->SetTransformType(Selectable::TransformType::Rotation);
        LoadModificationState(scene.Get());
    }
}

void QtMainWindow::OnScaleMode()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        scene->modifSystem->SetTransformType(Selectable::TransformType::Scale);
        LoadModificationState(scene.Get());
    }
}

void QtMainWindow::OnPivotCenterMode()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        scene->modifSystem->SetPivotPoint(Selectable::TransformPivot::ObjectCenter);
        LoadModificationState(scene.Get());
    }
}

void QtMainWindow::OnPivotCommonMode()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        scene->modifSystem->SetPivotPoint(Selectable::TransformPivot::CommonCenter);
        LoadModificationState(scene.Get());
    }
}

void QtMainWindow::OnManualModifMode()
{
    if (ui->actionManualModifMode->isChecked())
    {
        modificationWidget->SetPivotMode(ModificationWidget::PivotRelative);
    }
    else
    {
        modificationWidget->SetPivotMode(ModificationWidget::PivotAbsolute);
    }
}

void QtMainWindow::OnResetTransform()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        const SelectableGroup& selection = Selection::GetSelection();
        scene->modifSystem->ResetTransform(selection);
    }
}

void QtMainWindow::OnLockTransform()
{
    LockTransform(MainWindowDetails::GetCurrentScene().Get());
    UpdateModificationActionsState();
}

void QtMainWindow::OnUnlockTransform()
{
    UnlockTransform(MainWindowDetails::GetCurrentScene().Get());
    UpdateModificationActionsState();
}

void QtMainWindow::OnCenterPivotPoint()
{
    DAVA::RefPtr<SceneEditor2> curScene = MainWindowDetails::GetCurrentScene();
    if (curScene.Get() != nullptr)
    {
        const SelectableGroup& selection = Selection::GetSelection();
        curScene->modifSystem->MovePivotCenter(selection);
    }
}

void QtMainWindow::OnZeroPivotPoint()
{
    DAVA::RefPtr<SceneEditor2> curScene = MainWindowDetails::GetCurrentScene();
    if (curScene.Get() != nullptr)
    {
        const SelectableGroup& selection = Selection::GetSelection();
        curScene->modifSystem->MovePivotZero(selection);
    }
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

    DAVA::RefPtr<SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    TextureBrowser::Instance()->sceneActivated(sceneEditor.Get());
}

void QtMainWindow::OnSceneLightMode()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() != nullptr)
    {
        if (ui->actionEnableCameraLight->isChecked())
        {
            scene->editorLightSystem->SetCameraLightEnabled(true);
        }
        else
        {
            scene->editorLightSystem->SetCameraLightEnabled(false);
        }

        LoadEditorLightState(scene.Get());
    }
}

void QtMainWindow::OnCubemapEditor()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();

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
    DAVA::FilePath docsPath = ResourceEditor::DOCUMENTATION_PATH + "index.html";
    QString docsFile = QString::fromStdString("file:///" + docsPath.GetAbsolutePathname());
    QDesktopServices::openUrl(QUrl(docsFile));
}

// ###################################################################################################
// Mainwindow load state functions
// ###################################################################################################

void QtMainWindow::LoadViewState(SceneEditor2* scene)
{
    if (nullptr != scene)
    {
        ui->actionLightmapCanvas->setChecked(REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->materialShowLightmapCanvas);

        auto options = DAVA::Renderer::GetOptions();
        ui->actionEnableDisableShadows->setChecked(options->IsOptionEnabled(DAVA::RenderOptions::SHADOWVOLUME_DRAW));
    }
}

void QtMainWindow::LoadModificationState(SceneEditor2* scene)
{
    if (nullptr != scene)
    {
        ui->actionModifySelect->setChecked(false);
        ui->actionModifyMove->setChecked(false);
        ui->actionModifyRotate->setChecked(false);
        ui->actionModifyScale->setChecked(false);

        auto modifMode = scene->modifSystem->GetTransformType();
        modificationWidget->SetTransformType(modifMode);

        switch (modifMode)
        {
        case Selectable::TransformType::Disabled:
            ui->actionModifySelect->setChecked(true);
            break;
        case Selectable::TransformType::Translation:
            ui->actionModifyMove->setChecked(true);
            break;
        case Selectable::TransformType::Rotation:
            ui->actionModifyRotate->setChecked(true);
            break;
        case Selectable::TransformType::Scale:
            ui->actionModifyScale->setChecked(true);
            break;
        default:
            break;
        }

        // pivot point
        if (scene->modifSystem->GetPivotPoint() == Selectable::TransformPivot::ObjectCenter)
        {
            ui->actionPivotCenter->setChecked(true);
            ui->actionPivotCommon->setChecked(false);
        }
        else
        {
            ui->actionPivotCenter->setChecked(false);
            ui->actionPivotCommon->setChecked(true);
        }

        // way editor
        ui->actionWayEditor->setChecked(scene->wayEditSystem->IsWayEditEnabled());

        UpdateModificationActionsState();
    }
}

void QtMainWindow::LoadEditorLightState(SceneEditor2* scene)
{
    if (nullptr != scene)
    {
        ui->actionEnableCameraLight->setChecked(scene->editorLightSystem->GetCameraLightEnabled());
    }
}

void QtMainWindow::LoadMaterialLightViewMode()
{
    int curViewMode = REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->materialLightViewMode;

    ui->actionAlbedo->setChecked((bool)(curViewMode & EditorMaterialSystem::LIGHTVIEW_ALBEDO));
    ui->actionAmbient->setChecked((bool)(curViewMode & EditorMaterialSystem::LIGHTVIEW_AMBIENT));
    ui->actionSpecular->setChecked((bool)(curViewMode & EditorMaterialSystem::LIGHTVIEW_SPECULAR));
    ui->actionDiffuse->setChecked((bool)(curViewMode & EditorMaterialSystem::LIGHTVIEW_DIFFUSE));
}

void QtMainWindow::LoadLandscapeEditorState(SceneEditor2* scene)
{
    OnLandscapeEditorToggled(scene);
}

void QtMainWindow::OnSaveHeightmapToImage()
{
    if (MainWindowDetails::IsSavingAllowed("Save heightmap to Image") == false)
    {
        return;
    }

    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();

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

    QString selectedPath = FileDialog::getSaveFileName(this, "Save heightmap as", heightmapPath.GetAbsolutePathname().c_str(),
                                                       PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter);
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

    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    LandscapeEditorDrawSystem::eErrorType varifLandscapeError = scene->landscapeEditorDrawSystem->VerifyLandscape();
    if (varifLandscapeError != LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS)
    {
        DAVA::Logger::Error(LandscapeEditorDrawSystem::GetDescriptionByError(varifLandscapeError).c_str());
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
        ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
        DVASSERT(data != nullptr);
        QString selectedPath = FileDialog::getSaveFileName(this, "Save landscape texture as",
                                                           data->GetDataSource3DPath().GetAbsolutePathname().c_str(),
                                                           PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_IMAGE).fileFilter);

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

    SaveTextureToFile(landscapeTexture, pathToSave);
}

void QtMainWindow::OnConvertModifiedTextures()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (!scene)
    {
        return;
    }

    WaitStart("Conversion of modified textures.", "Checking for modified textures.", 0, 0);
    DAVA::Map<DAVA::Texture*, DAVA::Vector<DAVA::eGPUFamily>> textures;
    int filesToUpdate = SceneHelper::EnumerateModifiedTextures(scene.Get(), textures);

    if (filesToUpdate == 0)
    {
        WaitStop();
        return;
    }

    DAVA::TextureConverter::eConvertQuality quality = REGlobal::GetGlobalContext()->GetData<GeneralSettings>()->compressionQuality;

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

void QtMainWindow::EditorLightEnabled(bool enabled)
{
    ui->actionEnableCameraLight->setChecked(enabled);
}

void QtMainWindow::OnLandscapeEditorToggled(SceneEditor2* scene)
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
    if (tools & SceneEditor2::LANDSCAPE_TOOL_CUSTOM_COLOR)
    {
        ui->actionCustomColorsEditor->setChecked(true);
        anyEditorEnabled = true;
    }
    if (tools & SceneEditor2::LANDSCAPE_TOOL_HEIGHTMAP_EDITOR)
    {
        ui->actionHeightMapEditor->setChecked(true);
        anyEditorEnabled = true;
    }
    if (tools & SceneEditor2::LANDSCAPE_TOOL_RULER)
    {
        ui->actionRulerTool->setChecked(true);
        anyEditorEnabled = true;
    }
    if (tools & SceneEditor2::LANDSCAPE_TOOL_TILEMAP_EDITOR)
    {
        ui->actionTileMapEditor->setChecked(true);
        anyEditorEnabled = true;
    }
    if (tools & SceneEditor2::LANDSCAPE_TOOL_NOT_PASSABLE_TERRAIN)
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
    DAVA::RefPtr<SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (!sceneEditor)
    {
        return;
    }

    if (sceneEditor->customColorsSystem->IsLandscapeEditingEnabled())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DisableCustomColorsCommand(sceneEditor.Get(), true)));
    }
    else if (CanEnableLandscapeEditor())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new EnableCustomColorsCommand(sceneEditor.Get(), true)));
    }
    OnLandscapeEditorToggled(sceneEditor.Get());
}

void QtMainWindow::OnHeightmapEditor()
{
    DAVA::RefPtr<SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (!sceneEditor)
    {
        return;
    }

    if (sceneEditor->heightmapEditorSystem->IsLandscapeEditingEnabled())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DisableHeightmapEditorCommand(sceneEditor.Get())));
    }
    else if (CanEnableLandscapeEditor())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new EnableHeightmapEditorCommand(sceneEditor.Get())));
    }
    OnLandscapeEditorToggled(sceneEditor.Get());
}

void QtMainWindow::OnRulerTool()
{
    DAVA::RefPtr<SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (!sceneEditor)
    {
        return;
    }

    if (sceneEditor->rulerToolSystem->IsLandscapeEditingEnabled())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DisableRulerToolCommand(sceneEditor.Get())));
    }
    else if (CanEnableLandscapeEditor())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new EnableRulerToolCommand(sceneEditor.Get())));
    }
    OnLandscapeEditorToggled(sceneEditor.Get());
}

void QtMainWindow::OnTilemaskEditor()
{
    DAVA::RefPtr<SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (!sceneEditor)
    {
        return;
    }

    if (sceneEditor->tilemaskEditorSystem->IsLandscapeEditingEnabled())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DisableTilemaskEditorCommand(sceneEditor.Get())));
    }
    else if (CanEnableLandscapeEditor())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new EnableTilemaskEditorCommand(sceneEditor.Get())));
    }
    OnLandscapeEditorToggled(sceneEditor.Get());
}

void QtMainWindow::OnForceFirstLod(bool enabled)
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
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
    scene->visibilityCheckSystem->Recalculate();
}

void QtMainWindow::OnNotPassableTerrain()
{
    DAVA::RefPtr<SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (!sceneEditor)
    {
        return;
    }

    if (sceneEditor->landscapeEditorDrawSystem->IsNotPassableTerrainEnabled())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new DisableNotPassableCommand(sceneEditor.Get())));
    }
    else if (CanEnableLandscapeEditor())
    {
        sceneEditor->Exec(std::unique_ptr<DAVA::Command>(new EnableNotPassableCommand(sceneEditor.Get())));
    }
    OnLandscapeEditorToggled(sceneEditor.Get());
}

void QtMainWindow::OnWayEditor()
{
    DAVA::RefPtr<SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (!sceneEditor)
    {
        return;
    }

    if (sceneEditor->pathSystem->IsPathEditEnabled())
    {
        sceneEditor->Exec(std::make_unique<DisableWayEditCommand>(sceneEditor.Get()));
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
            sceneEditor->Exec(std::make_unique<EnableWayEditCommand>(sceneEditor.Get()));
        }
    }
}

void QtMainWindow::OnBuildStaticOcclusion()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (!scene)
        return;

    QtWaitDialog* waitOcclusionDlg = new QtWaitDialog(this);
    waitOcclusionDlg->Show("Static occlusion", "Please wait while building static occlusion.", true, true);

    bool sceneWasChanged = true;
    scene->staticOcclusionBuildSystem->Build();
    while (scene->staticOcclusionBuildSystem->IsInBuild())
    {
        if (waitOcclusionDlg->WasCanceled())
        {
            scene->staticOcclusionBuildSystem->Cancel();
            sceneWasChanged = false;
        }
        else
        {
            waitOcclusionDlg->SetValue(scene->staticOcclusionBuildSystem->GetBuildStatus());
            waitOcclusionDlg->SetMessage(QString::fromStdString(scene->staticOcclusionBuildSystem->GetBuildStatusInfo()));
        }
    }

    if (sceneWasChanged)
    {
        scene->MarkAsChanged();

        if (REGlobal::GetGlobalContext()->GetData<GlobalSceneSettings>()->saveStaticOcclusion)
        {
            REGlobal::GetInvoker()->Invoke(REGlobal::SaveCurrentScene.ID);
        }
    }

    delete waitOcclusionDlg;
}

void QtMainWindow::OnInavalidateStaticOcclusion()
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (!scene)
        return;
    scene->staticOcclusionSystem->InvalidateOcclusion();
    scene->MarkAsChanged();
}

void QtMainWindow::OnDataChanged(const DAVA::TArc::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields)
{
    if (projectDataWrapper == wrapper)
    {
        ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();

        // empty fields mean that ProjectManagerData was just added
        // this means that there is a first call
        if (data != nullptr && fields.empty())
        {
            const SpritesPackerModule* spritesPacker = data->GetSpritesModules();
            QObject::connect(spritesPacker, &SpritesPackerModule::SpritesReloaded, ui->sceneInfo, &SceneInfo::SpritesReloaded);
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
    else if (selectionWrapper == wrapper)
    {
        UpdateModificationActionsState();
    }
}

void QtMainWindow::OnMaterialLightViewChanged(bool)
{
    int newMode = EditorMaterialSystem::LIGHTVIEW_NOTHING;

    if (ui->actionAlbedo->isChecked())
        newMode |= EditorMaterialSystem::LIGHTVIEW_ALBEDO;
    if (ui->actionDiffuse->isChecked())
        newMode |= EditorMaterialSystem::LIGHTVIEW_DIFFUSE;
    if (ui->actionAmbient->isChecked())
        newMode |= EditorMaterialSystem::LIGHTVIEW_AMBIENT;
    if (ui->actionSpecular->isChecked())
        newMode |= EditorMaterialSystem::LIGHTVIEW_SPECULAR;

    REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->materialLightViewMode = static_cast<EditorMaterialSystem::MaterialLightViewMode>(newMode);
    if (MainWindowDetails::GetCurrentScene().Get() != nullptr)
    {
        MainWindowDetails::GetCurrentScene()->materialSystem->SetLightViewMode(newMode);
    }
}

void QtMainWindow::OnCustomQuality()
{
    QualitySwitcher::ShowDialog(globalOperations);
}

void QtMainWindow::SynchronizeStateWithUI()
{
    OnManualModifMode();
}

bool QtMainWindow::CanEnableLandscapeEditor() const
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (scene.Get() == nullptr)
        return false;

    if (scene->pathSystem->IsPathEditEnabled())
    {
        DAVA::Logger::Error("WayEditor should be disabled prior to enabling landscape tools");
        return false;
    }

    DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;
    if (fs->GetFilenamesTag().empty() == false)
    {
        DAVA::Logger::Error("Could not enable editing of landscape due to enabled tags at file system");
        return false;
    }

    return LoadAppropriateTextureFormat();
}

bool QtMainWindow::LoadAppropriateTextureFormat() const
{
    CommonInternalSettings* settings = REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>();
    DAVA::eGPUFamily gpuFormat = settings->textureViewGPU;
    if (gpuFormat != DAVA::GPU_ORIGIN)
    {
        int answer = ShowQuestion("Inappropriate texture format",
                                  "Landscape editing is only allowed in original texture format.\nDo you want to reload textures in original format?",
                                  MB_FLAG_YES | MB_FLAG_NO, MB_FLAG_NO);
        if (answer == MB_FLAG_NO)
        {
            return false;
        }

        REGlobal::GetInvoker()->Invoke(REGlobal::ReloadAllTextures.ID, DAVA::eGPUFamily::GPU_ORIGIN);
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
        DAVA::RefPtr<SceneEditor2> currentScene = MainWindowDetails::GetCurrentScene();
        if (currentScene.Get() != nullptr)
        {
            auto vec = conv.GetPointers<DAVA::Entity*>();
            if (!vec.empty())
            {
                SelectableGroup objects;
                DAVA::Vector<DAVA::Entity*> allEntities;
                currentScene->GetChildNodes(allEntities);
                for (auto entity : vec)
                {
                    if (std::find(allEntities.begin(), allEntities.end(), entity) != allEntities.end())
                    {
                        objects.Add(entity, currentScene->collisionSystem->GetUntransformedBoundingBox(entity));
                    }
                }

                if (!objects.IsEmpty())
                {
                    Selection::SetSelection(objects);
                    currentScene->cameraSystem->LookAt(objects.GetIntegralBoundingBox());
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

void QtMainWindow::OnBatchProcessScene()
{
    SceneProcessor sceneProcessor;

    // For client developers: need to set entity processor derived from EntityProcessorBase
    //DestructibleSoundAdder *entityProcessor = new DestructibleSoundAdder();
    //sceneProcessor.SetEntityProcessor(entityProcessor);
    //SafeRelease(entityProcessor);

    DAVA::RefPtr<SceneEditor2> sceneEditor = MainWindowDetails::GetCurrentScene();
    if (sceneProcessor.Execute(sceneEditor.Get()))
    {
        REGlobal::GetInvoker()->Invoke(REGlobal::SaveCurrentScene.ID);
    }
}

void QtMainWindow::OnSnapCameraToLandscape(bool snap)
{
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
    if (!scene)
        return;

    bool toggleProcessed = false;
    if (scene->cameraSystem)
    {
        toggleProcessed = scene->cameraSystem->SnapEditorCameraToLandscape(snap);
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
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
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
    DAVA::RefPtr<SceneEditor2> scene = MainWindowDetails::GetCurrentScene();
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
    ProjectManagerData* data = REGlobal::GetDataNode<ProjectManagerData>();
    DVASSERT(data != nullptr);
    return data->GetSpritesModules()->IsRunning();
}

void QtMainWindow::CallAction(ID id, DAVA::Any&& args)
{
    switch (id)
    {
    case GlobalOperations::OpenScene:
    {
        // OpenScene function open WaitDialog and run EventLoop
        // To avoid embedded DAVA::OnFrame calling we will execute OpenScene inside Qt loop.
        DAVA::FilePath scenePath(args.Cast<DAVA::String>());
        delayedExecutor.DelayedExecute([scenePath]()
                                       {
                                           REGlobal::GetInvoker()->Invoke(REGlobal::OpenSceneOperation.ID, scenePath);
                                       });
    }
    break;
    case GlobalOperations::ShowMaterial:
        OnMaterialEditor(args.Cast<DAVA::NMaterial*>());
        break;
    case GlobalOperations::ReloadTexture:
        REGlobal::GetInvoker()->Invoke(REGlobal::ReloadAllTextures.ID, REGlobal::GetGlobalContext()->GetData<CommonInternalSettings>()->textureViewGPU);
        break;
    default:
        DVASSERT(false, DAVA::Format("Not implemented action : %d", static_cast<DAVA::int32>(id)).c_str());
        break;
    }
}

QWidget* QtMainWindow::GetGlobalParentWidget() const
{
    return const_cast<QtMainWindow*>(this);
}

void QtMainWindow::ShowWaitDialog(const DAVA::String& tittle, const DAVA::String& message, DAVA::uint32 min /*= 0*/, DAVA::uint32 max /*= 100*/)
{
    WaitStart(QString::fromStdString(tittle), QString::fromStdString(message), min, max);
}

void QtMainWindow::HideWaitDialog()
{
    WaitStop();
}

void QtMainWindow::ForEachScene(const DAVA::Function<void(SceneEditor2*)>& functor)
{
    REGlobal::GetAccessor()->ForEachContext([&](DAVA::TArc::DataContext& ctx)
                                            {
                                                SceneData* data = ctx.GetData<SceneData>();
                                                functor(data->GetScene().Get());
                                            });
}

void QtMainWindow::OnValidateScene()
{
    DAVA::RefPtr<SceneEditor2> currentScene = MainWindowDetails::GetCurrentScene();
    DVASSERT(currentScene.Get() != nullptr);

    SceneValidationDialog dlg(currentScene.Get());
    dlg.exec();
}
