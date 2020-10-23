#ifndef __DAVAENGINE_SCENE3D_SPEEDTREEUPDATESYSTEM_H__
#define __DAVAENGINE_SCENE3D_SPEEDTREEUPDATESYSTEM_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/Observer.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{
class Entity;
class SpeedTreeComponent;

class SpeedTreeUpdateSystem : public SceneSystem, public Observer
{
public:
    SpeedTreeUpdateSystem(Scene* scene);
    ~SpeedTreeUpdateSystem() override;

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;
    void ImmediateEvent(Component* component, uint32 event) override;
    void Process(float32 timeElapsed) override;

    void HandleEvent(Observable* observable) override;

    void SceneDidLoaded() override;

protected:
    void UpdateAnimationFlag(Entity* entity);

private:
    Vector<SpeedTreeComponent*> allTrees;

    bool isAnimationEnabled;
    bool isVegetationAnimationEnabled;

    friend class SpeedTreeComponent;
};

} // ns

#endif /* __DAVAENGINE_SCENE3D_SPEEDTREEUPDATESYSTEM_H__ */
