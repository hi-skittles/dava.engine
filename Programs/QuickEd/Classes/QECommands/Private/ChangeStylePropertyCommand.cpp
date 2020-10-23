#include "QECommands/ChangeStylePropertyCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/StyleSheetNode.h"
#include "Model/ControlProperties/AbstractProperty.h"

ChangeStylePropertyCommand::ChangeStylePropertyCommand(PackageNode* package, StyleSheetNode* node_, AbstractProperty* property_, const DAVA::Any& newValue_)
    : QEPackageCommand(package, DAVA::String("change ") + property_->GetName().c_str())
    , node(DAVA::RefPtr<StyleSheetNode>::ConstructWithRetain(node_))
    , property(DAVA::RefPtr<AbstractProperty>::ConstructWithRetain(property_))
    , newValue(newValue_)
{
    oldValue = property->GetValue();
}

void ChangeStylePropertyCommand::Redo()
{
    package->SetStyleProperty(node.Get(), property.Get(), newValue);
}

void ChangeStylePropertyCommand::Undo()
{
    package->SetStyleProperty(node.Get(), property.Get(), oldValue);
}
