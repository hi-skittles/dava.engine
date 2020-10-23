#pragma once

#include "UI/Find/Filters/FindFilter.h"
#include <Base/BaseTypes.h>
#include <Reflection/ReflectedStructure.h>

class AcceptsInputFilter : public FindFilter
{
public:
    AcceptsInputFilter();

    ePackageStatus AcceptPackage(const PackageInformation* package) const override;
    bool AcceptControl(const ControlInformation* control) const override;

private:
    const DAVA::ReflectedStructure::Field* refMember = nullptr;
};
