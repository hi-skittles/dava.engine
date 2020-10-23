#include "UI/Find/Filters/PackageVersionFilter.h"

#include "UI/Find/PackageInformation/PackageInformation.h"

ENUM_DECLARE(PackageVersionFilter::eCmpType)
{
    ENUM_ADD_DESCR(static_cast<int>(PackageVersionFilter::eCmpType::EQ), "Equal");
    ENUM_ADD_DESCR(static_cast<int>(PackageVersionFilter::eCmpType::NOT_EQ), "Not Equal");
    ENUM_ADD_DESCR(static_cast<int>(PackageVersionFilter::eCmpType::LE), "Less Or Equal");
    ENUM_ADD_DESCR(static_cast<int>(PackageVersionFilter::eCmpType::LT), "Less Than");
    ENUM_ADD_DESCR(static_cast<int>(PackageVersionFilter::eCmpType::GE), "Greater Or equal");
    ENUM_ADD_DESCR(static_cast<int>(PackageVersionFilter::eCmpType::GT), "Greater Than");
}

PackageVersionFilter::PackageVersionFilter(DAVA::int32 version_, eCmpType cmpType_)
    : version(version_)
    , cmpType(cmpType_)
{
}

FindFilter::ePackageStatus PackageVersionFilter::AcceptPackage(const PackageInformation* package) const
{
    return CmpVersion(package->GetVersion()) ? PACKAGE_FOUND : PACKAGE_NOT_INTERESTED;
}

bool PackageVersionFilter::AcceptControl(const ControlInformation* control) const
{
    return false;
}

bool PackageVersionFilter::CmpVersion(DAVA::int32 packVersion) const
{
    switch (cmpType)
    {
    case eCmpType::EQ:
        return packVersion == version;
    case eCmpType::NOT_EQ:
        return packVersion != version;
    case eCmpType::LE:
        return packVersion <= version;
    case eCmpType::LT:
        return packVersion < version;
    case eCmpType::GE:
        return packVersion >= version;
    case eCmpType::GT:
        return packVersion > version;
    }
    DVASSERT(false);
    return false;
}
