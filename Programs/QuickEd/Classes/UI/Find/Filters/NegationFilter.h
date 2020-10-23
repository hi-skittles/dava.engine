#pragma once

#include "UI/Find/Filters/FindFilter.h"
#include <Base/BaseTypes.h>

class NegationFilter : public FindFilter
{
public:
    NegationFilter(std::shared_ptr<FindFilter> filter);

    FindFilter::ePackageStatus AcceptPackage(const PackageInformation* package) const override;
    bool AcceptControl(const ControlInformation* control) const override;

private:
    std::shared_ptr<FindFilter> filter;
};
