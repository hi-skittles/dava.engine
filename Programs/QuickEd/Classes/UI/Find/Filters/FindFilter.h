#pragma once

class PackageInformation;
class ControlInformation;

class FindFilter
{
public:
    enum ePackageStatus
    {
        PACKAGE_NOT_INTERESTED,
        PACKAGE_CAN_ACCEPT_CONTROLS,
        PACKAGE_FOUND
    };

    virtual ~FindFilter() = default;

    virtual ePackageStatus AcceptPackage(const PackageInformation* package) const = 0;
    virtual bool AcceptControl(const ControlInformation* control) const = 0;
};
