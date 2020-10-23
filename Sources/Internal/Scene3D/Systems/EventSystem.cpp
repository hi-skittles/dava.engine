#include "Scene3D/Systems/EventSystem.h"
#include "Entity/SceneSystem.h"
#include "Scene3D/Entity.h"

namespace DAVA
{
void EventSystem::RegisterSystemForEvent(SceneSystem* system, uint32 event)
{
    registeredSystems[event].push_back(system);
}

void EventSystem::UnregisterSystemForEvent(SceneSystem* system, uint32 event)
{
    Vector<SceneSystem*>& container = registeredSystems[event];
    uint32 size = static_cast<uint32>(container.size());
    for (uint32 i = 0; i < size; ++i)
    {
        if (container[i] == system)
        {
            container[i] = container[size - 1];
            container.pop_back();
            return;
        }
    }
}

void EventSystem::GroupNotifyAllSystems(Vector<Component*>& components, uint32 event)
{
    Vector<SceneSystem*>& container = registeredSystems[event];
    uint32 size = static_cast<uint32>(container.size());
    for (uint32 i = 0; i < size; ++i)
    {
        SceneSystem* system = container[i];
        const ComponentMask& requiredComponentMask = system->GetRequiredComponents();

        uint32 componentsVectorSize = static_cast<uint32>(components.size());
        for (uint32 k = 0; k < componentsVectorSize; ++k)
        {
            Component* comp = components[k];
            Entity* entity = comp->GetEntity();
            const ComponentMask& componentsInEntity = entity->GetAvailableComponentMask();

            if ((requiredComponentMask & componentsInEntity) == requiredComponentMask)
                system->ImmediateEvent(comp, event);
        }
    }
}

void EventSystem::NotifyAllSystems(Component* component, uint32 event)
{
    Vector<SceneSystem*>& container = registeredSystems[event];
    uint32 size = static_cast<uint32>(container.size());
    const ComponentMask& componentsInEntity = component->GetEntity()->GetAvailableComponentMask();
    for (uint32 i = 0; i < size; ++i)
    {
        SceneSystem* system = container[i];
        const ComponentMask& requiredComponentMask = system->GetRequiredComponents();
        if ((requiredComponentMask & componentsInEntity) == requiredComponentMask)
            system->ImmediateEvent(component, event);
    }
}

void EventSystem::NotifySystem(SceneSystem* system, Component* component, uint32 event)
{
    Vector<SceneSystem*>& container = registeredSystems[event];
    uint32 size = static_cast<uint32>(container.size());
    for (uint32 i = 0; i < size; ++i)
    {
        if (system == container[i])
        {
            system->ImmediateEvent(component, event);
            return;
        }
    }
}
}