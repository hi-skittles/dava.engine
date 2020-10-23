#include "ViewSceneScreen.h"
#include "SceneViewerApp.h"
#include "Settings.h"

#include <DocDirSetup/DocDirSetup.h>

#include <Entity/ComponentUtils.h>
#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <ModuleManager/ModuleManager.h>

#include <Math/MathHelpers.h>
#include <Render/2D/Sprite.h>
#include <Input/Keyboard.h>
#include <DeviceManager/DeviceManager.h>
#include <UI/Layouts/UIAnchorComponent.h>
#include <UI/Update/UIUpdateComponent.h>
#include <Platform/DeviceInfo.h>
#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
#include <Physics/StaticBodyComponent.h>
#include <Physics/HeightFieldShapeComponent.h>
#include <Physics/PhysicsSystem.h>
#endif
#include <Scene3D/Components/Controller/WASDControllerComponent.h>

#include <TestCharacterController/TestCharacterControllerModule.h>
#include <TestCharacterController/TestCharacterControllerSystems.h>

namespace ViewSceneScreenDetails
{
const DAVA::float32 ABOVE_LANDSCAPE_ELEVATION = 10.f;
const DAVA::float32 INFO_UPDATE_INTERVAL_SEC = 0.5f;

const DAVA::String BOTSPAWN_PROPERTY_VALUE = "botspawn";
const DAVA::String TANK_MODEL_PATH = "~res:/Tanks/USSR/T54S.sc2";
}

ViewSceneScreen::ViewSceneScreen(SceneViewerData& data)
    : data(data)
    , scenePath(data.scenePath)
    , scene(data.scene)
    , fpsMeter(ViewSceneScreenDetails::INFO_UPDATE_INTERVAL_SEC)
#ifdef WITH_SCENE_PERFORMANCE_TESTS
    , gridTest(data.engine, this)
#endif
{
    GetOrCreateComponent<DAVA::UIUpdateComponent>();
}

void ViewSceneScreen::LoadResources()
{
    BaseScreen::LoadResources();
    if (!scene)
    {
        LoadScene();
    }
    AddControls();
    PlaceSceneAtScreen();
}

void ViewSceneScreen::UnloadResources()
{
    RemoveSceneFromScreen();
    RemoveControls();
    BaseScreen::UnloadResources();
}

void ViewSceneScreen::PlaceSceneAtScreen()
{
    using namespace DAVA;

    if (scene)
    {
        sceneView = new DAVA::UI3DView(GetRect());
        sceneView->SetMultiInput(true);
        AddControl(sceneView);

        Camera* camera = scene->GetCurrentCamera();
        camera->SetupPerspective(70.f, data.screenAspect, 0.5f, 2500.f);
        camera->SetUp(DAVA::Vector3(0.f, 0.f, 1.f));
        SetCameraAtCenter(camera);

        rotationControllerSystem = new DAVA::RotationControllerSystem(scene);
        wasdSystem = new WASDControllerSystem(scene);

        AddCameraControllerSystems();

        sceneView->SetScene(scene);

        if (menu)
        {
            menu->BringAtFront();
            qualitySettingsMenuItem->SetEnabled(true);
            reloadShadersMenuItem->SetEnabled(true);
#ifdef WITH_SCENE_PERFORMANCE_TESTS
            performanceTestMenuItem->SetEnabled(true);
#endif
            characterSpawnMenuItem->SetEnabled(true);
        }

        if (moveJoyPAD)
        {
            BringChildFront(moveJoyPAD);
        }
    }
}

