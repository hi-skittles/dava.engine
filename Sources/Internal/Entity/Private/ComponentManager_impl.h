#pragma once

#include "Entity/Component.h"

namespace DAVA
{
template <typename T>
void ComponentManager::RegisterComponent()
{
    static_assert(std::is_base_of<UIComponent, T>::value || std::is_base_of<Component, T>::value, "T should be derived from Component or UIComponent");

    const Type* type = Type::Instance<T>();

    RegisterComponent(type);
}

inline bool ComponentManager::IsRegisteredUIComponent(const Type* type) const
{
    DVASSERT(type != nullptr);

    return ((type->GetUserData(runtimeIdUserDataIndex) != nullptr) && (type->GetUserData(componentTypeUserDataIndex) == Uint32ToVoidPtr(ComponentType::UI_COMPONENT)));
}

inline bool ComponentManager::IsRegisteredSceneComponent(const Type* type) const
{
    DVASSERT(type != nullptr);

    return ((type->GetUserData(runtimeIdUserDataIndex) != nullptr) && (type->GetUserData(componentTypeUserDataIndex) == Uint32ToVoidPtr(ComponentType::SCENE_COMPONENT)));
}

inline uint32 ComponentManager::GetRuntimeComponentId(const Type* type) const
{
    DVASSERT(IsRegisteredUIComponent(type) || IsRegisteredSceneComponent(type));
    return VoidPtrToUint32(type->GetUserData(runtimeIdUserDataIndex)) - 1;
}

inline const Vector<const Type*>& ComponentManager::GetRegisteredUIComponents() const
{
    return registeredUIComponents;
}

inline const Vector<const Type*>& ComponentManager::GetRegisteredSceneComponents() const
{
    return registeredSceneComponents;
}

inline uint32 ComponentManager::GetUIComponentsCount() const
{
    return runtimeUIComponentsCount;
}

inline uint32 ComponentManager::GetSceneComponentsCount() const
{
    return runtimeSceneComponentsCount;
}

inline void* ComponentManager::Uint32ToVoidPtr(uint32 value) const
{
    return reinterpret_cast<void*>(static_cast<uintptr_t>(value));
}

inline uint32 ComponentManager::VoidPtrToUint32(void* ptr) const
{
    return static_cast<uint32>(reinterpret_cast<uintptr_t>(ptr));
}
}