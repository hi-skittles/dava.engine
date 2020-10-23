#include "QECommands/AddStylePropertyCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/StyleSheetNode.h"
#include "Model/ControlProperties/StyleSheetProperty.h"
#include "Model/ControlProperties/StyleSheetRootProperty.h"
#include "UI/Components/UIComponent.h"

using namespace DAVA;

AddStylePropertyCommand::AddStylePropertyCommand(PackageNode* package, StyleSheetNode* node_, StyleSheetProperty* property_)
    : QEPackageCommand(package, "Add Style Property")
    , node(RefPtr<StyleSheetNode>::ConstructWithRetain(node_))
    , property(RefPtr<StyleSheetProperty>::ConstructWithRetain(property_))
{
}

void AddStylePropertyCommand::Redo()
{
    package->AddStyleProperty(node.Get(), property.Get());
}

void AddStylePropertyCommand::Undo()
{
    package->RemoveStyleProperty(node.Get(), property.Get());
}
