#ifndef __DAVAENGINE_RENDER_CULLINGSYSTEM_H__
#define __DAVAENGINE_RENDER_CULLINGSYSTEM_H__

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{
class RenderPass;
class RenderLayer;
class RenderObject;
class Entity;
class Camera;

class CullingSystem : public SceneSystem
{
public:
    CullingSystem(Scene* scene);
    virtual ~CullingSystem();

    virtual void AddEntity(Entity* entity);
    virtual void RemoveEntity(Entity* entity);
    virtual void ImmediateUpdate(Entity* entity);

    virtual void Process(float32 timeElapsed);

    void SetCamera(Camera* camera);

private:
    Vector<RenderObject*> renderObjectArray;
    Camera* camera = nullptr;
};

} // ns

#endif /* __DAVAENGINE_RENDER_CULLINGSYSTEM_H__ */
