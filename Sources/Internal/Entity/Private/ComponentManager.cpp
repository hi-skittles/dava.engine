#include "Entity/ComponentManager.h"

#include "Entity/Component.h"
#include "Entity/ComponentUtils.h"
#include "Reflection/ReflectedTypeDB.h"
#include "Scene3D/Entity.h"
#include "Utils/CRC32.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
ComponentManager::ComponentManager()
{
    ComponentUtils::componentManager = this;

    DVASSERT(runtimeIdUserDataIndex == -1 && componentTypeUserDataIndex == -1);

    runtimeIdUserDataIndex = Type::AllocUserData();
    componentTypeUserDataIndex = Type::AllocUserData();
}

void ComponentManager::RegisterComponent(const Type* type)
{
    DVASSERT(type != nullptr);

    const Type* sceneComponentType = Type::Instance<Component>();
    const Type* uiComponentType = Type::Instance<UIComponent>();

    if (TypeInheritance::CanDownCast(type, sceneComponentType))
    {
        DVASSERT(!TypeInheritance::CanDownCast(type, uiComponentType));

        auto it = std::find(registeredSceneComponents.begin(), registeredSceneComponents.end(), type);

        if (it != registeredSceneComponents.end())
        {
            DVASSERT(false, "Component is already registered.");
            return;
        }

        ++runtimeSceneComponentsCount;

        type->SetUserData(runtimeIdUserDataIndex, Uint32ToVoidPtr(runtimeSceneComponentsCount));
        type->SetUserData(componentTypeUserDataIndex, Uint32ToVoidPtr(ComponentType::SCENE_COMPONENT));

        DVASSERT(static_cast<size_t>(runtimeSceneComponentsCount) < ComponentMask().size());

        registeredSceneComponents.push_back(type);

        DVASSERT(registeredSceneComponents.size() == runtimeSceneComponentsCount);

        UpdateSceneComponentsSortedVector(type);
    }
    else if (TypeInheritance::CanDownCast(type, uiComponentType))
    {
        DVASSERT(!TypeInheritance::CanDownCast(type, sceneComponentType));

        auto it = std::find(registeredUIComponents.begin(), registeredUIComponents.end(), type);

        if (it != registeredUIComponents.end())
        {
            DVASSERT(false, "UIComponent is already registered.");
            return;
        }

        ++runtimeUIComponentsCount;

        type->SetUserData(runtimeIdUserDataIndex, Uint32ToVoidPtr(runtimeUIComponentsCount));
        type->SetUserData(componentTypeUserDataIndex, Uint32ToVoidPtr(ComponentType::UI_COMPONENT));

        registeredUIComponents.push_back(type);
    }
    else
    {
        DVASSERT(false, "'type' should be derived from Component or UIComponent.");
    }
}

void ComponentManager::PreregisterAllDerivedSceneComponentsRecursively()
{
    DVASSERT(!componentsWerePreregistered);
    DVASSERT(runtimeSceneComponentsCount == 0);
    DVASSERT(registeredSceneComponents.empty());

    const TypeInheritance* inheritance = Type::Instance<Component>()->GetInheritance();

    if (inheritance == nullptr || inheritance->GetDerivedTypes().empty())
    {
        DVASSERT(false, "'Component' type has no derived types.");
        return;
    }

    const Vector<TypeInheritance::Info>& derivedTypes = inheritance->GetDerivedTypes();

    Vector<const Type*> componentsToRegister;
    componentsToRegister.reserve(derivedTypes.size() + derivedTypes.size() / 4); // reserve ~125% of derived types, since each type may have its own children

    for (const TypeInheritance::Info& info : derivedTypes)
    {
        CollectSceneComponentsRecursively(info.type, componentsToRegister);
    }

    const ReflectedTypeDB* db = ReflectedTypeDB::GetLocalDB();

    std::sort(componentsToRegister.begin(), componentsToRegister.end(), [db](const Type* l, const Type* r) {
        const ReflectedType* lReflType = db->GetByType(l);
        const ReflectedType* rReflType = db->GetByType(r);

        DVASSERT(lReflType != nullptr && rReflType != nullptr);

        const String& lName = lReflType->GetPermanentName();
        const String& rName = rReflType->GetPermanentName();

        DVASSERT(!lName.empty() && !rName.empty(), "Component permanent name is empty, order will be incorrect.");

        return lName < rName;
    });

    CRC32 crc;

    for (const Type* type : componentsToRegister)
    {
        RegisterComponent(type);
        const String& permanentName = db->GetByType(type)->GetPermanentName();
        crc.AddData(permanentName.data(), permanentName.size() * sizeof(String::value_type));
    }

    crc32HashOfPreregisteredComponents = crc.Done();

    componentsWerePreregistered = true;
}

