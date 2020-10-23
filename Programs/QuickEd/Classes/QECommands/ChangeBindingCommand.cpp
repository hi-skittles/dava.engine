#include "ChangeBindingCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/ControlProperties/AbstractProperty.h"

#include <QString>
using namespace DAVA;

ChangeBindingCommand::ChangeBindingCommand(PackageNode* root_, ControlNode* node_, AbstractProperty* prop_, const String& newVal_, int32 newMode_)
    : DAVA::Command(DAVA::String("changed property: ") + prop_->GetName().c_str())
    , root(root_)
    , node(node_)
    , property(prop_)
    , oldBindingValue(GetValueFromProperty(prop_))
    , newBindingValue(newVal_)
    , oldMode(property->GetBindingUpdateMode())
    , newMode(newMode_)
{
    oldValue = property->IsOverriddenLocally() ? property->GetValue() : DAVA::Any();
    wasBound = property->IsBound();
}

void ChangeBindingCommand::Redo()
{
    root->SetControlBindingProperty(node, property, newBindingValue, newMode);
}

void ChangeBindingCommand::Undo()
{
    if (wasBound)
    {
        root->SetControlBindingProperty(node, property, oldBindingValue, oldMode);
    }
    else
    {
        root->ResetControlProperty(node, property);
        if (!oldValue.IsEmpty())
        {
            root->SetControlProperty(node, property, oldValue);
        }
    }
}

String ChangeBindingCommand::GetValueFromProperty(AbstractProperty* property)
{
    return property->GetBindingExpression();
}