void ViewSceneScreen::RemoveSceneFromScreen()
{
    if (scene)
    {
        if (!characterSpawned)
        {
            RemoveCameraControllerSystems();
        }
        else
        {
            DAVA::TestCharacterControllerModule* characterModule = DAVA::GetEngineContext()->moduleManager->GetModule<DAVA::TestCharacterControllerModule>();
            characterModule->DisableController(scene);
            characterSpawned = false;
        }

        SafeDelete(rotationControllerSystem);
        SafeDelete(wasdSystem);

        RemoveControl(sceneView);
        sceneView.reset();

        if (menu)
        {
            qualitySettingsMenuItem->SetEnabled(false);
            reloadShadersMenuItem->SetEnabled(false);
            performanceTestMenuItem->SetEnabled(false);
            characterSpawnMenuItem->SetEnabled(false);
        }
    }
}

void ViewSceneScreen::LoadScene()
{
    using namespace DAVA;

    scene = new Scene();

    SceneFileV2::eError result = scene->LoadScene(scenePath);
    if (result == SceneFileV2::ERROR_NO_ERROR)
    {
#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
        Entity* landscapeEntity = scene->FindByName("Landscape");
        if (landscapeEntity != nullptr)
        {
            landscapeEntity->AddComponent(new StaticBodyComponent());
            landscapeEntity->AddComponent(new HeightFieldShapeComponent());
        }

        scene->physicsSystem->SetSimulationEnabled(true);
#endif

        ScopedPtr<Camera> camera(new Camera);
        scene->AddCamera(camera);
        scene->SetCurrentCamera(camera);

        ScopedPtr<Entity> cameraEntity(new Entity());
        cameraEntity->AddComponent(new CameraComponent(camera));
        cameraEntity->AddComponent(new WASDControllerComponent());
        cameraEntity->AddComponent(new RotationControllerComponent());
        scene->AddNode(cameraEntity);
    }
    else
    {
        scene.reset();
    }
}

void ViewSceneScreen::AddTanksAtScene()
{
    using namespace ViewSceneScreenDetails;
    using namespace DAVA;

    List<Entity*> spawnPoints;
    scene->GetChildEntitiesWithComponent(spawnPoints, Type::Instance<CustomPropertiesComponent>(), false);

    for (List<Entity*>::iterator it = spawnPoints.begin(); it != spawnPoints.end();)
    {
        KeyedArchive* props = GetCustomPropertiesArchieve(*it);
        DVASSERT(props);

        if (props->IsKeyExists("type") && props->GetString("type") == BOTSPAWN_PROPERTY_VALUE)
        {
            ++it;
        }
        else
        {
            auto itDel = it++;
            spawnPoints.erase(itDel);
        }
    }

    if (!spawnPoints.empty())
    {
        ScopedPtr<Scene> tank(new Scene());
        if (SceneFileV2::eError::ERROR_NO_ERROR == tank->LoadScene(ViewSceneScreenDetails::TANK_MODEL_PATH))
        {
            for (Entity* spawnPoint : spawnPoints)
            {
                Entity* clonedTank = new Entity;
                tank->Clone(clonedTank);
                while (clonedTank->GetChildrenCount() != 0)
                {
                    spawnPoint->AddNode(clonedTank->GetChild(0));
                }
            }
        }
    }
}

