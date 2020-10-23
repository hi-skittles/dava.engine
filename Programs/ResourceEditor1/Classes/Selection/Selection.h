#pragma once

#include "Classes/Selection/Selectable.h"

#include "Base/BaseTypes.h"

namespace DAVA
{
class Entity;
}

class SelectableGroup;

namespace Selection
{
const SelectableGroup& GetSelection();
void SetSelection(SelectableGroup& newSelection);

void CancelSelection();

Selectable GetSelectableObject(const Selectable& object);
DAVA::Entity* GetSelectableEntity(DAVA::Entity* selectionCandidate);
bool IsEntitySelectable(DAVA::Entity* selectionCandidate);

void ResetSelectionComponentMask();
void SetSelectionComponentMask(const DAVA::ComponentMask& mask);
DAVA::ComponentMask GetSelectionComponentMask();

bool Lock();
void Unlock();
}
