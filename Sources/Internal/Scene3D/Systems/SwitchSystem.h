#ifndef __DAVAENGINE_SCENE3D_SWITCHSYSTEM_H__
#define __DAVAENGINE_SCENE3D_SWITCHSYSTEM_H__

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{
class SwitchSystem : public SceneSystem
{
public:
    SwitchSystem(Scene* scene);

    void Process(float32 timeElapsed) override;
    void ImmediateEvent(Component* component, uint32 event) override;
    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;

private:
    Set<Entity*> updatableEntities;

    void SetSwitchHierarchy(Entity* entity, int32 switchIndex);
};
}

#endif //__DAVAENGINE_SCENE3D_SWITCHSYSTEM_H__
