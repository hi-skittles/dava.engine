#include "QECommands/RemoveImportedPackageCommand.h"

#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/PackageControlsNode.h"
#include "Model/PackageHierarchy/ImportedPackagesNode.h"

using namespace DAVA;

RemoveImportedPackageCommand::RemoveImportedPackageCommand(PackageNode* package, PackageNode* importedPackage_)
    : QEPackageCommand(package, "Remove Imported Package")
    , importedPackage(RefPtr<PackageNode>::ConstructWithRetain(importedPackage_))
    , index(0)
{
    index = package->GetImportedPackagesNode()->GetIndex(importedPackage.Get());
}

void RemoveImportedPackageCommand::Redo()
{
    package->RemoveImportedPackage(importedPackage.Get());
}

void RemoveImportedPackageCommand::Undo()
{
    package->InsertImportedPackage(importedPackage.Get(), index);
}