void ViewSceneScreen::AddMenuControl()
{
    DVASSERT(!menu);
    DAVA::Rect rect = DAVA::Rect(10.f, 30.f, 250.f, 60.f);
    menu.reset(new Menu(nullptr, this, font, rect));

    SubMenuItem* mainSubMenuItem = menu->AddSubMenuItem(L"Menu");
    Menu* mainSubMenu = mainSubMenuItem->submenu.get();

    SubMenuItem* selectSceneSubMenuItem = mainSubMenu->AddSubMenuItem(L"Select scene");
    qualitySettingsMenuItem = mainSubMenu->AddActionItem(L"Quality settings", DAVA::Message(this, &ViewSceneScreen::OnButtonQualitySettings));
    reloadShadersMenuItem = mainSubMenu->AddActionItem(L"Reload shaders", DAVA::Message(this, &ViewSceneScreen::OnButtonReloadShaders));
    performanceTestMenuItem = mainSubMenu->AddActionItem(L"Performance test", DAVA::Message(this, &ViewSceneScreen::OnButtonPerformanceTest));
    characterSpawnMenuItem = mainSubMenu->AddActionItem(L"Toggle Spawn Character", DAVA::Message(this, &ViewSceneScreen::OnButtonToggleSpawnCharacter));
    mainSubMenu->AddBackItem();

    qualitySettingsMenuItem->SetEnabled(false);
    reloadShadersMenuItem->SetEnabled(false);
    performanceTestMenuItem->SetEnabled(false);
    characterSpawnMenuItem->SetEnabled(false);

    Menu* selectSceneSubMenu = selectSceneSubMenuItem->submenu.get();
    selectSceneSubMenu->AddActionItem(L"Select from ~res", DAVA::Message(this, &ViewSceneScreen::OnButtonSelectFromRes));
    selectSceneSubMenu->AddActionItem(L"Select from documents", DAVA::Message(this, &ViewSceneScreen::OnButtonSelectFromDoc));
    selectSceneSubMenu->AddActionItem(L"Select from ext storage", DAVA::Message(this, &ViewSceneScreen::OnButtonSelectFromExt));
    selectSceneSubMenu->AddBackItem();
}

void ViewSceneScreen::AddFileDialogControl()
{
    DVASSERT(!fileSystemDialog);
    fileSystemDialog = new DAVA::UIFileSystemDialog("~res:/SceneViewer/Fonts/korinna.ttf");
    fileSystemDialog->SetDelegate(this);
    fileSystemDialog->SetExtensionFilter(".sc2");
    fileSystemDialog->SetOperationType(DAVA::UIFileSystemDialog::OPERATION_LOAD);

    DAVA::UIAnchorComponent* anchor = fileSystemDialog->GetOrCreateComponent<DAVA::UIAnchorComponent>();
    anchor->SetRightAnchorEnabled(true);
    anchor->SetRightAnchor(30.f);
    anchor->SetVCenterAnchorEnabled(true);
}

void ViewSceneScreen::AddJoypadControl()
{
    DVASSERT(!moveJoyPAD);
    moveJoyPAD = new DAVA::UIJoypad(DAVA::Rect(10, GetRect().dy - 210.f, 200.f, 200.f));
    moveJoyPAD->SetDeadAreaSize(30.f);
    DAVA::ScopedPtr<DAVA::Sprite> stickSprite(DAVA::Sprite::CreateFromSourceFile("~res:/SceneViewer/UI/Joypad.png", true));
    moveJoyPAD->SetStickSprite(stickSprite, 0);
    AddControl(moveJoyPAD);
}

void ViewSceneScreen::AddInfoTextControl()
{
    DVASSERT(!infoText);
    infoText = new DAVA::UIStaticText(DAVA::Rect(GetRect().dx / 2 - 150, 30, 300, 30));
    infoText->SetFont(font);
    infoText->SetTextColor(DAVA::Color::White);
    infoText->SetTextAlign(DAVA::ALIGN_HCENTER);
    AddControl(infoText);
}

void ViewSceneScreen::AddQualitySettingsDialog()
{
    DVASSERT(!qualitySettingsDialog);
    qualitySettingsDialog = new QualitySettingsDialog(data.settings);
    qualitySettingsDialog->SetParentControl(this);
    qualitySettingsDialog->SetDelegate(this);
}

void ViewSceneScreen::AddControls()
{
    AddMenuControl();
    AddFileDialogControl();
    AddInfoTextControl();
    AddJoypadControl();
    AddQualitySettingsDialog();
}

void ViewSceneScreen::RemoveControls()
{
    qualitySettingsDialog.reset();
    infoText.reset();
    moveJoyPAD.reset();
    fileSystemDialog.reset();

    menu.reset();
    qualitySettingsMenuItem = nullptr;
    reloadShadersMenuItem = nullptr;
    performanceTestMenuItem = nullptr;
    characterSpawnMenuItem = nullptr;
}

