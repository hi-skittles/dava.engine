#include "QECommands/ChangePropertyForceOverrideCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/ControlProperties/ValueProperty.h"

#include <Utils/StringFormat.h>

ChangePropertyForceOverrideCommand::ChangePropertyForceOverrideCommand(PackageNode* package, ControlNode* node_, ValueProperty* property_)
    : QEPackageCommand(package, DAVA::Format("Change Property Force Override: %s", property_->GetName().c_str()))
{
    node = DAVA::RefPtr<ControlNode>::ConstructWithRetain(node_);
    property = DAVA::RefPtr<ValueProperty>::ConstructWithRetain(property_);
    prevForceOverriden = property->IsForceOverride();
}

void ChangePropertyForceOverrideCommand::Redo()
{
    package->SetControlPropertyForceOverride(node.Get(), property.Get(), true);
}

void ChangePropertyForceOverrideCommand::Undo()
{
    package->SetControlPropertyForceOverride(node.Get(), property.Get(), prevForceOverriden);
}
