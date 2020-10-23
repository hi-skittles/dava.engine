#include "Classes/Commands2/SlotCommands.h"
#include "Classes/Commands2/RECommandIDs.h"
#include "Classes/Qt/Scene/SceneEditor2.h"
#include "Classes/SlotSupportModule/Private/EditorSlotSystem.h"

#include <Scene3D/Entity.h>
#include <Scene3D/Systems/SlotSystem.h>
#include <Scene3D/Components/SlotComponent.h>

AttachEntityToSlot::AttachEntityToSlot(SceneEditor2* sceneEditor_, DAVA::SlotComponent* slotComponent, DAVA::Entity* entity, DAVA::FastName itemName)
    : RECommand(CMDID_ATTACH_TO_SLOT, "Add item to slot")
    , sceneEditor(sceneEditor_)
    , component(slotComponent)
    , redoEntity(DAVA::RefPtr<DAVA::Entity>::ConstructWithRetain(entity))
    , redoItemName(itemName)
    , redoEntityInited(true)
{
}

AttachEntityToSlot::AttachEntityToSlot(SceneEditor2* sceneEditor_, DAVA::SlotComponent* slotComponent, DAVA::FastName itemName)
    : RECommand(CMDID_ATTACH_TO_SLOT, "Add item to slot")
    , sceneEditor(sceneEditor_)
    , component(slotComponent)
    , redoItemName(itemName)
    , redoEntityInited(false)
{
    DVASSERT(redoItemName.IsValid());
}

void AttachEntityToSlot::Redo()
{
    EditorSlotSystem* system = sceneEditor->LookupEditorSystem<EditorSlotSystem>();
    DVASSERT(system != nullptr);

    if (executed == false)
    {
        undoEntity = DAVA::RefPtr<DAVA::Entity>::ConstructWithRetain(sceneEditor->slotSystem->LookUpLoadedEntity(component));
        undoItemName = component->GetLoadedItemName();
        executed = true;
    }

    if (undoEntity.Get() != nullptr)
    {
        system->DetachEntity(component, undoEntity.Get());
    }

    if (redoEntityInited == false)
    {
        redoEntity = system->AttachEntity(component, redoItemName);
        if (redoEntity.Get() == nullptr)
        {
            DAVA::Logger::Error("Couldn't load item %s to slot %s", redoItemName.c_str(), component->GetSlotName().c_str());
            redoEntity.ConstructInplace();
            redoItemName = EditorSlotSystem::emptyItemName;
        }
        redoEntityInited = true;
    }
    else if (redoEntity.Get() != nullptr)
    {
        system->AttachEntity(component, redoEntity.Get(), redoItemName);
    }
}

void AttachEntityToSlot::Undo()
{
    EditorSlotSystem* system = sceneEditor->LookupEditorSystem<EditorSlotSystem>();
    DVASSERT(system != nullptr);
    if (redoEntity.Get() != nullptr)
    {
        system->DetachEntity(component, redoEntity.Get());
    }

    if (undoEntity.Get() != nullptr)
    {
        system->AttachEntity(component, undoEntity.Get(), undoItemName);
    }
}

bool AttachEntityToSlot::IsClean() const
{
    return true;
}

SlotTypeFilterEdit::SlotTypeFilterEdit(DAVA::SlotComponent* slotComponent_, DAVA::FastName typeFilter_, bool isAddCommand_)
    : RECommand(CMDID_EDIT_SLOT_FILTERS, "Edit slot filters")
    , slotComponent(slotComponent_)
    , typeFilter(typeFilter_)
    , isAddCommand(isAddCommand_)
{
    if (isAddCommand == false)
    {
        bool filterFound = false;
        for (DAVA::uint32 i = 0; i < slotComponent->GetTypeFiltersCount(); ++i)
        {
            if (slotComponent->GetTypeFilter(i) == typeFilter)
            {
                filterFound = true;
                break;
            }
        }

        DVASSERT(filterFound == true);
    }
}

void SlotTypeFilterEdit::Redo()
{
    if (isAddCommand == true)
    {
        slotComponent->AddTypeFilter(typeFilter);
    }
    else
    {
        slotComponent->RemoveTypeFilter(typeFilter);
    }
}

void SlotTypeFilterEdit::Undo()
{
    if (isAddCommand == false)
    {
        slotComponent->AddTypeFilter(typeFilter);
    }
    else
    {
        slotComponent->RemoveTypeFilter(typeFilter);
    }
}
