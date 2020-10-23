#ifndef __SOUND_COMPONENT_COMMANDS_H__
#define __SOUND_COMPONENT_COMMANDS_H__

#include "DAVAEngine.h"
#include "Commands2/Base/RECommand.h"

class AddSoundEventCommand : public RECommand
{
public:
    AddSoundEventCommand(DAVA::Entity* entity, DAVA::SoundEvent* sEvent);
    ~AddSoundEventCommand();

    void Undo() override;
    void Redo() override;

    DAVA::Entity* GetEntity() const;

private:
    DAVA::Entity* entity;
    DAVA::SoundEvent* savedEvent;
};

class RemoveSoundEventCommand : public RECommand
{
public:
    RemoveSoundEventCommand(DAVA::Entity* entity, DAVA::SoundEvent* sEvent);
    ~RemoveSoundEventCommand();

    void Undo() override;
    void Redo() override;

    DAVA::Entity* GetEntity() const;

private:
    DAVA::Entity* entity;
    DAVA::SoundEvent* savedEvent;
};

class SetSoundEventFlagsCommand : public RECommand
{
public:
    SetSoundEventFlagsCommand(DAVA::Entity* entity, DAVA::uint32 eventIndex, DAVA::uint32 flags);
    ~SetSoundEventFlagsCommand();

    void Undo() override;
    void Redo() override;

    DAVA::Entity* GetEntity() const;

private:
    DAVA::Entity* entity;
    DAVA::SoundComponent* affectComponent;

    DAVA::uint32 index;
    DAVA::uint32 oldFlags;
    DAVA::uint32 newFlags;
};
#endif // __SOUND_COMPONENT_COMMANDS_H__
