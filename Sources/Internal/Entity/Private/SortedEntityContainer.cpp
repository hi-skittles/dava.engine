#include "Entity/SortedEntityContainer.h"
#include "Scene3D/Entity.h"
#include "Scene3D/EntityFamily.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
void SortedEntityContainer::Push(Entity* entity)
{
    DVASSERT(entity);
    map[entity->GetFamily()].push_back(entity);
}

void SortedEntityContainer::Clear()
{
    map.clear();
}

void SortedEntityContainer::EraseEntity(const Entity* entity)
{
    EntityFamily* family = entity->GetFamily();
    auto iter = map.find(family);
    if (iter != map.end())
    {
        Vector<Entity*>& vector = iter->second;
        size_t size = vector.size();
        for (size_t k = 0; k < size; ++k)
        {
            if (vector[k] == entity)
            {
                vector[k] = vector[size - 1];
                vector.pop_back();
                size--;
            }
        }
    }
}
}
