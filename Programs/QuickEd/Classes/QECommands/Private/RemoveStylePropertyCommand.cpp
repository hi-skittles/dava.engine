#include "QECommands/RemoveStylePropertyCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/StyleSheetNode.h"
#include "Model/ControlProperties/StyleSheetProperty.h"
#include "Model/ControlProperties/StyleSheetRootProperty.h"
#include "UI/Components/UIComponent.h"

using namespace DAVA;

RemoveStylePropertyCommand::RemoveStylePropertyCommand(PackageNode* package, StyleSheetNode* node_, StyleSheetProperty* property_)
    : QEPackageCommand(package, "Remove Style Property")
    , node(RefPtr<StyleSheetNode>::ConstructWithRetain(node_))
    , property(RefPtr<StyleSheetProperty>::ConstructWithRetain(property_))
{
}

void RemoveStylePropertyCommand::Redo()
{
    package->RemoveStyleProperty(node.Get(), property.Get());
}

void RemoveStylePropertyCommand::Undo()
{
    package->AddStyleProperty(node.Get(), property.Get());
}
