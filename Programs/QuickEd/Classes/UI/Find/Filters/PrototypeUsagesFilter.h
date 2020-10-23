#pragma once

#include <Base/BaseTypes.h>
#include <Base/FastName.h>
#include "UI/Find/Filters/FindFilter.h"

class PrototypeUsagesFilter : public FindFilter
{
public:
    PrototypeUsagesFilter(const DAVA::String& packagePath, const DAVA::FastName& prototypeName);

    FindFilter::ePackageStatus AcceptPackage(const PackageInformation* package) const override;
    bool AcceptControl(const ControlInformation* control) const override;

private:
    DAVA::String packagePath;
    DAVA::FastName prototypeName;
};
