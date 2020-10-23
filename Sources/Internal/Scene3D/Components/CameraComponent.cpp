#include "Scene3D/Components/CameraComponent.h"
#include "Render/Highlevel/Camera.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(CameraComponent)
{
    ReflectionRegistrator<CameraComponent>::Begin()[M::CantBeCreatedManualyComponent()]
    .ConstructorByPointer()
    .Field("camera", &CameraComponent::GetCamera, &CameraComponent::SetCamera)[M::DisplayName("Camera"), M::FrequentlyChangedValue()]
    .End();
}

CameraComponent::CameraComponent(Camera* _camera)
{
    camera = SafeRetain(_camera);
}

CameraComponent::~CameraComponent()
{
    SafeRelease(camera);
}

Camera* CameraComponent::GetCamera()
{
    return camera;
}

void CameraComponent::SetCamera(Camera* _camera)
{
    SafeRelease(camera);
    camera = SafeRetain(_camera);
}

Component* CameraComponent::Clone(Entity* toEntity)
{
    CameraComponent* newComponent = new CameraComponent();
    newComponent->SetEntity(toEntity);
    newComponent->camera = static_cast<Camera*>(camera->Clone());

    return newComponent;
}

void CameraComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (NULL != archive && NULL != camera)
    {
        KeyedArchive* camArch = new KeyedArchive();
        camera->SaveObject(camArch);

        archive->SetArchive("cc.camera", camArch);

        camArch->Release();
    }
}

void CameraComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (NULL != archive)
    {
        KeyedArchive* camArch = archive->GetArchive("cc.camera");
        if (NULL != camArch)
        {
            Camera* cam = new Camera();
            cam->LoadObject(camArch);
            SetCamera(cam);
            cam->Release();
        }
    }

    Component::Deserialize(archive, serializationContext);
}
};