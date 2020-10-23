#include "UI/Find/Filters/ControlNameFilter.h"
#include "UI/Find/PackageInformation/ControlInformation.h"
#include "UI/Find/PackageInformation/PackageInformation.h"

using namespace DAVA;

ControlNameFilter::ControlNameFilter(const DAVA::String& pattern, bool caseSensitive)
    : regExp(pattern.c_str(), caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive)
{
}

FindFilter::ePackageStatus ControlNameFilter::AcceptPackage(const PackageInformation* package) const
{
    return PACKAGE_CAN_ACCEPT_CONTROLS;
}

bool ControlNameFilter::AcceptControl(const ControlInformation* control) const
{
    return regExp.exactMatch(control->GetName().c_str());
}
