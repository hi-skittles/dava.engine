#include "Scene3D/Components/Waypoint/EdgeComponent.h"
#include "Scene3D/Entity.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(EdgeComponent)
{
    ReflectionRegistrator<EdgeComponent>::Begin()
    [M::CantBeCreatedManualyComponent(), M::CantBeDeletedManualyComponent(), M::NonExportableComponent(), M::NonSerializableComponent()]
    .ConstructorByPointer()
    .Field("properties", &EdgeComponent::GetProperties, &EdgeComponent::SetProperties)[M::DisplayName("Edge properties")]
    .Field("nextEntityName", &EdgeComponent::GetNextEntityName, &EdgeComponent::SetNextEntityName)[M::ReadOnly(), M::DisplayName("Next Entity Name")]
    .End();
}

EdgeComponent::EdgeComponent()
    : Component()
{
}

EdgeComponent::~EdgeComponent()
{
}

EdgeComponent::EdgeComponent(const EdgeComponent& cp)
    : Component(cp)
{
    nextEntity = cp.nextEntity;
    path = cp.path;
    edge = cp.edge;
}

void EdgeComponent::Init(PathComponent* path_, PathComponent::Edge* edge_)
{
    path = path_;
    edge = edge_;
}

PathComponent* EdgeComponent::GetPath() const
{
    return path;
}

PathComponent::Edge* EdgeComponent::GetEdge() const
{
    return edge;
}

Component* EdgeComponent::Clone(Entity* toEntity)
{
    EdgeComponent* newComponent = new EdgeComponent();
    newComponent->SetEntity(toEntity);
    newComponent->nextEntity = nextEntity;
    newComponent->path = path;
    newComponent->edge = edge;

    return newComponent;
}

void EdgeComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    DVASSERT(false);
}

void EdgeComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    DVASSERT(false);
}

void EdgeComponent::SetProperties(KeyedArchive* archieve)
{
    DVASSERT(false);
}

void EdgeComponent::SetNextEntity(Entity* _entity)
{
    nextEntity = _entity;
}

void EdgeComponent::SetNextEntityName(const FastName& name)
{
    DVASSERT(false);
}

const FastName EdgeComponent::GetNextEntityName() const
{
    FastName nextEntityName;
    if (nextEntity)
    {
        nextEntityName = nextEntity->GetName();
    }

    return nextEntityName;
}
}
