#pragma once

#include <Base/BaseTypes.h>
#include <Base/FastName.h>
#include "UI/Find/Filters/FindFilter.h"

class HasErrorsFilter : public FindFilter
{
public:
    HasErrorsFilter();

    FindFilter::ePackageStatus AcceptPackage(const PackageInformation* package) const override;
    bool AcceptControl(const ControlInformation* control) const override;
};
