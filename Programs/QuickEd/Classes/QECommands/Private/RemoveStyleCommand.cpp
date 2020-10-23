#include "QECommands/RemoveStyleCommand.h"

#include "Model/PackageHierarchy/StyleSheetNode.h"
#include "Model/PackageHierarchy/StyleSheetsNode.h"
#include "Model/PackageHierarchy/PackageNode.h"

using namespace DAVA;

RemoveStyleCommand::RemoveStyleCommand(PackageNode* package, StyleSheetNode* node_, StyleSheetsNode* dest_, int index_)
    : QEPackageCommand(package, "Remove Style")
    , node(RefPtr<StyleSheetNode>::ConstructWithRetain(node_))
    , dest(RefPtr<StyleSheetsNode>::ConstructWithRetain(dest_))
    , index(index_)
{
}

void RemoveStyleCommand::Redo()
{
    package->RemoveStyle(node.Get(), dest.Get());
}

void RemoveStyleCommand::Undo()
{
    package->InsertStyle(node.Get(), dest.Get(), index);
}
