#include "UI/Find/PackageInformation/PackageNodeInformation.h"
#include "UI/Find/PackageInformation/ControlNodeInformation.h"
#include "Model/PackageHierarchy/ImportedPackagesNode.h"
#include "Model/PackageHierarchy/PackageControlsNode.h"
#include "Model/PackageHierarchy/PackageNode.h"

#include <UI/UIPackage.h>

using namespace DAVA;

PackageNodeInformation::PackageNodeInformation(const PackageNode* packageNode_, DAVA::int32 version_)
    : packageNode(packageNode_)
    , version(version_)
{
}

String PackageNodeInformation::GetPath() const
{
    return packageNode->GetPath().GetFrameworkPath();
}

DAVA::int32 PackageNodeInformation::GetVersion() const
{
    return version;
}

void PackageNodeInformation::VisitImportedPackages(const DAVA::Function<void(const PackageInformation*)>& visitor) const
{
    ImportedPackagesNode* importedPackagesNode = packageNode->GetImportedPackagesNode();

    for (int32 i = 0; i < importedPackagesNode->GetCount(); i++)
    {
        PackageNodeInformation importedPackageInfo(importedPackagesNode->GetImportedPackage(i), UIPackage::CURRENT_VERSION);

        visitor(&importedPackageInfo);
    }
}

void PackageNodeInformation::VisitControls(const DAVA::Function<void(const ControlInformation*)>& visitor) const
{
    PackageControlsNode* controlsNode = packageNode->GetPackageControlsNode();

    for (int32 i = 0; i < controlsNode->GetCount(); i++)
    {
        ControlNodeInformation controlInfo(controlsNode->Get(i));

        visitor(&controlInfo);
    }
}

void PackageNodeInformation::VisitPrototypes(const DAVA::Function<void(const ControlInformation*)>& visitor) const
{
    PackageControlsNode* prototypesNode = packageNode->GetPrototypes();

    for (int32 i = 0; i < prototypesNode->GetCount(); i++)
    {
        ControlNodeInformation prototypeInfo(prototypesNode->Get(i));

        visitor(&prototypeInfo);
    }
}
