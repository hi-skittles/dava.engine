#include "Scene3D/Components/WindComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "FileSystem/KeyedArchive.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(WindComponent)
{
    ReflectionRegistrator<WindComponent>::Begin()[M::CantBeCreatedManualyComponent()]
    .ConstructorByPointer()
    .Field("influenceBbox", &WindComponent::GetInfluenceBBox, &WindComponent::SetInfluenceBBox)[M::DisplayName("Influence Bounding Box")]
    .Field("windForce", &WindComponent::GetWindForce, &WindComponent::SetWindForce)[M::DisplayName("Wind force")]
    .Field("windSpeed", &WindComponent::GetWindSpeed, &WindComponent::SetWindSpeed)[M::DisplayName("Wind speed")]
    .End();
}

WindComponent::WindComponent()
    : influenceBbox(Vector3(), 10000.f)
    , windForce(1.f)
    , windSpeed(1.f)
{
}

WindComponent::~WindComponent()
{
}

Component* WindComponent::Clone(Entity* toEntity)
{
    WindComponent* component = new WindComponent();
    component->SetEntity(toEntity);

    component->windForce = windForce;
    component->windSpeed = windSpeed;
    component->influenceBbox = influenceBbox;

    return component;
}

void WindComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (archive != 0)
    {
        archive->SetFloat("wc.windForce", windForce);
        archive->SetFloat("wc.windSpeed", windSpeed);
        archive->SetVariant("wc.aabbox", VariantType(influenceBbox));
    }
}

void WindComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (archive)
    {
        windForce = archive->GetFloat("wc.windForce");
        windSpeed = archive->GetFloat("wc.windSpeed");
        influenceBbox = archive->GetVariant("wc.aabbox")->AsAABBox3();
    }

    Component::Deserialize(archive, serializationContext);
}

Vector3 WindComponent::GetDirection() const
{
    DVASSERT(entity);
    DVASSERT(GetTransformComponent(entity));

    const Matrix4& wtMx = GetTransformComponent(entity)->GetWorldMatrix();

    return Vector3(wtMx._00, wtMx._01, wtMx._02); //Get world direction only: wtMx * Vec3(1, 0, 0)
}
};
