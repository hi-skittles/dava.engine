#ifndef __DAVAENGINE_SCENE3D_ACTIONUPDATESYSTEM_H__
#define __DAVAENGINE_SCENE3D_ACTIONUPDATESYSTEM_H__

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"
#include "Scene3D/Components/ActionComponent.h"

namespace DAVA
{
class ActionComponent;
class ActionUpdateSystem : public SceneSystem
{
public:
    ActionUpdateSystem(Scene* scene);
    void Process(float32 timeElapsed) override;

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;

    void Watch(ActionComponent* component);
    void UnWatch(ActionComponent* component);

    void SetBlockEvent(ActionComponent::Action::eEvent eventType, bool block);
    bool IsBlockEvent(ActionComponent::Action::eEvent eventType);
    void UnblockAllEvents();

protected:
    bool eventBlocked[ActionComponent::Action::EVENTS_COUNT];
    Vector<ActionComponent*> activeActions;

    void DelayedDeleteActions();
    Vector<ActionComponent*> deleteActions;
};
}

#endif //__DAVAENGINE_SCENE3D_ACTIONUPDATESYSTEM_H__