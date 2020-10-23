#pragma once

#include "Classes/Selection/SelectableGroup.h"
#include "Classes/Selection/SelectionSystem.h"
#include "TArc/DataProcessing/DataNode.h"
#include "Reflection/ReflectionRegistrator.h"

#include <memory>

namespace DAVA
{
class Entity;
}

class SelectionData : public DAVA::TArc::DataNode
{
public:
    static const char* selectionPropertyName;
    static const char* selectionBoxPropertyName;
    static const char* selectionAllowedPropertyName;

    const SelectableGroup& GetSelection() const;
    SelectableGroup GetMutableSelection() const;
    void SetSelection(SelectableGroup& newSelection);

    const DAVA::AABBox3& GetSelectionBox() const;

    void CancelSelection();

    //Support old selectionSystem interface
    void ResetSelectionComponentMask();
    void SetSelectionComponentMask(const DAVA::ComponentMask& mask);
    const DAVA::ComponentMask& GetSelectionComponentMask() const;

    void SetSelectionAllowed(bool allowed);
    bool IsSelectionAllowed() const;

    bool Lock();
    bool IsLocked() const;
    void Unlock();

    bool IsEntitySelectable(DAVA::Entity* selectionCandidate) const;
    //end of old interface

private:
    friend class SelectionModule;

    std::unique_ptr<SelectionSystem> selectionSystem;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SelectionData, DAVA::TArc::DataNode)
    {
        DAVA::ReflectionRegistrator<SelectionData>::Begin()
        .Field(selectionPropertyName, &SelectionData::GetSelection, nullptr)
        .Field(selectionBoxPropertyName, &SelectionData::GetSelectionBox, nullptr)
        .Field(selectionAllowedPropertyName, &SelectionData::IsSelectionAllowed, nullptr)
        .End();
    }
};

namespace DAVA
{
template <>
bool AnyCompare<SelectableGroup>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2);
extern template struct AnyCompare<SelectableGroup>;
}
