#include "QECommands/InsertImportedPackageCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/PackageControlsNode.h"

using namespace DAVA;

InsertImportedPackageCommand::InsertImportedPackageCommand(PackageNode* package, PackageNode* importedPackage_, int index_)
    : QEPackageCommand(package, "Insert Imported Package")
    , importedPackage(RefPtr<PackageNode>::ConstructWithRetain(importedPackage_))
    , index(index_)
{
}

void InsertImportedPackageCommand::Redo()
{
    package->InsertImportedPackage(importedPackage.Get(), index);
}

void InsertImportedPackageCommand::Undo()
{
    package->RemoveImportedPackage(importedPackage.Get());
}
