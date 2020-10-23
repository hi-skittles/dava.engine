#include "QECommands/ResizeCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/ControlProperties/AbstractProperty.h"

#include <Utils/StringFormat.h>

namespace ResizeCommandDetails
{
DAVA::Any GetValueFromProperty(AbstractProperty* property)
{
    return property->IsOverriddenLocally() ? property->GetValue() : DAVA::Any();
}
}

ResizeCommand::ResizeCommand(PackageNode* package)
    : QEPackageCommand(package, DAVA::Format("Resize"))
{
}

void ResizeCommand::AddNodePropertyValue(ControlNode* node, AbstractProperty* sizeProperty, const DAVA::Any& sizeValue, AbstractProperty* pivotProperty, const DAVA::Any& pivotValue)
{
    DVASSERT(node != nullptr);
    DVASSERT(sizeProperty != nullptr);
    DVASSERT(pivotProperty != nullptr);
    items.emplace_back(node, sizeProperty, sizeValue, pivotProperty, pivotValue);
}

void ResizeCommand::Redo()
{
    for (const Item& item : items)
    {
        package->SetControlProperty(item.node.Get(), item.sizeProperty.Get(), item.sizeNewValue);
        package->SetControlProperty(item.node.Get(), item.pivotProperty.Get(), item.pivotNewValue);
    }
}

void ResizeCommand::Undo()
{
    for (const Item& item : items)
    {
        if (item.sizeOldValue.IsEmpty())
        {
            package->ResetControlProperty(item.node.Get(), item.sizeProperty.Get());
        }
        else
        {
            package->SetControlProperty(item.node.Get(), item.sizeProperty.Get(), item.sizeOldValue);
        }

        if (item.pivotOldValue.IsEmpty())
        {
            package->ResetControlProperty(item.node.Get(), item.pivotProperty.Get());
        }
        else
        {
            package->SetControlProperty(item.node.Get(), item.pivotProperty.Get(), item.pivotOldValue);
        }
    }
}

bool ResizeCommand::MergeWith(const DAVA::Command* command)
{
    const ResizeCommand* other = DAVA::DynamicTypeCheck<const ResizeCommand*>(command);
    DVASSERT(other != nullptr);
    if (package != other->package)
    {
        return false;
    }
    const size_t itemsSize = items.size();
    if (itemsSize != other->items.size())
    {
        return false;
    }
    for (size_t i = 0; i < itemsSize; ++i)
    {
        const Item& item = items.at(i);
        const Item& otherItem = other->items.at(i);
        if (item.node != otherItem.node ||
            item.sizeProperty != otherItem.sizeProperty ||
            item.pivotProperty != otherItem.pivotProperty)
        {
            return false;
        }
    }
    for (size_t i = 0; i < itemsSize; ++i)
    {
        items.at(i).sizeNewValue = other->items.at(i).sizeNewValue;
        items.at(i).pivotNewValue = other->items.at(i).pivotNewValue;
    }
    return true;
}

ResizeCommand::Item::Item(ControlNode* node_, AbstractProperty* sizeProperty_, const DAVA::Any& sizeValue, AbstractProperty* pivotProperty_, const DAVA::Any& pivotValue)
    : node(DAVA::RefPtr<ControlNode>::ConstructWithRetain(node_))
    , sizeProperty(DAVA::RefPtr<AbstractProperty>::ConstructWithRetain(sizeProperty_))
    , sizeNewValue(sizeValue)
    , sizeOldValue(ResizeCommandDetails::GetValueFromProperty(sizeProperty_))
    , pivotProperty(DAVA::RefPtr<AbstractProperty>::ConstructWithRetain(pivotProperty_))
    , pivotNewValue(pivotValue)
    , pivotOldValue(ResizeCommandDetails::GetValueFromProperty(pivotProperty_))
{
}
