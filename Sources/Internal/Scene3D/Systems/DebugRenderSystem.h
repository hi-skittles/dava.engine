#ifndef __DAVAENGINE_SCENE3D_DEBUGRENDERSYSTEM_H__
#define __DAVAENGINE_SCENE3D_DEBUGRENDERSYSTEM_H__

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{
class Camera;

class DebugRenderSystem : public SceneSystem
{
public:
    DebugRenderSystem(Scene* scene);
    ~DebugRenderSystem();

    void Process(float32 timeElapsed) override;
    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;

    void SetCamera(Camera* camera);

private:
    Vector<Entity*> entities;
    Camera* camera;
};
}

#endif //__DAVAENGINE_SCENE3D_DEBUGRENDERSYSTEM_H__