#pragma once

#include <Base/BaseTypes.h>
#include <Base/FastName.h>
#include "UI/Find/Filters/FindFilter.h"

class PackageVersionFilter : public FindFilter
{
public:
    enum class eCmpType
    {
        EQ,
        NOT_EQ,
        LE,
        LT,
        GE,
        GT,
    };

    PackageVersionFilter(DAVA::int32 version, eCmpType cmpType);

    FindFilter::ePackageStatus AcceptPackage(const PackageInformation* package) const override;
    bool AcceptControl(const ControlInformation* control) const override;

private:
    bool CmpVersion(DAVA::int32 packVersion) const;
    DAVA::int32 version;
    eCmpType cmpType = eCmpType::EQ;
};
