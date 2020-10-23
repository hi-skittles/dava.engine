#include "MaterialSwitchParentCommand.h"
#include "Commands2/RECommandIDs.h"

MaterialSwitchParentCommand::MaterialSwitchParentCommand(DAVA::NMaterial* instance, DAVA::NMaterial* _newParent)
    : RECommand(CMDID_MATERIAL_SWITCH_PARENT, "Switch Material Parent")
{
    DVASSERT(instance);
    DVASSERT(_newParent);
    DVASSERT(instance->GetParent());

    currentInstance = DAVA::SafeRetain(instance);
    newParent = DAVA::SafeRetain(_newParent);
    oldParent = DAVA::SafeRetain(instance->GetParent());
}

MaterialSwitchParentCommand::~MaterialSwitchParentCommand()
{
    DAVA::SafeRelease(oldParent);
    DAVA::SafeRelease(newParent);
    DAVA::SafeRelease(currentInstance);
}

void MaterialSwitchParentCommand::Redo()
{
    currentInstance->SetParent(newParent);
}

void MaterialSwitchParentCommand::Undo()
{
    currentInstance->SetParent(oldParent);
}
