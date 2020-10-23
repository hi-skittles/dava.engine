#pragma once

#include "QECommands/Private/QEPackageCommand.h"

class PackageControlsNode;

class InsertImportedPackageCommand : public QEPackageCommand
{
public:
    InsertImportedPackageCommand(PackageNode* package, PackageNode* importedPackage, int index);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<PackageNode> importedPackage;
    const int index;
};
