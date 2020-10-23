#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Components/SoundComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Components/SingleComponents/TransformSingleComponent.h"
#include "Scene3D/Systems/SoundUpdateSystem.h"
#include "Sound/SoundSystem.h"
#include "Sound/SoundEvent.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Engine/Engine.h"

namespace DAVA
{
SoundUpdateSystem::AutoTriggerSound::AutoTriggerSound(Entity* _owner, SoundEvent* _sound)
    : owner(_owner)
    , soundEvent(_sound)
{
    float32 distance = soundEvent->GetMaxDistance();
    maxSqDistance = distance * distance;
}

SoundUpdateSystem::SoundUpdateSystem(Scene* scene)
    : SceneSystem(scene)
{
    scene->GetEventSystem()->RegisterSystemForEvent(this, EventSystem::SOUND_COMPONENT_CHANGED);
}

SoundUpdateSystem::~SoundUpdateSystem()
{
    DVASSERT(sounds.size() == 0);
    pausedEvents.clear();
}

void SoundUpdateSystem::ImmediateEvent(Component* component, uint32 event)
{
    if (event == EventSystem::SOUND_COMPONENT_CHANGED)
    {
        const Matrix4& worldTransform = GetTransformComponent(component->GetEntity())->GetWorldMatrix();
        Vector3 translation = worldTransform.GetTranslationVector();

        SoundComponent* sc = GetSoundComponent(component->GetEntity());
        DVASSERT(sc);

        uint32 eventsCount = sc->GetEventsCount();
        for (uint32 i = 0; i < eventsCount; ++i)
        {
            SoundEvent* sound = sc->GetSoundEvent(i);
            sound->SetPosition(translation);
            if (sound->IsDirectional())
            {
                Vector3 worldDirection = MultiplyVectorMat3x3(sc->GetLocalDirection(i), worldTransform);
                sound->SetDirection(worldDirection);
            }
            sound->UpdateInstancesPosition();
        }
    }
}

void SoundUpdateSystem::Process(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::SCENE_SOUND_UPDATE_SYSTEM);

    TransformSingleComponent* tsc = GetScene()->transformSingleComponent;

    for (auto& pair : tsc->worldTransformChanged.map)
    {
        if (pair.first->GetComponentsCount(Type::Instance<SoundComponent>()) > 0)
        {
            for (Entity* entity : pair.second)
            {
                SoundComponent* sc = entity->GetComponent<SoundComponent>();
                const Matrix4& worldTransform = GetTransformComponent(entity)->GetWorldMatrix();
                Vector3 translation = worldTransform.GetTranslationVector();

                uint32 eventsCount = sc->GetEventsCount();
                for (uint32 i = 0; i < eventsCount; ++i)
                {
                    SoundEvent* sound = sc->GetSoundEvent(i);
                    sound->SetPosition(translation);
                    if (sound->IsDirectional())
                    {
                        Vector3 worldDirection = MultiplyVectorMat3x3(sc->GetLocalDirection(i), worldTransform);
                        sound->SetDirection(worldDirection);
                    }
                    sound->UpdateInstancesPosition();
                }
            }
        }
    }

    Camera* activeCamera = GetScene()->GetCurrentCamera();

    if (activeCamera)
    {
        SoundSystem* ss = GetEngineContext()->soundSystem;
        const Vector3& listenerPosition = activeCamera->GetPosition();
        ss->SetListenerPosition(listenerPosition);
        ss->SetListenerOrientation(activeCamera->GetDirection(), activeCamera->GetLeft());

        uint32 autoCount = static_cast<uint32>(autoTriggerSounds.size());
        for (uint32 i = 0; i < autoCount; ++i)
        {
            AutoTriggerSound& autoTriggerSound = autoTriggerSounds[i];
            float32 distanceSq = (listenerPosition - GetTransformComponent(autoTriggerSound.owner)->GetWorldMatrixPtr()->GetTranslationVector()).SquareLength();
            if (distanceSq < autoTriggerSound.maxSqDistance)
            {
                if (!autoTriggerSound.soundEvent->IsActive())
                    autoTriggerSound.soundEvent->Trigger();
            }
            else
            {
                if (autoTriggerSound.soundEvent->IsActive())
                    autoTriggerSound.soundEvent->Stop();
            }
        }
    }
}

void SoundUpdateSystem::AddEntity(Entity* entity)
{
    SoundComponent* sc = GetSoundComponent(entity);
    DVASSERT(sc);

    uint32 eventsCount = sc->GetEventsCount();
    for (uint32 i = 0; i < eventsCount; ++i)
    {
        if ((sc->GetSoundEventFlags(i) & SoundComponent::FLAG_AUTO_DISTANCE_TRIGGER) > 0 && sc->GetSoundEvent(i)->IsActive())
        {
            AddAutoTriggerSound(entity, sc->GetSoundEvent(i));
        }
    }

    sounds.push_back(entity);
}

void SoundUpdateSystem::RemoveEntity(Entity* entity)
{
    SoundComponent* sc = GetSoundComponent(entity);
    DVASSERT(sc);

    uint32 eventsCount = sc->GetEventsCount();
    for (uint32 i = 0; i < eventsCount; ++i)
    {
        SoundEvent* sound = sc->GetSoundEvent(i);
        sound->Stop(true);
    }

    RemoveAutoTriggerSound(entity);
    FindAndRemoveExchangingWithLast(sounds, entity);
}

void SoundUpdateSystem::PrepareForRemove()
{
    autoTriggerSounds.clear();
    sounds.clear();
    pausedEvents.clear();
}

void SoundUpdateSystem::AddAutoTriggerSound(Entity* soundOwner, SoundEvent* sound)
{
    int32 soundsCount = static_cast<int32>(autoTriggerSounds.size());
    for (int32 i = 0; i < soundsCount; ++i)
    {
        if (autoTriggerSounds[i].owner == soundOwner && autoTriggerSounds[i].soundEvent == sound)
        {
            return;
        }
    }

    autoTriggerSounds.push_back(AutoTriggerSound(soundOwner, sound));
}

void SoundUpdateSystem::RemoveAutoTriggerSound(Entity* soundOwner, SoundEvent* sound /* = 0 */)
{
    for (int32 i = static_cast<int32>(autoTriggerSounds.size() - 1); i >= 0; --i)
    {
        if (autoTriggerSounds[i].owner == soundOwner && (sound == 0 || autoTriggerSounds[i].soundEvent == sound))
        {
            RemoveExchangingWithLast(autoTriggerSounds, i);
        }
    }
}

void SoundUpdateSystem::Deactivate()
{
    DVASSERT(pausedEvents.size() == 0);

    for (auto entity : sounds)
    {
        auto sound = DAVA::GetSoundComponent(entity);
        DVASSERT(sound);

        auto eventCount = sound->GetEventsCount();
        for (uint32 i = 0; i < eventCount; ++i)
        {
            auto soundEvent = sound->GetSoundEvent(i);
            if (soundEvent->IsActive())
            {
                auto flags = sound->GetSoundEventFlags(i);
                if ((flags & DAVA::SoundComponent::FLAG_AUTO_DISTANCE_TRIGGER) == DAVA::SoundComponent::FLAG_AUTO_DISTANCE_TRIGGER)
                {
                    soundEvent->Stop();
                }
                else
                {
                    soundEvent->SetPaused(true);
                    pausedEvents.push_back(soundEvent);
                }
            }
        }
    }
}

void SoundUpdateSystem::Activate()
{
    for (auto soundEvent : pausedEvents)
    {
        soundEvent->SetPaused(false);
    }

    pausedEvents.clear();
}
};
