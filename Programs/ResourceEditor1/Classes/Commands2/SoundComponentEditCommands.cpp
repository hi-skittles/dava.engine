#include "Commands2/SoundComponentEditCommands.h"
#include "Commands2/RECommandIDs.h"

AddSoundEventCommand::AddSoundEventCommand(DAVA::Entity* _entity, DAVA::SoundEvent* _event)
    : RECommand(CMDID_SOUND_ADD_EVENT, "Add Sound Event")
{
    DVASSERT(_entity);
    DVASSERT(_event);

    savedEvent = SafeRetain(_event);
    entity = SafeRetain(_entity);
}

AddSoundEventCommand::~AddSoundEventCommand()
{
    SafeRelease(savedEvent);
    SafeRelease(entity);
}

void AddSoundEventCommand::Redo()
{
    DAVA::SoundComponent* component = GetSoundComponent(entity);
    DVASSERT(component);
    component->AddSoundEvent(savedEvent);
}

void AddSoundEventCommand::Undo()
{
    savedEvent->Stop();
    DAVA::SoundComponent* component = GetSoundComponent(entity);
    DVASSERT(component);
    component->RemoveSoundEvent(savedEvent);
}

DAVA::Entity* AddSoundEventCommand::GetEntity() const
{
    return entity;
}

RemoveSoundEventCommand::RemoveSoundEventCommand(DAVA::Entity* _entity, DAVA::SoundEvent* _event)
    : RECommand(CMDID_SOUND_REMOVE_EVENT, "Remove Sound Event")
{
    DVASSERT(_entity);
    DVASSERT(_event);

    savedEvent = SafeRetain(_event);
    entity = SafeRetain(_entity);
}

RemoveSoundEventCommand::~RemoveSoundEventCommand()
{
    SafeRelease(savedEvent);
    SafeRelease(entity);
}

void RemoveSoundEventCommand::Redo()
{
    savedEvent->Stop();
    DAVA::SoundComponent* component = GetSoundComponent(entity);
    DVASSERT(component);
    component->RemoveSoundEvent(savedEvent);
}

void RemoveSoundEventCommand::Undo()
{
    DAVA::SoundComponent* component = GetSoundComponent(entity);
    DVASSERT(component);
    component->AddSoundEvent(savedEvent);
}

DAVA::Entity* RemoveSoundEventCommand::GetEntity() const
{
    return entity;
}

SetSoundEventFlagsCommand::SetSoundEventFlagsCommand(DAVA::Entity* _entity, DAVA::uint32 eventIndex, DAVA::uint32 flags)
    : RECommand(CMDID_SOUND_REMOVE_EVENT, "Set Sound Event Flags")
    ,
    index(eventIndex)
    ,
    newFlags(flags)
{
    entity = SafeRetain(_entity);
    DVASSERT(entity);

    affectComponent = GetSoundComponent(entity);
    DVASSERT(affectComponent);

    oldFlags = affectComponent->GetSoundEventFlags(index);
}

SetSoundEventFlagsCommand::~SetSoundEventFlagsCommand()
{
    SafeRelease(entity);
}

void SetSoundEventFlagsCommand::Redo()
{
    affectComponent->SetSoundEventFlags(index, newFlags);
}

void SetSoundEventFlagsCommand::Undo()
{
    affectComponent->SetSoundEventFlags(index, oldFlags);
}

DAVA::Entity* SetSoundEventFlagsCommand::GetEntity() const
{
    return entity;
}