#ifndef __DAVAENGINE_SCENE3D_GLOBALEVENTSYSTEM_H__
#define __DAVAENGINE_SCENE3D_GLOBALEVENTSYSTEM_H__

#include "Base/BaseTypes.h"
#include "Base/StaticSingleton.h"

namespace DAVA
{
class Component;
class Entity;
class Scene;

class GlobalEventSystem : public StaticSingleton<GlobalEventSystem>
{
public:
    void Event(Component* component, uint32 event);
    void GroupEvent(Scene* scene, Vector<Component*>& components, uint32 event);
    void PerformAllEventsFromCache(Component* component);
    void RemoveAllEvents(Component* component);

private:
    Map<Component*, List<uint32>> eventsCache;
};
}

#endif //__DAVAENGINE_SCENE3D_EVENTSYSTEM_H__