void ViewSceneScreen::SetCameraAtCenter(DAVA::Camera* camera)
{
    DAVA::Vector3 position = DAVA::Vector3(0.f, -65.f, 10.f);

    DAVA::Landscape* landscape = FindLandscape(scene);
    if (landscape != nullptr)
    {
        DAVA::float32 landscapeHeight = 0.f;
        landscape->GetHeightAtPoint(position, landscapeHeight);
        position.z = landscapeHeight + ViewSceneScreenDetails::ABOVE_LANDSCAPE_ELEVATION;
    }

    camera->SetLeft(DAVA::Vector3(1.f, 0.f, 0.f));
    camera->SetTarget(DAVA::Vector3(0.f, 0.f, 0.f));
    camera->SetPosition(position);
}

void ViewSceneScreen::AddCameraControllerSystems()
{
    if (scene)
    {
        scene->AddSystem(rotationControllerSystem, DAVA::ComponentUtils::MakeMask<DAVA::CameraComponent>() | DAVA::ComponentUtils::MakeMask<DAVA::RotationControllerComponent>(),
                         DAVA::Scene::SCENE_SYSTEM_REQUIRE_PROCESS | DAVA::Scene::SCENE_SYSTEM_REQUIRE_INPUT);

        scene->AddSystem(wasdSystem, DAVA::ComponentUtils::MakeMask<DAVA::CameraComponent>() | DAVA::ComponentUtils::MakeMask<DAVA::WASDControllerComponent>(),
                         DAVA::Scene::SCENE_SYSTEM_REQUIRE_PROCESS);
    }
}

void ViewSceneScreen::RemoveCameraControllerSystems()
{
    if (scene)
    {
        scene->RemoveSystem(rotationControllerSystem);
        scene->RemoveSystem(wasdSystem);
    }
}

void ViewSceneScreen::OnFileSelected(DAVA::UIFileSystemDialog* forDialog, const DAVA::FilePath& pathToFile)
{
    menu->SetEnabled(true);
    scenePath = pathToFile;
    data.settings.SetLastOpenedScenePath(scenePath);
    ReloadScene();
}

void ViewSceneScreen::OnFileSytemDialogCanceled(DAVA::UIFileSystemDialog* forDialog)
{
    menu->SetEnabled(true);
}

void ViewSceneScreen::OnButtonSelectFromRes(DAVA::BaseObject* caller, void* param, void* callerData)
{
    DVASSERT(fileSystemDialog);
    fileSystemDialog->SetCurrentDir("~res:/");
    fileSystemDialog->Show(this);
    menu->SetEnabled(false);
}

void ViewSceneScreen::OnButtonSelectFromDoc(DAVA::BaseObject* caller, void* param, void* callerData)
{
    DVASSERT(fileSystemDialog);
    fileSystemDialog->SetCurrentDir(DAVA::DocumentsDirectorySetup::GetEngineDocumentsPath());
    fileSystemDialog->Show(this);
    menu->SetEnabled(false);
}

void ViewSceneScreen::OnButtonSelectFromExt(DAVA::BaseObject* caller, void* param, void* callerData)
{
    using namespace DAVA;
    DVASSERT(fileSystemDialog);

    List<DeviceInfo::StorageInfo> storageList = DeviceInfo::GetStoragesList();
    for (const DeviceInfo::StorageInfo& storage : storageList)
    {
        if (storage.type == DeviceInfo::STORAGE_TYPE_PRIMARY_EXTERNAL ||
            storage.type == DeviceInfo::STORAGE_TYPE_SECONDARY_EXTERNAL)
        {
            menu->SetEnabled(false);
            fileSystemDialog->SetCurrentDir(storage.path);
            fileSystemDialog->Show(this);
            return;
        }
    }
}

