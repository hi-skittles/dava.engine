#include "Scene3D/Components/Waypoint/WaypointComponent.h"
#include "Scene3D/Entity.h"

#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(WaypointComponent)
{
    ReflectionRegistrator<WaypointComponent>::Begin()
    [M::CantBeCreatedManualyComponent(), M::CantBeDeletedManualyComponent(), M::NonExportableComponent(), M::NonSerializableComponent(), M::Tooltip("pathName")]
    .ConstructorByPointer()
    .Field("pathName", &WaypointComponent::GetPathName, nullptr)[M::ReadOnly(), M::DisplayName("Path Name")]
    .Field("properties", &WaypointComponent::GetProperties, nullptr)[M::DisplayName("Waypoint properties")]
    .End();
}

WaypointComponent::WaypointComponent()
    : Component()
{
}

WaypointComponent::~WaypointComponent()
{
}

void WaypointComponent::Init(PathComponent* path_, PathComponent::Waypoint* waypoint_)
{
    path = path_;
    waypoint = waypoint_;
}

PathComponent* WaypointComponent::GetPath() const
{
    return path;
}

PathComponent::Waypoint* WaypointComponent::GetWaypoint() const
{
    return waypoint;
}

Component* WaypointComponent::Clone(Entity* toEntity)
{
    WaypointComponent* newComponent = new WaypointComponent();
    newComponent->SetEntity(toEntity);
    newComponent->path = path;
    newComponent->waypoint = waypoint;
    return newComponent;
}

void WaypointComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    DVASSERT(false);
}

void WaypointComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    DVASSERT(false);
}
}
