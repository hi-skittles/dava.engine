#ifndef __DAVAENGINE_SNAPTOLANDSCAPE_CONTROLLER_SYSTEM_H__
#define __DAVAENGINE_SNAPTOLANDSCAPE_CONTROLLER_SYSTEM_H__

#include "Entity/SceneSystem.h"
#include "Math/Vector.h"

namespace DAVA
{
class Camera;
class Landscape;
class SnapToLandscapeControllerSystem : public SceneSystem
{
public:
    SnapToLandscapeControllerSystem(Scene* scene);
    ~SnapToLandscapeControllerSystem() override;

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;

    void Process(float32 timeElapsed) override;
    void ImmediateEvent(Component* component, uint32 event) override;

private:
    void SnapToLandscape(Landscape* landscape, Entity* entity, bool forceSnap = false);

    Vector<Entity*> entities;
    Map<Entity*, Vector3> positions;
};
};

#endif //__DAVAENGINE_SNAPTOLANDSCAPE_CONTROLLER_SYSTEM_H__