void ViewSceneScreen::OnButtonPerformanceTest(DAVA::BaseObject* caller, void* param, void* callerData)
{
#ifdef WITH_SCENE_PERFORMANCE_TESTS
    if (scene && gridTest.GetState() == GridTest::StateFinished && gridTest.Start(sceneView) == true)
    {
        menu->Show(false);
        RemoveControl(moveJoyPAD);
    }
#endif
}

void ViewSceneScreen::OnButtonQualitySettings(DAVA::BaseObject* caller, void* param, void* callerData)
{
    menu->SetEnabled(false);
    qualitySettingsDialog->SetCurrentScene(scene);
    qualitySettingsDialog->Show();
}

void ViewSceneScreen::OnQualitySettingsEditDone()
{
    menu->SetEnabled(true);
}

void ViewSceneScreen::OnButtonReloadShaders(DAVA::BaseObject* caller, void* param, void* callerData)
{
    using namespace DAVA;

    if (scene)
    {
        ShaderDescriptorCache::ReloadShaders();

        List<NMaterial*> materials;
        scene->GetDataNodes(materials);
        for (auto material : materials)
        {
            material->InvalidateRenderVariants();
        }

        const auto particleInstances = scene->particleEffectSystem->GetMaterialInstances();
        for (auto& material : particleInstances)
        {
            material.second->InvalidateRenderVariants();
        }

        DAVA::Set<DAVA::NMaterial*> materialList;
        scene->foliageSystem->CollectFoliageMaterials(materialList);
        for (auto material : materialList)
        {
            if (material)
                material->InvalidateRenderVariants();
        }

        scene->renderSystem->GetDebugDrawer()->InvalidateMaterials();
        scene->renderSystem->SetForceUpdateLights();
        
    #define INVALIDATE_2D_MATERIAL(material) \
        if (RenderSystem2D::material) \
            RenderSystem2D::material->InvalidateRenderVariants();

        INVALIDATE_2D_MATERIAL(DEFAULT_2D_COLOR_MATERIAL)
        INVALIDATE_2D_MATERIAL(DEFAULT_2D_FILL_ALPHA_MATERIAL)
        INVALIDATE_2D_MATERIAL(DEFAULT_2D_TEXTURE_MATERIAL)
        INVALIDATE_2D_MATERIAL(DEFAULT_2D_TEXTURE_NOBLEND_MATERIAL)
        INVALIDATE_2D_MATERIAL(DEFAULT_2D_TEXTURE_ALPHA8_MATERIAL)
        INVALIDATE_2D_MATERIAL(DEFAULT_2D_TEXTURE_GRAYSCALE_MATERIAL)
        
    #undef INVALIDATE_2D_MATERIAL
    }
}

void ViewSceneScreen::OnButtonToggleSpawnCharacter(DAVA::BaseObject* caller, void* param, void* callerData)
{
    DAVA::TestCharacterControllerModule* characterModule = DAVA::GetEngineContext()->moduleManager->GetModule<DAVA::TestCharacterControllerModule>();

    if (characterSpawned)
    {
        characterModule->DisableController(scene);
        AddCameraControllerSystems();
        scene->GetCurrentCamera()->SetUp(DAVA::Vector3::UnitZ);

        DAVA::Engine::Instance()->PrimaryWindow()->SetCursorCapture(DAVA::eCursorCapture::OFF);
        characterSpawned = false;
    }
    else
    {
        if (DAVA::DeviceInfo::GetPlatform() == DAVA::DeviceInfo::PLATFORM_MACOS
            || DAVA::DeviceInfo::GetPlatform() == DAVA::DeviceInfo::PLATFORM_WIN32
            || DAVA::DeviceInfo::GetPlatform() == DAVA::DeviceInfo::PLATFORM_DESKTOP_WIN_UAP)
        {
            DAVA::Engine::Instance()->PrimaryWindow()->SetCursorCapture(DAVA::eCursorCapture::PINNING);
            sceneView->SetMultiInput(true);
        }

        characterModule->EnableController(scene, scene->GetCurrentCamera()->GetPosition());
        RemoveCameraControllerSystems();

        characterSpawned = true;
    }
}

