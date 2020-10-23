#pragma once

#include "Entity/Private/FamilyRepository.h"

namespace DAVA
{
class Component;
class Type;

class EntityFamily
{
private:
    EntityFamily(const Vector<Component*>& components);
    EntityFamily(const EntityFamily& other);

public:
    static EntityFamily* GetOrCreate(const Vector<Component*>& components);
    static void Release(EntityFamily*& family);

    uint32 GetComponentIndex(const Type* type, uint32 index) const;
    uint32 GetComponentsCount(const Type* type) const;
    const ComponentMask& GetComponentsMask() const;

    bool operator==(const EntityFamily& rhs) const;

private:
    Vector<uint32> componentsIndices;
    Vector<uint32> componentsCount;
    ComponentMask componentsMask;
    Atomic<int32> refCount;

    template <typename EntityFamilyType>
    friend class FamilyRepository;

    static FamilyRepository<EntityFamily> repository;
};
}
