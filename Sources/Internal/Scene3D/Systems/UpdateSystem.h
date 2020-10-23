#ifndef __DAVAENGINE_SCENE3D_UPDATABLESYSTEM_H__
#define __DAVAENGINE_SCENE3D_UPDATABLESYSTEM_H__

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{
class IUpdatableBeforeTransform;
class IUpdatableAfterTransform;
class UpdateSystem : public SceneSystem
{
public:
    UpdateSystem(Scene* scene);

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;

    void UpdatePreTransform(float32 timeElapsed);
    void UpdatePostTransform(float32 timeElapsed);

private:
    Vector<IUpdatableBeforeTransform*> updatesBeforeTransform;
    Vector<IUpdatableAfterTransform*> updatesAfterTransform;
};
}

#endif //__DAVAENGINE_SCENE3D_UPDATABLESYSTEM_H__