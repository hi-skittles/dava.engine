#include "QECommands/AddStyleSelectorCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/StyleSheetNode.h"
#include "Model/ControlProperties/StyleSheetSelectorProperty.h"
#include "Model/ControlProperties/StyleSheetRootProperty.h"
#include "Model/ControlProperties/SectionProperty.h"
#include "UI/Components/UIComponent.h"

using namespace DAVA;

AddStyleSelectorCommand::AddStyleSelectorCommand(PackageNode* package, StyleSheetNode* node_, StyleSheetSelectorProperty* property_)
    : QEPackageCommand(package, "Add Style Selector")
    , node(RefPtr<StyleSheetNode>::ConstructWithRetain(node_))
    , property(RefPtr<StyleSheetSelectorProperty>::ConstructWithRetain(property_))
    , index(-1)
{
    index = node->GetRootProperty()->GetSelectors()->GetCount();
    DVASSERT(index != -1);
}

void AddStyleSelectorCommand::Redo()
{
    if (index != -1)
    {
        package->InsertSelector(node.Get(), property.Get(), index);
    }
}

void AddStyleSelectorCommand::Undo()
{
    if (index != -1)
    {
        package->RemoveSelector(node.Get(), property.Get());
    }
}
