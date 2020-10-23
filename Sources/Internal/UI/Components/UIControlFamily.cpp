#include "UI/Components/UIControlFamily.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"

namespace DAVA
{
// call constructor on first use not globally
FamilyRepository<UIControlFamily>& GetUIControlFamilyRepository()
{
    static FamilyRepository<UIControlFamily> repository;
    return repository;
}

UIControlFamily* UIControlFamily::GetOrCreate(const Vector<UIComponent*>& components)
{
    return GetUIControlFamilyRepository().GetOrCreate(UIControlFamily(components));
}

UIControlFamily::UIControlFamily(const Vector<UIComponent*>& components)
{
    ComponentManager* cm = GetEngineContext()->componentManager;
    componentIndices.resize(cm->GetUIComponentsCount());
    componentsCount.resize(cm->GetUIComponentsCount());

    int32 size = static_cast<int32>(components.size());
    hash = size;
    for (int32 i = size - 1; i >= 0; --i)
    {
        int32 type = components[i]->GetRuntimeType();
        hash ^= type + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        componentIndices[type] = i;
        componentsCount[type]++;
    }
}

UIControlFamily::UIControlFamily(const UIControlFamily& other)
{
    componentIndices = other.componentIndices;
    componentsCount = other.componentsCount;
    hash = other.hash;
    refCount.Set(other.refCount.Get());
}

uint32 UIControlFamily::GetComponentIndex(int32 runtimeType, uint32 index) const
{
    DVASSERT(GetComponentsCount(runtimeType) >= index);

    return componentIndices[runtimeType] + index;
}

uint32 UIControlFamily::GetComponentsCount(int32 runtimeType) const
{
    DVASSERT(runtimeType < static_cast<int32>(componentsCount.size()));

    return componentsCount[runtimeType];
}

void UIControlFamily::Release(UIControlFamily*& family)
{
    GetUIControlFamilyRepository().ReleaseFamily(family);
    family = nullptr;
}

bool UIControlFamily::operator==(const UIControlFamily& rhs) const
{
    return (hash == rhs.hash) && (componentsCount == rhs.componentsCount);
}
}
