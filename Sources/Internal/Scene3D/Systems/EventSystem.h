#ifndef __DAVAENGINE_SCENE3D_EVENTSYSTEM_H__
#define __DAVAENGINE_SCENE3D_EVENTSYSTEM_H__

#include "Base/BaseTypes.h"

namespace DAVA
{
class SceneSystem;
class Component;
class EventSystem
{
public:
    enum eEventType
    {
        SWITCH_CHANGED,
        START_PARTICLE_EFFECT,
        STOP_PARTICLE_EFFECT,
        START_ANIMATION,
        STOP_ANIMATION,
        MOVE_ANIMATION_TO_THE_LAST_FRAME,
        MOVE_ANIMATION_TO_THE_FIRST_FRAME,
        SPEED_TREE_MAX_ANIMATED_LOD_CHANGED,
        WAVE_TRIGGERED,
        SOUND_COMPONENT_CHANGED,
        STATIC_OCCLUSION_COMPONENT_CHANGED,
        SKELETON_CONFIG_CHANGED,
        SNAP_TO_LANDSCAPE_HEIGHT_CHANGED,
        LOD_DISTANCE_CHANGED,
        LOD_RECURSIVE_UPDATE_ENABLED,
        GEO_DECAL_CHANGED,

        EVENTS_COUNT
    };

    void RegisterSystemForEvent(SceneSystem* system, uint32 event);
    void UnregisterSystemForEvent(SceneSystem* system, uint32 event);
    void NotifySystem(SceneSystem* system, Component* component, uint32 event);
    void NotifyAllSystems(Component* component, uint32 event);
    void GroupNotifyAllSystems(Vector<Component*>& components, uint32 event);

private:
    Vector<SceneSystem*> registeredSystems[EVENTS_COUNT];
};
}

#endif //__DAVAENGINE_SCENE3D_EVENTSYSTEM_H__