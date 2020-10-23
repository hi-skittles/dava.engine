#include "Commands2/RemoveComponentCommand.h"
#include "Commands2/RECommandIDs.h"
#include "DAVAEngine.h"

RemoveComponentCommand::RemoveComponentCommand(DAVA::Entity* _entity, DAVA::Component* _component)
    : RECommand(CMDID_COMPONENT_REMOVE, "Remove Component")
    , entity(_entity)
    , component(_component)
{
    DVASSERT(entity);
    DVASSERT(component);
}

RemoveComponentCommand::~RemoveComponentCommand()
{
    DAVA::SafeDelete(backup);
}

void RemoveComponentCommand::Redo()
{
    backup = component;
    entity->DetachComponent(component);
}

void RemoveComponentCommand::Undo()
{
    entity->AddComponent(backup);
    backup = nullptr;
}

DAVA::Entity* RemoveComponentCommand::GetEntity() const
{
    return entity;
}

const DAVA::Component* RemoveComponentCommand::GetComponent() const
{
    return component;
}