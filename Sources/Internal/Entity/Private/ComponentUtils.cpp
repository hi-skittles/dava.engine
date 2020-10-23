#include "Entity/ComponentUtils.h"

#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Entity/Component.h"
#include "Reflection/ReflectedTypeDB.h"

namespace DAVA
{
ComponentManager* ComponentUtils::componentManager = nullptr;

Component* ComponentUtils::Create(const Type* type)
{
    DVASSERT(type != nullptr);

    if (TypeInheritance::CanDownCast(type, Type::Instance<Component>()))
    {
        const ReflectedType* reflType = ReflectedTypeDB::GetByType(type);
        Any obj = reflType->CreateObject(ReflectedType::CreatePolicy::ByPointer);
        return static_cast<Component*>(obj.Get<void*>());
    }

    DVASSERT(false, "'type' is not derived from Component or not registered in the reflection db.");

    return nullptr;
}

Component* ComponentUtils::Create(uint32 runtimeId)
{
    const Type* type = componentManager->GetSceneComponentType(runtimeId);

    return Create(type);
}

uint32 ComponentUtils::GetRuntimeId(const Type* type)
{
    DVASSERT(type != nullptr);

    uint32 runtimeId = componentManager->GetRuntimeComponentId(type);

    return runtimeId;
}

const Type* ComponentUtils::GetType(uint32 runtimeId)
{
    const Type* type = componentManager->GetSceneComponentType(runtimeId);

    return type;
}

uint32 ComponentUtils::GetSortedId(const Type* type)
{
    DVASSERT(type != nullptr);

    uint32 sortedId = componentManager->GetSortedComponentId(type);

    return sortedId;
}

} // namespace DAVA