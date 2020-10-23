#include "Entity/Component.h"

#include "Base/ObjectFactory.h"
#include "Scene3D/Entity.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(Component)
{
    ReflectionRegistrator<Component>::Begin()
    .Field("entity", &Component::entity)[M::ReadOnly(), M::HiddenField()]
    .End();
}

Component::~Component()
{
    GlobalEventSystem::Instance()->RemoveAllEvents(this);
}

const Type* Component::GetType() const
{
    if (typeCache != nullptr)
    {
        return typeCache;
    }

    typeCache = ReflectedTypeDB::GetByPointer(this)->GetType();

    DVASSERT(typeCache != nullptr);

    return typeCache;
}

void Component::SetEntity(Entity* entity_)
{
    entity = entity_;
}

void Component::GetDataNodes(Set<DAVA::DataNode*>& dataNodes)
{
    //Empty as default
}

void Component::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (archive != nullptr)
    {
        archive->SetString("comp.typename", ObjectFactory::Instance()->GetName(this));
    }
}

void Component::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    // Do we need this?
}
}
