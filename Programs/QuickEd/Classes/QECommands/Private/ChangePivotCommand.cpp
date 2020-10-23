#include "QECommands/ChangePivotCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/ControlProperties/AbstractProperty.h"

#include <Utils/StringFormat.h>

namespace ChangePivotCommandDetails
{
DAVA::Any GetValueFromProperty(AbstractProperty* property)
{
    return property->IsOverriddenLocally() ? property->GetValue() : DAVA::Any();
}
}

ChangePivotCommand::ChangePivotCommand(PackageNode* package)
    : QEPackageCommand(package, DAVA::Format("Change pivot"))
{
}

void ChangePivotCommand::AddNodePropertyValue(ControlNode* node, AbstractProperty* pivotProperty, const DAVA::Any& pivotValue, AbstractProperty* positionProperty, const DAVA::Any& positionValue)
{
    DVASSERT(node != nullptr);
    DVASSERT(pivotProperty != nullptr);
    DVASSERT(positionProperty != nullptr);
    items.emplace_back(node, pivotProperty, pivotValue, positionProperty, positionValue);
}

void ChangePivotCommand::Redo()
{
    for (const Item& item : items)
    {
        package->SetControlProperty(item.node.Get(), item.pivotProperty.Get(), item.pivotNewValue);
        package->SetControlProperty(item.node.Get(), item.positionProperty.Get(), item.positionNewValue);
    }
}

void ChangePivotCommand::Undo()
{
    for (const Item& item : items)
    {
        if (item.pivotOldValue.IsEmpty())
        {
            package->ResetControlProperty(item.node.Get(), item.pivotProperty.Get());
        }
        else
        {
            package->SetControlProperty(item.node.Get(), item.pivotProperty.Get(), item.pivotOldValue);
        }

        if (item.positionOldValue.IsEmpty())
        {
            package->ResetControlProperty(item.node.Get(), item.positionProperty.Get());
        }
        else
        {
            package->SetControlProperty(item.node.Get(), item.positionProperty.Get(), item.positionOldValue);
        }
    }
}

bool ChangePivotCommand::MergeWith(const DAVA::Command* command)
{
    const ChangePivotCommand* other = DAVA::DynamicTypeCheck<const ChangePivotCommand*>(command);
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
            item.pivotProperty != otherItem.pivotProperty ||
            item.positionProperty != otherItem.positionProperty)
        {
            return false;
        }
    }
    for (size_t i = 0; i < itemsSize; ++i)
    {
        items.at(i).pivotNewValue = other->items.at(i).pivotNewValue;
        items.at(i).positionNewValue = other->items.at(i).positionNewValue;
    }
    return true;
}

ChangePivotCommand::Item::Item(ControlNode* node_, AbstractProperty* pivotProperty_, const DAVA::Any& pivotValue, AbstractProperty* positionProperty_, const DAVA::Any& positionValue)
    : node(DAVA::RefPtr<ControlNode>::ConstructWithRetain(node_))
    , pivotProperty(DAVA::RefPtr<AbstractProperty>::ConstructWithRetain(pivotProperty_))
    , pivotNewValue(pivotValue)
    , pivotOldValue(ChangePivotCommandDetails::GetValueFromProperty(pivotProperty_))
    , positionProperty(DAVA::RefPtr<AbstractProperty>::ConstructWithRetain(positionProperty_))
    , positionNewValue(positionValue)
    , positionOldValue(ChangePivotCommandDetails::GetValueFromProperty(positionProperty_))
{
}