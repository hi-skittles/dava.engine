#include "Classes/Selection/Selection.h"
#include "Classes/Selection/SelectionData.h"
#include "Classes/Application/REGlobal.h"

#include <TArc/DataProcessing/DataContext.h>

#include <Scene3D/Components/ParticleEffectComponent.h>
#include <Scene3D/Entity.h>
#include <Particles/ParticleEmitterInstance.h>

namespace Selection
{
const SelectableGroup& GetSelection()
{
    SelectionData* selectionData = REGlobal::GetActiveDataNode<SelectionData>();
    if (selectionData != nullptr)
    {
        return selectionData->GetSelection();
    }

    static SelectableGroup emptyGroup;
    return emptyGroup;
}

void CancelSelection()
{
    SelectionData* selectionData = REGlobal::GetActiveDataNode<SelectionData>();
    if (selectionData != nullptr)
    {
        selectionData->CancelSelection();
    }
}

Selectable GetSelectableObject(const Selectable& object)
{
    if (object.CanBeCastedTo<DAVA::Entity>())
    {
        DAVA::Entity* obj = object.Cast<DAVA::Entity>();
        obj = GetSelectableEntity(obj);
        return Selectable(DAVA::Any(obj));
    }
    else if (object.CanBeCastedTo<DAVA::ParticleEmitterInstance>())
    {
        DAVA::ParticleEmitterInstance* instance = object.Cast<DAVA::ParticleEmitterInstance>();
        DAVA::ParticleEffectComponent* component = instance->GetOwner();
        if (component == nullptr)
        {
            return Selectable();
        }

        Selectable result = object;
        DAVA::Entity* entity = component->GetEntity();
        while (entity != nullptr)
        {
            if (entity->GetSolid() == true)
            {
                result = Selectable(DAVA::Any(entity));
                break;
            }
            entity = entity->GetParent();
        }

        return result;
    }

    DVASSERT(false);
    return Selectable();
}

void SetSelection(SelectableGroup& newSelection)
{
    SelectionData* selectionData = REGlobal::GetActiveDataNode<SelectionData>();
    if (selectionData != nullptr)
    {
        selectionData->SetSelection(newSelection);
    }
}

DAVA::Entity* GetSelectableEntity(DAVA::Entity* selectionCandidate)
{
    DAVA::Entity* parent = selectionCandidate;
    while (nullptr != parent)
    {
        if (parent->GetSolid())
        {
            selectionCandidate = parent;
        }
        parent = parent->GetParent();
    }
    return selectionCandidate;
}

bool IsEntitySelectable(DAVA::Entity* selectionCandidate)
{
    SelectionData* selectionData = REGlobal::GetActiveDataNode<SelectionData>();
    if (selectionData != nullptr)
    {
        return selectionData->IsEntitySelectable(selectionCandidate);
    }

    return false;
}

void ResetSelectionComponentMask()
{
    SelectionData* selectionData = REGlobal::GetActiveDataNode<SelectionData>();
    if (selectionData != nullptr)
    {
        selectionData->ResetSelectionComponentMask();
    }
}

void SetSelectionComponentMask(const DAVA::ComponentMask& mask)
{
    SelectionData* selectionData = REGlobal::GetActiveDataNode<SelectionData>();
    if (selectionData != nullptr)
    {
        selectionData->SetSelectionComponentMask(mask);
    }
}

DAVA::ComponentMask GetSelectionComponentMask()
{
    SelectionData* selectionData = REGlobal::GetActiveDataNode<SelectionData>();
    if (selectionData != nullptr)
    {
        return selectionData->GetSelectionComponentMask();
    }

    return DAVA::ComponentMask();
}

bool Lock()
{
    SelectionData* selectionData = REGlobal::GetActiveDataNode<SelectionData>();
    if (selectionData != nullptr)
    {
        return selectionData->Lock();
    }
    return false;
}

void Unlock()
{
    SelectionData* selectionData = REGlobal::GetActiveDataNode<SelectionData>();
    if (selectionData != nullptr)
    {
        selectionData->Unlock();
    }
}
}
