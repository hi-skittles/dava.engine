#include "ScenePreviewControl.h"

#include "Classes/Library/Private/ControlsFactory.h"

#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Deprecated/SceneValidator.h>

#include <TArc/Core/Deprecated.h>

#include <Entity/ComponentUtils.h>
#include <Scene3D/Components/CameraComponent.h>
#include <Scene3D/Components/Controller/RotationControllerComponent.h>
#include <Scene3D/Systems/Controller/WASDControllerSystem.h>
#include <Scene3D/Systems/Controller/RotationControllerSystem.h>
#include <UI/Update/UIUpdateComponent.h>

ScenePreviewControl::ScenePreviewControl(const DAVA::Rect& rect)
    : UI3DView(rect)
{
    SetName(DAVA::FastName("Preview_3D_View"));

    SetBasePriority(-100);
    SetInputEnabled(true, true);
    SetDrawToFrameBuffer(true);
    GetOrCreateComponent<DAVA::UIUpdateComponent>();
}

ScenePreviewControl::~ScenePreviewControl()
{
    ReleaseScene();
    rotationSystem = nullptr;
}

void ScenePreviewControl::RecreateScene()
{
    DVASSERT(editorScene == nullptr);

    editorScene = new DAVA::Scene();

    rotationSystem = new DAVA::RotationControllerSystem(editorScene);
    rotationSystem->SetRotationSpeeed(0.10f);
    editorScene->AddSystem(rotationSystem, DAVA::ComponentUtils::MakeMask<DAVA::CameraComponent>() | DAVA::ComponentUtils::MakeMask<DAVA::RotationControllerComponent>(),
                           DAVA::Scene::SCENE_SYSTEM_REQUIRE_PROCESS | DAVA::Scene::SCENE_SYSTEM_REQUIRE_INPUT);
}

void ScenePreviewControl::ReleaseScene()
{
    SetScene(nullptr);
    SafeRelease(editorScene);
    currentScenePath = DAVA::FilePath();
}

DAVA::int32 ScenePreviewControl::OpenScene(const DAVA::FilePath& pathToFile)
{
    ReleaseScene();
    RecreateScene();

    DAVA::int32 retError = DAVA::SceneFileV2::ERROR_NO_ERROR;

    if (pathToFile.IsEqualToExtension(".sc2"))
    {
        DAVA::SceneFileV2* file = new DAVA::SceneFileV2();
        file->EnableDebugLog(false);
        retError = file->LoadScene(pathToFile, editorScene);
        SafeRelease(file);
    }
    else
    {
        retError = ERROR_WRONG_EXTENSION;
    }

    CreateCamera();

    DAVA::SceneValidator::ExtractEmptyRenderObjects(editorScene);
    DAVA::SceneValidator validator;
    DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
    if (data)
    {
        validator.SetPathForChecking(data->GetProjectPath());
    }
    validator.ValidateScene(editorScene, pathToFile);

    SetScene(editorScene);

    return retError;
}

void ScenePreviewControl::Update(DAVA::float32 timeElapsed)
{
    UI3DView::Update(timeElapsed);

    if (needSetCamera)
    {
        needSetCamera = false;
        SetupCamera();
    }
}

void ScenePreviewControl::CreateCamera()
{
    auto sceneBox = editorScene->GetWTMaximumBoundingBoxSlow();

    DAVA::ScopedPtr<DAVA::Camera> camera(new DAVA::Camera());
    DAVA::ScopedPtr<DAVA::Entity> cameraNode(new DAVA::Entity());
    camera->SetUp(DAVA::Vector3(0.0f, 0.0f, 1.0f));
    camera->SetPosition(sceneBox.max * std::sqrt(3.0f));
    camera->SetTarget(sceneBox.min);
    camera->SetupPerspective(70.0f, 1.0f, 1.0f, 5000.0f);
    cameraNode->SetName("preview-camera");
    cameraNode->AddComponent(new DAVA::CameraComponent(camera));
    cameraNode->AddComponent(new DAVA::RotationControllerComponent());

    DAVA::ScopedPtr<DAVA::Light> light(new DAVA::Light());
    DAVA::ScopedPtr<DAVA::Entity> lightNode(new DAVA::Entity());
    light->SetIntensity(300.0f);
    light->SetDiffuseColor(DAVA::Color::White);
    light->SetAmbientColor(DAVA::Color::White);
    light->SetPosition(DAVA::Vector3(0.0f, 0.0f, sceneBox.max.z + std::abs(sceneBox.max.z)));
    light->SetType(DAVA::Light::TYPE_POINT);
    light->AddFlag(DAVA::Light::IS_DYNAMIC);
    light->AddFlag(DAVA::Light::CAST_SHADOW);
    lightNode->SetName("preview-light");
    lightNode->AddComponent(new DAVA::LightComponent(light));

    DAVA::TransformComponent* tc = lightNode->GetComponent<DAVA::TransformComponent>();
    tc->SetLocalTranslation(light->GetPosition());

    editorScene->AddNode(cameraNode);
    editorScene->AddNode(lightNode);

    editorScene->AddCamera(camera);
    editorScene->SetCurrentCamera(camera);
}

void ScenePreviewControl::SetupCamera()
{
    DAVA::Camera* camera = editorScene->GetCurrentCamera();
    if (camera && editorScene)
    {
        DAVA::AABBox3 sceneBox = editorScene->GetWTMaximumBoundingBoxSlow();
        DAVA::Vector3 target = sceneBox.GetCenter();
        camera->SetTarget(target);
        DAVA::Vector3 dir = (sceneBox.max - sceneBox.min);
        camera->SetPosition(target + dir);

        editorScene->SetCurrentCamera(camera);
        rotationSystem->RecalcCameraViewAngles(camera);
    }
}
