#include "QECommands/InsertStyleCommand.h"

#include "Model/PackageHierarchy/StyleSheetNode.h"
#include "Model/PackageHierarchy/StyleSheetsNode.h"
#include "Model/PackageHierarchy/PackageNode.h"

using namespace DAVA;

InsertStyleCommand::InsertStyleCommand(PackageNode* package, StyleSheetNode* node_, StyleSheetsNode* dest_, int index_)
    : QEPackageCommand(package, "Insert Style")
    , node(RefPtr<StyleSheetNode>::ConstructWithRetain(node_))
    , dest(RefPtr<StyleSheetsNode>::ConstructWithRetain(dest_))
    , index(index_)
{
}

void InsertStyleCommand::Redo()
{
    package->InsertStyle(node.Get(), dest.Get(), index);
}

void InsertStyleCommand::Undo()
{
    package->RemoveStyle(node.Get(), dest.Get());
}
