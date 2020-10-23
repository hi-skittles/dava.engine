#include "UI/Find/Filters/NegationFilter.h"
#include "UI/Find/PackageInformation/ControlInformation.h"
#include "UI/Find/PackageInformation/PackageInformation.h"

using namespace DAVA;

NegationFilter::NegationFilter(std::shared_ptr<FindFilter> filter_)
    : filter(filter_)
{
}

FindFilter::ePackageStatus NegationFilter::AcceptPackage(const PackageInformation* package) const
{
    return filter->AcceptPackage(package);
}

bool NegationFilter::AcceptControl(const ControlInformation* control) const
{
    return !filter->AcceptControl(control);
}
