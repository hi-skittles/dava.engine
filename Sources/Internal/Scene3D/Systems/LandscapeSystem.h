#ifndef __DAVAENGINE_SCENE3D_LANDSCAPESYSTEM_H__
#define __DAVAENGINE_SCENE3D_LANDSCAPESYSTEM_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{
class Entity;
class Landscape;

class LandscapeSystem : public SceneSystem
{
public:
    LandscapeSystem(Scene* scene);
    virtual ~LandscapeSystem();

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;
    void Process(float32 timeElapsed) override;

    Vector<Landscape*> GetLandscapeObjects();
    const Vector<Entity*>& GetLandscapeEntities();

protected:
    void DrawPatchMetrics(Landscape* landscape, uint32 subdivLevel, uint32 x, uint32 y);

    Vector<Entity*> landscapeEntities;
};

} // ns

#endif /* __DAVAENGINE_SCENE3D_LANDSCAPESYSTEM_H__ */
