#pragma once

#include "UI/Find/Filters/FindFilter.h"
#include <Base/BaseTypes.h>
#include <UI/Components/UIComponent.h>

class HasComponentFilter : public FindFilter
{
public:
    HasComponentFilter(const DAVA::Type* componentType);

    ePackageStatus AcceptPackage(const PackageInformation* package) const override;
    bool AcceptControl(const ControlInformation* control) const override;

private:
    const DAVA::Type* requiredComponentType;
};
