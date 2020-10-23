#ifndef __DAVAENGINE_SCENE3D_SOUNDUPDATESYSTEM_H__
#define __DAVAENGINE_SCENE3D_SOUNDUPDATESYSTEM_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{
class SoundEvent;
class SoundComponent;
class SoundUpdateSystem : public SceneSystem
{
    struct AutoTriggerSound
    {
        AutoTriggerSound(Entity* owner, SoundEvent* sound);

        Entity* owner;
        SoundEvent* soundEvent;
        float32 maxSqDistance;
    };

public:
    SoundUpdateSystem(Scene* scene);
    ~SoundUpdateSystem() override;

    void ImmediateEvent(Component* component, uint32 event) override;
    void Process(float32 timeElapsed) override;
    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;

    void Activate() override;
    void Deactivate() override;

protected:
    void AddAutoTriggerSound(Entity* soundOwner, SoundEvent* sound);
    void RemoveAutoTriggerSound(Entity* soundOwner, SoundEvent* sound = 0);

    Vector<AutoTriggerSound> autoTriggerSounds;

    Vector<Entity*> sounds;
    Vector<SoundEvent*> pausedEvents;

    friend class SoundComponent;
};

} // ns

#endif /* __DAVAENGINE_SCENE3D_SOUNDUPDATESYSTEM_H__ */
