#include "QECommands/RemoveStyleSelectorCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/StyleSheetNode.h"
#include "Model/ControlProperties/StyleSheetSelectorProperty.h"
#include "Model/ControlProperties/StyleSheetRootProperty.h"
#include "Model/ControlProperties/SectionProperty.h"
#include "UI/Components/UIComponent.h"

using namespace DAVA;

RemoveStyleSelectorCommand::RemoveStyleSelectorCommand(PackageNode* package, StyleSheetNode* node_, StyleSheetSelectorProperty* property_)
    : QEPackageCommand(package, "Remove Style Selector")
    , node(RefPtr<StyleSheetNode>::ConstructWithRetain(node_))
    , property(RefPtr<StyleSheetSelectorProperty>::ConstructWithRetain(property_))
    , index(-1)
{
    index = node->GetRootProperty()->GetSelectors()->GetIndex(property.Get());

    DVASSERT(index != -1);
}

void RemoveStyleSelectorCommand::Redo()
{
    if (index != -1)
    {
        package->RemoveSelector(node.Get(), property.Get());
    }
}

void RemoveStyleSelectorCommand::Undo()
{
    if (index != -1)
    {
        package->InsertSelector(node.Get(), property.Get(), index);
    }
}