uint32 ComponentManager::GetCRC32HashOfPreregisteredSceneComponents()
{
    DVASSERT(componentsWerePreregistered);

    return crc32HashOfPreregisteredComponents;
}

uint32 ComponentManager::GetCRC32HashOfRegisteredSceneComponents()
{
    const ReflectedTypeDB* db = ReflectedTypeDB::GetLocalDB();

    CRC32 crc;

    for (const auto& x : sceneComponentsSortedByPermanentName)
    {
        const Type* type = registeredSceneComponents[x.first];
        const String& permanentName = db->GetByType(type)->GetPermanentName();
        crc.AddData(permanentName.data(), permanentName.size() * sizeof(String::value_type));
    }

    return crc.Done();
}

uint32 ComponentManager::GetSortedComponentId(const Type* type)
{
    DVASSERT(IsRegisteredSceneComponent(type));

    uint32 runtimeId = GetRuntimeComponentId(type);

    DVASSERT(runtimeId < sceneComponentsSortedByPermanentName.size());

    return sceneComponentsSortedByPermanentName[runtimeId].second;
}

const Type* ComponentManager::GetSceneComponentType(uint32 runtimeId) const
{
    const Type* type = nullptr;

    if (runtimeId < registeredSceneComponents.size())
    {
        type = registeredSceneComponents[runtimeId];
    }

    return type;
}

void ComponentManager::CollectSceneComponentsRecursively(const Type* type, Vector<const Type*>& components)
{
    components.push_back(type);

    const TypeInheritance* inheritance = type->GetInheritance();

    if (inheritance != nullptr)
    {
        for (const TypeInheritance::Info& info : inheritance->GetDerivedTypes())
        {
            CollectSceneComponentsRecursively(info.type, components);
        }
    }
}

void ComponentManager::UpdateSceneComponentsSortedVector(const Type* type)
{
    const ReflectedTypeDB* db = ReflectedTypeDB::GetLocalDB();

    uint32 x = GetRuntimeComponentId(type);

    auto p = std::make_pair(x, x);

    // Keep sorted order
    auto position = std::lower_bound(sceneComponentsSortedByPermanentName.begin(), sceneComponentsSortedByPermanentName.end(), p, [this, db](const auto& l, const auto& r) {
        const ReflectedType* lReflType = db->GetByType(registeredSceneComponents[l.first]);
        const ReflectedType* rReflType = db->GetByType(registeredSceneComponents[r.first]);

        DVASSERT(lReflType != nullptr && rReflType != nullptr);

        const String& lName = lReflType->GetPermanentName();
        const String& rName = rReflType->GetPermanentName();

        DVASSERT(!lName.empty() && !rName.empty(), "Component permanent name is empty, order will be incorrect.");

        return lName < rName;
    });

    sceneComponentsSortedByPermanentName.insert(position, p);

    // Remap runtime ids to sorted ids
    for (size_t i = 0; i < sceneComponentsSortedByPermanentName.size(); ++i)
    {
        sceneComponentsSortedByPermanentName[sceneComponentsSortedByPermanentName[i].first].second = static_cast<uint32>(i);
    }
}
}