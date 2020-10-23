#pragma once

#include <Base/BaseTypes.h>
#include "UI/Find/PackageInformation/ControlInformation.h"

class PackageInformation
{
public:
    virtual ~PackageInformation() = default;

    virtual DAVA::String GetPath() const = 0;
    virtual DAVA::int32 GetVersion() const = 0;

    virtual void VisitImportedPackages(const DAVA::Function<void(const PackageInformation*)>& visitor) const = 0;
    virtual void VisitControls(const DAVA::Function<void(const ControlInformation*)>& visitor) const = 0;
    virtual void VisitPrototypes(const DAVA::Function<void(const ControlInformation*)>& visitor) const = 0;
};
