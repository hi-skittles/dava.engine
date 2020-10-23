#include "QECommands/InsertControlCommand.h"

#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/PackageHierarchy/PackageControlsNode.h"
#include "Model/PackageHierarchy/PackageNode.h"

InsertControlCommand::InsertControlCommand(PackageNode* package, ControlNode* node_, ControlsContainerNode* dest_, int index_)
    : QEPackageCommand(package, "Insert Control")
    , node(DAVA::RefPtr<ControlNode>::ConstructWithRetain(node_))
    , dest(DAVA::RefPtr<ControlsContainerNode>::ConstructWithRetain(dest_))
    , index(index_)
{
}

void InsertControlCommand::Redo()
{
    package->InsertControl(node.Get(), dest.Get(), index);
}

void InsertControlCommand::Undo()
{
    package->RemoveControl(node.Get(), dest.Get());
}
