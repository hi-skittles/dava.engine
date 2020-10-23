#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Scene.h"
#include "Entity/Component.h"

namespace DAVA
{
void GlobalEventSystem::GroupEvent(Scene* scene, Vector<Component*>& components, uint32 event)
{
    scene->GetEventSystem()->GroupNotifyAllSystems(components, event);
}

void GlobalEventSystem::Event(Component* component, uint32 event)
{
    if (component)
    {
        if (component->GetEntity() && component->GetEntity()->GetScene())
        {
            component->GetEntity()->GetScene()->GetEventSystem()->NotifyAllSystems(component, event);
            return;
        }

        List<uint32>& events = eventsCache[component];
        events.push_back(event);
    }
}

void GlobalEventSystem::PerformAllEventsFromCache(Component* component)
{
    auto it = eventsCache.find(component);
    if (it != eventsCache.end())
    {
        List<uint32>& list = it->second;

        for (List<uint32>::iterator listIt = list.begin(); listIt != list.end(); ++listIt)
        {
            component->GetEntity()->GetScene()->GetEventSystem()->NotifyAllSystems(component, *listIt);
        }

        eventsCache.erase(it);
    }
}

void GlobalEventSystem::RemoveAllEvents(Component* component)
{
    auto it = eventsCache.find(component);
    if (it != eventsCache.end())
    {
        eventsCache.erase(it);
    }
}
}