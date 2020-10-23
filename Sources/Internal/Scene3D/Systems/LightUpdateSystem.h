#ifndef __DAVAENGINE_SCENE3D_LIGHTUPDATESYSTEM_H__
#define __DAVAENGINE_SCENE3D_LIGHTUPDATESYSTEM_H__

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{
class RenderPass;
class RenderLayer;
class RenderObject;
class RenderBatch;
class Entity;
class Camera;
class Light;

/**
    \brief This system is required to transfer all changes from scene to render system and update render object properties.
 */
class LightUpdateSystem : public SceneSystem
{
public:
    LightUpdateSystem(Scene* scene);

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;
    void Process(float32 timeElapsed) override;

private:
    void RecalcLight(Entity* entity);
    UnorderedMap<Entity*, Light*> entityObjectMap;
};

} // ns

#endif /* __DAVAENGINE_SCENE3D_LIGHTUPDATESYSTEM_H__ */