void ViewSceneScreen::Draw(const DAVA::UIGeometricData& geometricData)
{
    //DAVA::uint64 startTime = DAVA::SystemTimer::Instance()->GetNs();

    BaseScreen::Draw(geometricData);

    //drawTime += (SystemTimer::Instance()->GetNs() - startTime);
}

void ViewSceneScreen::ReloadScene()
{
    RemoveSceneFromScreen();
    LoadScene();
    PlaceSceneAtScreen();
}

void ViewSceneScreen::ProcessUserInput(DAVA::float32 timeElapsed)
{
    if (scene)
    {
        using namespace DAVA;

        Keyboard* keyboard = GetEngineContext()->deviceManager->GetKeyboard();
        if (keyboard != nullptr)
        {
            if (keyboard->GetKeyState(eInputElements::KB_SPACE).IsPressed())
                wasdSystem->SetMoveSpeed(30.f);
            else
                wasdSystem->SetMoveSpeed(10.f);

            if (characterSpawned && keyboard->GetKeyState(eInputElements::KB_ESCAPE).IsJustPressed())
                OnButtonToggleSpawnCharacter(nullptr, nullptr, nullptr);
        }

        if (characterSpawned)
        {
            DAVA::TestCharacterControllerModule* characterModule = DAVA::GetEngineContext()->moduleManager->GetModule<DAVA::TestCharacterControllerModule>();
            DAVA::TestCharacterControllerSystem* characterControllerSystem = characterModule->GetCharacterControllerSystem(scene);
            if (characterControllerSystem != nullptr)
                characterControllerSystem->SetJoypadDirection(moveJoyPAD->GetAnalogPosition());
        }
        else
        {
            Vector2 joypadPos = moveJoyPAD->GetDigitalPosition();

            Camera* camera = scene->GetDrawCamera();
            Vector3 cameraMoveOffset = (-joypadPos.x * camera->GetLeft() - joypadPos.y * camera->GetDirection()) * timeElapsed * 20.f;

            camera->SetPosition(camera->GetPosition() + cameraMoveOffset);
            camera->SetTarget(camera->GetTarget() + cameraMoveOffset);
        }
    }
}

void ViewSceneScreen::Update(DAVA::float32 timeElapsed)
{
    BaseScreen::Update(timeElapsed);

    UpdateInfo(timeElapsed);
    ProcessUserInput(timeElapsed);
}

void ViewSceneScreen::UpdateInfo(DAVA::float32 timeElapsed)
{
    fpsMeter.Update(timeElapsed);
    if (fpsMeter.IsFpsReady())
    {
        infoText->SetText(DAVA::Format(L"FPS: %.0f", fpsMeter.GetFps()));
        //      drawTime = updateTime = 0;
    }
}

#ifdef WITH_SCENE_PERFORMANCE_TESTS
void ViewSceneScreen::OnGridTestStateChanged()
{
    if (gridTest.GetState() == GridTest::StateFinished)
    {
        data.gridTestResult = gridTest.GetResult();
        SetNextScreen();
    }
}
#endif

// void ViewSceneScreen::DidAppear()
// {
//     framesTime = 0.f;
//     frameCounter = 0;
//
//     drawTime = updateTime = 0;
//
//     info->SetText(L"");
// }

void ViewSceneScreen::Input(DAVA::UIEvent* currentInput)
{
    //     if (currentInput->phase == UIEvent::Phase::CHAR)
    //     {
    //         if (currentInput->keyChar == '+')
    //             cursorSize *= 1.25f;
    //         if (currentInput->keyChar == '-')
    //             cursorSize *= .8f;
    //     }

    BaseScreen::Input(currentInput);
}
