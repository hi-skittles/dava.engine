#pragma once

#include "Base/UnordererMap.h"
#include "Base/Vector.h"

namespace DAVA
{
class EntityFamily;
class Entity;
class SortedEntityContainer
{
public:
    void Push(Entity* entity);
    void Clear();
    void EraseEntity(const Entity* entity);
    UnorderedMap<EntityFamily*, Vector<Entity*>> map;
};
}
