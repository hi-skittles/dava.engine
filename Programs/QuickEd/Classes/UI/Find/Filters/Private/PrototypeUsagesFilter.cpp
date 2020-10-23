#include "UI/Find/Filters/PrototypeUsagesFilter.h"
#include "UI/Find/PackageInformation/ControlInformation.h"
#include "UI/Find/PackageInformation/PackageInformation.h"

using namespace DAVA;

PrototypeUsagesFilter::PrototypeUsagesFilter(const DAVA::String& packagePath_, const DAVA::FastName& prototypeName_)
    : packagePath(packagePath_)
    , prototypeName(prototypeName_)
{
}

FindFilter::ePackageStatus PrototypeUsagesFilter::AcceptPackage(const PackageInformation* package) const
{
    if (package->GetPath() == packagePath)
    {
        return PACKAGE_CAN_ACCEPT_CONTROLS;
    }

    bool imports = false;

    package->VisitImportedPackages(
    [&imports, this](const PackageInformation* package)
    {
        if (package->GetPath() == packagePath)
        {
            imports = true;
        }
    });

    return imports ? PACKAGE_CAN_ACCEPT_CONTROLS : PACKAGE_NOT_INTERESTED;
}

bool PrototypeUsagesFilter::AcceptControl(const ControlInformation* control) const
{
    while (control != nullptr)
    {
        if (control->GetPrototypeName() == prototypeName && control->GetPrototypePackagePath() == packagePath)
        {
            return true;
        }
        control = control->GetPrototype();
    }

    return false;
}
