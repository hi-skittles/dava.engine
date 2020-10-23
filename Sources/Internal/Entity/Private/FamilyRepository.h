#pragma once

#include "Debug/DVAssert.h"
#include "Concurrency/Atomic.h"

namespace DAVA
{
template <typename EntityFamilyType>
class FamilyRepository
{
public:
    FamilyRepository() = default;
    ~FamilyRepository();

    EntityFamilyType* GetOrCreate(const EntityFamilyType& localFamily);
    void ReleaseFamily(EntityFamilyType* family);

    void ReleaseAllFamilies();

private:
    Vector<EntityFamilyType*> families;
    Atomic<int32> refCount;
};

template <typename EntityFamilyType>
FamilyRepository<EntityFamilyType>::~FamilyRepository()
{
    // DVASSERT(refCount == 0);
    ReleaseAllFamilies();
}

template <typename EntityFamilyType>
EntityFamilyType* FamilyRepository<EntityFamilyType>::GetOrCreate(const EntityFamilyType& localFamily)
{
    // Check whether family already is in cache
    auto iter = std::find_if(families.begin(), families.end(), [&localFamily](const EntityFamilyType* o) -> bool { return *o == localFamily; });
    if (iter == families.end())
    { // Family not found in cache so add it
        families.push_back(new EntityFamilyType(localFamily));
        iter = families.end() - 1;
    }
    (*iter)->refCount.Increment(); // Increase family ref counter
    refCount.Increment(); // Increase repository ref counter
    return *iter;
}

template <typename EntityFamilyType>
void FamilyRepository<EntityFamilyType>::ReleaseFamily(EntityFamilyType* family)
{
    if (family != nullptr)
    {
        DVASSERT(refCount.Get() > 0);
        DVASSERT(family->refCount.Get() > 0);

        family->refCount.Decrement();
        int32 newRefCount = refCount.Decrement();
        if (0 == newRefCount)
        {
            for (auto x : families)
            {
                DVASSERT(x->refCount.Get() == 0);
                delete x;
            }
            families.clear();
        }
    }
}

template <typename EntityFamilyType>
void FamilyRepository<EntityFamilyType>::ReleaseAllFamilies()
{
    // Release all families from cache and force repository's ref counter to 0
    refCount = 0;
    for (auto x : families)
    {
        delete x;
    }
    families.clear();
}
}