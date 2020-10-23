#include "UI/Find/Filters/HasErrorsFilter.h"

#include "UI/Find/PackageInformation/ControlInformation.h"
#include "UI/Find/PackageInformation/PackageInformation.h"

using namespace DAVA;

HasErrorsFilter::HasErrorsFilter()
{
}

FindFilter::ePackageStatus HasErrorsFilter::AcceptPackage(const PackageInformation* package) const
{
    return PACKAGE_CAN_ACCEPT_CONTROLS;
}

bool HasErrorsFilter::AcceptControl(const ControlInformation* control) const
{
    return control->HasErrors();
}
