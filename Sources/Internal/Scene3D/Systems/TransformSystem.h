#pragma once

#include "Base/BaseTypes.h"
#include "Math/MathConstants.h"
#include "Math/Matrix4.h"
#include "Base/Singleton.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{
class Entity;
class Transform;
class TransformComponent;
class Transform;

class TransformSystem : public SceneSystem
{
public:
    TransformSystem(Scene* scene);

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;

    void PrepareForRemove() override;
    void Process(float32 timeElapsed) override;

private:
    Vector<Entity*> updatableEntities;

    void EntityNeedUpdate(Entity* entity);
    void HierarchicAddToUpdate(Entity* entity);
    void FindNodeThatRequireUpdate(Entity* entity);
    void TransformAllChildEntities(Entity* entity);

    int32 passedNodes;
    int32 multipliedNodes;
};
};
