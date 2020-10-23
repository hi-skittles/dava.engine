#include "UI/Find/PackageInformation/StaticPackageInformation.h"

using namespace DAVA;

StaticPackageInformation::StaticPackageInformation(const String& path_, int32 version_)
    : path(path_)
    , version(version_)
{
}

String StaticPackageInformation::GetPath() const
{
    return path;
}

DAVA::int32 StaticPackageInformation::GetVersion() const
{
    return version;
}

void StaticPackageInformation::VisitImportedPackages(const Function<void(const PackageInformation*)>& visitor) const
{
    for (const std::shared_ptr<PackageInformation>& importedPackage : importedPackages)
    {
        visitor(importedPackage.get());
    }
}

void StaticPackageInformation::VisitControls(const Function<void(const ControlInformation*)>& visitor) const
{
    for (const std::shared_ptr<ControlInformation>& control : controls)
    {
        visitor(control.get());
    }
}

void StaticPackageInformation::VisitPrototypes(const Function<void(const ControlInformation*)>& visitor) const
{
    for (const std::shared_ptr<ControlInformation>& prototype : prototypes)
    {
        visitor(prototype.get());
    }
}

void StaticPackageInformation::AddImportedPackage(const std::shared_ptr<StaticPackageInformation>& package)
{
    importedPackages.push_back(package);
}

void StaticPackageInformation::AddControl(const std::shared_ptr<StaticControlInformation>& control)
{
    controls.push_back(control);
}

void StaticPackageInformation::AddPrototype(const std::shared_ptr<StaticControlInformation>& prototype)
{
    prototypes.push_back(prototype);
}

const Vector<std::shared_ptr<StaticPackageInformation>>& StaticPackageInformation::GetImportedPackages() const
{
    return importedPackages;
}

const Vector<std::shared_ptr<StaticControlInformation>>& StaticPackageInformation::GetPrototypes() const
{
    return prototypes;
}

const Vector<std::shared_ptr<StaticControlInformation>>& StaticPackageInformation::GetControls() const
{
    return controls;
}

std::shared_ptr<StaticControlInformation> StaticPackageInformation::FindPrototypeByName(const FastName& name) const
{
    for (const std::shared_ptr<StaticControlInformation>& prototype : prototypes)
    {
        if (prototype->GetName() == name)
        {
            return prototype;
        }
    }
    return std::shared_ptr<StaticControlInformation>();
}

void StaticPackageInformation::AddResult(const DAVA::Result& result)
{
    results.AddResult(result);
}

bool StaticPackageInformation::HasErrors() const
{
    return results.HasErrors();
}
