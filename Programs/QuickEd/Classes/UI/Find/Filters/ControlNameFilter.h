#pragma once

#include "UI/Find/Filters/FindFilter.h"
#include <Base/BaseTypes.h>
#include <QRegExp>

class ControlNameFilter : public FindFilter
{
public:
    ControlNameFilter(const DAVA::String& pattern, bool caseSensitive);

    ePackageStatus AcceptPackage(const PackageInformation* package) const override;
    bool AcceptControl(const ControlInformation* control) const override;

private:
    QRegExp regExp;
};
