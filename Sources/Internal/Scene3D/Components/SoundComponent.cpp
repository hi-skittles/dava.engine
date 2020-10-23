#include "Sound/SoundSystem.h"
#include "Sound/SoundEvent.h"
#include "Base/FastName.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Components/SoundComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Systems/GlobalEventSystem.h"
#include "Scene3D/Systems/SoundUpdateSystem.h"
#include "Utils/Utils.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(SoundComponentElement)
{
    ReflectionRegistrator<SoundComponentElement>::Begin()
    .End();
}

template <>
bool AnyCompare<SoundComponentElement>::IsEqual(const Any& v1, const Any& v2)
{
    return v1.Get<SoundComponentElement>() == v2.Get<SoundComponentElement>();
}

bool SoundComponentElement::operator==(const SoundComponentElement& other) const
{
    return soundEvent == other.soundEvent &&
    localDirection == other.localDirection &&
    flags == other.flags;
}

DAVA_VIRTUAL_REFLECTION_IMPL(SoundComponent)
{
    ReflectionRegistrator<SoundComponent>::Begin()
    .ConstructorByPointer()
    .Field("events", &SoundComponent::events)[M::DisplayName("Events")]
    .End();
}

SoundComponent::SoundComponent()
{
}

SoundComponent::~SoundComponent()
{
    RemoveAllEvents();
}

void SoundComponent::AddSoundEvent(SoundEvent* _event, uint32 flags /*= 0*/, const Vector3& direction /*= Vector3(1.f, 0.f, 0.f)*/)
{
    DVASSERT(_event);

    SafeRetain(_event);
    events.push_back(SoundComponentElement(_event, flags, direction));

    GlobalEventSystem::Instance()->Event(this, EventSystem::SOUND_COMPONENT_CHANGED);
}

void SoundComponent::RemoveSoundEvent(SoundEvent* event)
{
    uint32 eventCount = static_cast<uint32>(events.size());
    for (uint32 i = 0; i < eventCount; ++i)
    {
        if (events[i].soundEvent == event)
        {
            events[i].soundEvent->Stop(true);
            SafeRelease(events[i].soundEvent);
            RemoveExchangingWithLast(events, i);

            return;
        }
    }
}

void SoundComponent::RemoveAllEvents()
{
    uint32 eventsCount = static_cast<uint32>(events.size());
    for (uint32 i = 0; i < eventsCount; ++i)
    {
        events[i].soundEvent->Stop(true);
        SafeRelease(events[i].soundEvent);
    }

    events.clear();
}

void SoundComponent::Trigger()
{
    uint32 eventsCount = static_cast<uint32>(events.size());
    for (uint32 i = 0; i < eventsCount; ++i)
        Trigger(i);
}

void SoundComponent::Stop()
{
    uint32 eventsCount = static_cast<uint32>(events.size());
    for (uint32 i = 0; i < eventsCount; ++i)
        Stop(i);
}

void SoundComponent::Trigger(uint32 index)
{
    DVASSERT(index < events.size());

    SoundComponentElement& sound = events[index];
    sound.soundEvent->Trigger();

    if ((sound.flags & SoundComponent::FLAG_AUTO_DISTANCE_TRIGGER) && entity && entity->GetScene())
    {
        entity->GetScene()->soundSystem->AddAutoTriggerSound(entity, sound.soundEvent);
    }
}

void SoundComponent::Stop(uint32 index)
{
    DVASSERT(index < events.size());

    SoundComponentElement& sound = events[index];
    sound.soundEvent->Stop();

    if ((sound.flags & SoundComponent::FLAG_AUTO_DISTANCE_TRIGGER) && entity && entity->GetScene())
    {
        entity->GetScene()->soundSystem->RemoveAutoTriggerSound(entity, sound.soundEvent);
    }
}

void SoundComponent::SetSoundEventFlags(uint32 index, uint32 flags)
{
    DVASSERT(index < static_cast<uint32>(events.size()));

    if (events[index].flags != flags)
    {
        Stop(index);
        events[index].flags = flags;

        GlobalEventSystem::Instance()->Event(this, EventSystem::SOUND_COMPONENT_CHANGED);
    }
}

void SoundComponent::SetLocalDirection(uint32 eventIndex, const Vector3& direction)
{
    DVASSERT(eventIndex < static_cast<uint32>(events.size()));
    events[eventIndex].localDirection = direction;
}

void SoundComponent::SetLocalDirection(const DAVA::Vector3& direction)
{
    uint32 eventsCount = static_cast<uint32>(events.size());
    for (uint32 i = 0; i < eventsCount; ++i)
        SetLocalDirection(i, direction);
}

Component* SoundComponent::Clone(Entity* toEntity)
{
    SoundComponent* soundComponent = new SoundComponent();
    soundComponent->SetEntity(toEntity);

    SoundSystem* soundSystem = GetEngineContext()->soundSystem;
    int32 eventCount = static_cast<int32>(events.size());
    for (int32 i = 0; i < eventCount; ++i)
    {
        SoundEvent* clonedEvent = soundSystem->CloneEvent(events[i].soundEvent);
        soundComponent->AddSoundEvent(clonedEvent, events[i].flags, events[i].localDirection);
        clonedEvent->Release();
    }

    return soundComponent;
}

void SoundComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (archive)
    {
        uint32 eventsCount = static_cast<uint32>(events.size());
        archive->SetUInt32("sc.eventCount", eventsCount);
        for (uint32 i = 0; i < eventsCount; ++i)
        {
            KeyedArchive* eventArchive = new KeyedArchive();

            GetEngineContext()->soundSystem->SerializeEvent(events[i].soundEvent, eventArchive);
            eventArchive->SetUInt32("sce.flags", events[i].flags);
            eventArchive->SetVector3("sce.localDirection", events[i].localDirection);

            archive->SetArchive(KeyedArchive::GenKeyFromIndex(i), eventArchive);
            SafeRelease(eventArchive);
        }
    }
}

void SoundComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    events.clear();

    if (archive)
    {
        uint32 eventsCount = archive->GetUInt32("sc.eventCount");
        for (uint32 i = 0; i < eventsCount; ++i)
        {
            KeyedArchive* eventArchive = archive->GetArchive(KeyedArchive::GenKeyFromIndex(i));
            SoundEvent* sEvent = GetEngineContext()->soundSystem->DeserializeEvent(eventArchive);
            AddSoundEvent(sEvent, eventArchive->GetUInt32("sce.flags"), eventArchive->GetVector3("sce.localDirection", Vector3(1.f, 0.f, 0.f)));
            SafeRelease(sEvent);
        }
    }

    Component::Deserialize(archive, serializationContext);
}
};
