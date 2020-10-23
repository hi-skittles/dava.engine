#pragma once

#include "QECommands/Private/QEPackageCommand.h"

class PackageNode;
class PackageControlsNode;

class RemoveImportedPackageCommand : public QEPackageCommand
{
public:
    RemoveImportedPackageCommand(PackageNode* package, PackageNode* importedPackage);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<PackageNode> importedPackage;
    int index = -1;
};
