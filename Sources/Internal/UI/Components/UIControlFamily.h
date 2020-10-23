#pragma once

#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"
#include "Entity/Private/FamilyRepository.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
class UIControlFamily
{
public:
    uint32 GetComponentIndex(int32 runtimeType, uint32 index) const;
    uint32 GetComponentsCount(int32 runtimeType) const;

    static UIControlFamily* GetOrCreate(const Vector<UIComponent*>& components);
    static void Release(UIControlFamily*& family);

    bool operator==(const UIControlFamily& rhs) const;

private:
    UIControlFamily(const Vector<UIComponent*>& components);
    UIControlFamily(const UIControlFamily& other);

    Atomic<int32> refCount;
    int32 hash = 0;

    Vector<uint32> componentIndices;
    Vector<uint32> componentsCount;

    template <typename EntityFamilyType>
    friend class FamilyRepository;
};
}
