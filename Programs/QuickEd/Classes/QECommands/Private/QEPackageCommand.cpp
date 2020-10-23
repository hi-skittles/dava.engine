#include "QECommands/Private/QEPackageCommand.h"
#include "Model/PackageHierarchy/PackageNode.h"

QEPackageCommand::QEPackageCommand(PackageNode* package_, const DAVA::String& description)
    : DAVA::Command(description)
    , package(DAVA::RefPtr<PackageNode>::ConstructWithRetain(package_))
{
}
