#pragma once

#include "Base/Vector.h"
#include "Entity/SortedEntityContainer.h"

namespace DAVA
{
class Entity;
class TransformSingleComponent
{
public:
    Vector<Entity*> localTransformChanged;
    Vector<Entity*> transformParentChanged;
    SortedEntityContainer worldTransformChanged; //sorted by EntityFamily in TransformSystem
    Vector<Entity*> animationTransformChanged;

    void Clear();
    void EraseEntity(const Entity* entity);
};
}
