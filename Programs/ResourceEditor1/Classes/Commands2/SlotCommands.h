#pragma once

#include "Classes/Commands2/Base/RECommand.h"

#include <Base/FastName.h>
#include <Base/RefPtr.h>

class SceneEditor2;
namespace DAVA
{
class Entity;
class SlotComponent;
}

class AttachEntityToSlot : public RECommand
{
public:
    explicit AttachEntityToSlot(SceneEditor2* sceneEditor, DAVA::SlotComponent* slotComponent, DAVA::Entity* entity, DAVA::FastName itemName);
    explicit AttachEntityToSlot(SceneEditor2* sceneEditor, DAVA::SlotComponent* slotComponent, DAVA::FastName itemName);

    void Redo() override;
    void Undo() override;

    bool IsClean() const override;

private:
    SceneEditor2* sceneEditor = nullptr;
    DAVA::SlotComponent* component = nullptr;
    DAVA::RefPtr<DAVA::Entity> redoEntity;
    DAVA::RefPtr<DAVA::Entity> undoEntity;
    DAVA::FastName undoItemName;
    DAVA::FastName redoItemName;
    bool redoEntityInited = false;
    bool executed = false;
};

class SlotTypeFilterEdit : public RECommand
{
public:
    explicit SlotTypeFilterEdit(DAVA::SlotComponent* slotComponent, DAVA::FastName typeFilter, bool isAddCommand);

    void Redo() override;
    void Undo() override;

private:
    DAVA::SlotComponent* slotComponent = nullptr;
    DAVA::FastName typeFilter;
    bool isAddCommand = true;
};
