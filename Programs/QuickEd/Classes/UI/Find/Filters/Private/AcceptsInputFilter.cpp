#include "UI/Find/Filters/AcceptsInputFilter.h"
#include "UI/Find/PackageInformation/ControlInformation.h"
#include "UI/Find/PackageInformation/PackageInformation.h"

#include <UI/UIControl.h>
#include <Reflection/ReflectedTypeDB.h>

using namespace DAVA;

AcceptsInputFilter::AcceptsInputFilter()
{
    FastName noInput("noInput");

    for (const auto& field : ReflectedTypeDB::Get<UIControl>()->GetStructure()->fields)
    {
        if (field->name == noInput)
        {
            refMember = field.get();
        }
    }
    DVASSERT(refMember != nullptr);
}

FindFilter::ePackageStatus AcceptsInputFilter::AcceptPackage(const PackageInformation* package) const
{
    return PACKAGE_CAN_ACCEPT_CONTROLS;
}

bool AcceptsInputFilter::AcceptControl(const ControlInformation* control) const
{
    const Any& noInput = control->GetControlPropertyValue(*refMember);
    if (noInput.CanCast<bool>())
    {
        return noInput.Cast<bool>() == false;
    }
    else
    {
        return true;
    }
}
