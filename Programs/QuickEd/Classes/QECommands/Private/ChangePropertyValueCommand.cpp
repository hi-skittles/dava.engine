#include "QECommands/ChangePropertyValueCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/ControlProperties/AbstractProperty.h"

#include <Utils/StringFormat.h>

namespace ChangePropertyValueCommandDetails
{
DAVA::Any GetValueFromProperty(AbstractProperty* property)
{
    return property->IsOverriddenLocally() ? property->GetValue() : DAVA::Any();
}
}

ChangePropertyValueCommand::ChangePropertyValueCommand(PackageNode* package)
    : QEPackageCommand(package, "Change property")
{
}

ChangePropertyValueCommand::ChangePropertyValueCommand(PackageNode* package, ControlNode* node, AbstractProperty* property, const DAVA::Any& newValue)
    : QEPackageCommand(package, DAVA::Format("Change property: %s", property->GetName().c_str()))
{
    AddNodePropertyValue(node, property, newValue);
}

void ChangePropertyValueCommand::AddNodePropertyValue(ControlNode* node, AbstractProperty* property, const DAVA::Any& newValue)
{
    DVASSERT(node != nullptr);
    DVASSERT(property != nullptr);
    items.emplace_back(node, property, newValue);
}

void ChangePropertyValueCommand::Redo()
{
    for (const Item& item : items)
    {
        ApplyProperty(item.node.Get(), item.property.Get(), item.newValue);
    }
}

void ChangePropertyValueCommand::Undo()
{
    for (const Item& item : items)
    {
        if (item.wasBound)
        {
            package->SetControlBindingProperty(item.node.Get(), item.property.Get(), item.oldBindingValue, item.oldBindingMode);
        }
        else
        {
            ApplyProperty(item.node.Get(), item.property.Get(), item.oldValue);
        }
    }
}

void ChangePropertyValueCommand::ApplyProperty(ControlNode* node, AbstractProperty* property, const DAVA::Any& value)
{
    if (value.IsEmpty())
    {
        package->ResetControlProperty(node, property);
    }
    else
    {
        package->SetControlProperty(node, property, value);
    }
}

bool ChangePropertyValueCommand::MergeWith(const DAVA::Command* command)
{
    const ChangePropertyValueCommand* other = DAVA::DynamicTypeCheck<const ChangePropertyValueCommand*>(command);
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
            item.property != otherItem.property)
        {
            return false;
        }
    }
    for (size_t i = 0; i < itemsSize; ++i)
    {
        items.at(i).newValue = other->items.at(i).newValue;
    }
    return true;
}

ChangePropertyValueCommand::Item::Item(ControlNode* node_, AbstractProperty* property_, const DAVA::Any& newValue_)
    : node(DAVA::RefPtr<ControlNode>::ConstructWithRetain(node_))
    , property(DAVA::RefPtr<AbstractProperty>::ConstructWithRetain(property_))
    , newValue(newValue_)
    , oldValue(ChangePropertyValueCommandDetails::GetValueFromProperty(property_))
    , oldBindingValue(property_->GetBindingExpression())
    , oldBindingMode(property_->GetBindingUpdateMode())
    , wasBound(property_->IsBound())
{
}
