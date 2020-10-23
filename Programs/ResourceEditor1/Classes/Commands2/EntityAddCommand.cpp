#include "Commands2/EntityAddCommand.h"
#include "Commands2/RECommandIDs.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Components/SwitchComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"

#include "Utils/StringFormat.h"

EntityAddCommand::EntityAddCommand(DAVA::Entity* _entityToAdd, DAVA::Entity* toParent)
    : RECommand(CMDID_ENTITY_ADD, DAVA::Format("Add Entity %s", _entityToAdd->GetName().c_str()))
    , entityToAdd(_entityToAdd)
    , parentToAdd(toParent)
{
    SafeRetain(entityToAdd);
}

EntityAddCommand::~EntityAddCommand()
{
    SafeRelease(entityToAdd);
}

void EntityAddCommand::Undo()
{
    if (NULL != parentToAdd && NULL != entityToAdd)
    {
        parentToAdd->RemoveNode(entityToAdd);
    }
}

void EntityAddCommand::Redo()
{
    if (parentToAdd)
    {
        parentToAdd->AddNode(entityToAdd);

        //Workaround for correctly adding of switch
        DAVA::SwitchComponent* sw = GetSwitchComponent(entityToAdd);
        if (sw)
        {
            sw->SetSwitchIndex(sw->GetSwitchIndex());
        }
    }
}

DAVA::Entity* EntityAddCommand::GetEntity() const
{
    return entityToAdd;
}