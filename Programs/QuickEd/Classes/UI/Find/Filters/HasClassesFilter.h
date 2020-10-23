#pragma once

#include "UI/Find/Filters/FindFilter.h"
#include <Base/BaseTypes.h>
#include <Reflection/ReflectedStructure.h>

class HasClassesFilter : public FindFilter
{
public:
    HasClassesFilter(const DAVA::Vector<DAVA::String>& requiredClasses);

    ePackageStatus AcceptPackage(const PackageInformation* package) const override;
    bool AcceptControl(const ControlInformation* control) const override;

private:
    DAVA::Vector<DAVA::String> requiredClasses;
    const DAVA::ReflectedStructure::Field* refMember = nullptr;
};
