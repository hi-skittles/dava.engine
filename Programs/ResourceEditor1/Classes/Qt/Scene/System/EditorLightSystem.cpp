#include "Scene/System/EditorLightSystem.h"
#include "Qt/Scene/SceneEditor2.h"
#include "Qt/Scene/SceneSignals.h"
#include "Commands2/RECommandIDs.h"
#include "StringConstants.h"
#include "Constants.h"

#include <Entity/ComponentUtils.h>

using namespace DAVA;

EditorLightSystem::EditorLightSystem(DAVA::Scene* scene)
    : DAVA::SceneSystem(scene)
{
    ScopedPtr<Light> light(new Light());
    light->SetType(Light::TYPE_POINT);
    light->SetAmbientColor(DAVA::Color(0.3f, 0.3f, 0.3f, 1.0f));

    cameraLight = new DAVA::Entity();
    cameraLight->SetLocked(true);
    cameraLight->SetName(ResourceEditor::EDITOR_CAMERA_LIGHT);
    cameraLight->AddComponent(new LightComponent(light));

    SetRequiredComponents(ComponentUtils::MakeMask<LightComponent>());

    if (isEnabled)
    {
        AddCameraLightOnScene();
    }
}

EditorLightSystem::~EditorLightSystem()
{
    SafeRelease(cameraLight);
}

void EditorLightSystem::UpdateCameraLightState()
{
    if (isEnabled && lightEntities == 0)
    {
        AddCameraLightOnScene();
    }
    else if (!isEnabled || lightEntities != 0)
    {
        RemoveCameraLightFromScene();
    }
}

void EditorLightSystem::UpdateCameraLightPosition()
{
    if (cameraLight && cameraLight->GetParent())
    {
        Camera* camera = GetScene()->GetCurrentCamera();
        if (!camera)
            return;

        Matrix4 m = Matrix4::MakeTranslation(camera->GetPosition() - camera->GetLeft() * 20.f + camera->GetUp() * 20.f);
        if (m != cameraLight->GetLocalTransform())
        {
            cameraLight->SetLocalTransform(m);
        }
    }
}

void EditorLightSystem::SetCameraLightEnabled(bool enabled)
{
    if (enabled != isEnabled)
    {
        isEnabled = enabled;
        UpdateCameraLightState();
    }
}

void EditorLightSystem::AddCameraLightOnScene()
{
    SceneEditor2* sc = static_cast<SceneEditor2*>(GetScene());
    if (cameraLight->GetParent() == nullptr)
    {
        sc->AddEditorEntity(cameraLight);
    }
}

void EditorLightSystem::RemoveCameraLightFromScene()
{
    if (cameraLight && cameraLight->GetParent())
    {
        cameraLight->GetParent()->RemoveNode(cameraLight);
    }
}

void EditorLightSystem::SceneDidLoaded()
{
    if (isEnabled && lightEntities == 0)
    {
        AddCameraLightOnScene();
    }
}

void EditorLightSystem::AddEntity(DAVA::Entity* entity)
{
    DVASSERT(GetLightComponent(entity) != nullptr);
    if (entity == cameraLight)
    {
        return;
    }

    ++lightEntities;
    RemoveCameraLightFromScene();
}

void EditorLightSystem::RemoveEntity(DAVA::Entity* entity)
{
    if (entity == cameraLight)
    {
        return;
    }

    --lightEntities;

    if (isEnabled && lightEntities == 0)
    {
        AddCameraLightOnScene();
    }
}

void EditorLightSystem::PrepareForRemove()
{
}

void EditorLightSystem::Process(float32 timeElapsed)
{
    if (isEnabled)
    {
        UpdateCameraLightPosition();
    }
}
