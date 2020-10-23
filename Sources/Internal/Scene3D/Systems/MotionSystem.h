#pragma once

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Entity/SceneSystem.h"
#include "Scene3D/Components/SkeletonComponent.h"

namespace DAVA
{
class Component;
class Entity;
class MotionComponent;
class MotionSingleComponent;
class SimpleMotion;

class MotionSystem : public SceneSystem
{
public:
    MotionSystem(Scene* scene);
    ~MotionSystem();

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;

    void ImmediateEvent(Component* component, uint32 event) override;
    void Process(float32 timeElapsed) override;

protected:
    void SetScene(Scene* scene) override;

private:
    void UpdateMotionLayers(MotionComponent* motionComponent, float32 dTime);

    Vector<MotionComponent*> activeComponents;
    MotionSingleComponent* motionSingleComponent = nullptr;
};

} //ns
