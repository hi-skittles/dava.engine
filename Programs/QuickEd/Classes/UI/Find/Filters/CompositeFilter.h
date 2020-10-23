#pragma once

#include "UI/Find/Filters/FindFilter.h"
#include <Base/BaseTypes.h>

class CompositeFilter : public FindFilter
{
public:
    enum eCompositionType
    {
        AND,
        OR
    };

    CompositeFilter(const DAVA::Vector<std::shared_ptr<FindFilter>>& filters);

    ePackageStatus AcceptPackage(const PackageInformation* package) const override;
    bool AcceptControl(const ControlInformation* control) const override;
    void SetCompositionType(eCompositionType type);
    eCompositionType GetCompositionType() const;

private:
    DAVA::Vector<std::shared_ptr<FindFilter>> filters;
    eCompositionType compositionType = AND;
};
