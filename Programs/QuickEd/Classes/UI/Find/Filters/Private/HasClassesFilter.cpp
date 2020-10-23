#include "UI/Find/Filters/HasClassesFilter.h"
#include "UI/Find/PackageInformation/ControlInformation.h"
#include "UI/Find/PackageInformation/PackageInformation.h"

#include <algorithm>
#include <Reflection/ReflectedTypeDB.h>
#include <UI/UIControl.h>
#include <Utils/Utils.h>

using namespace DAVA;

HasClassesFilter::HasClassesFilter(const Vector<String>& requiredClasses_)
    : requiredClasses(requiredClasses_)
{
    FastName classes("classes");

    std::sort(requiredClasses.begin(), requiredClasses.end());

    for (const auto& field : ReflectedTypeDB::Get<UIControl>()->GetStructure()->fields)
    {
        if (field->name == classes)
        {
            refMember = field.get();
        }
    }
}

FindFilter::ePackageStatus HasClassesFilter::AcceptPackage(const PackageInformation* package) const
{
    return PACKAGE_CAN_ACCEPT_CONTROLS;
}

bool HasClassesFilter::AcceptControl(const ControlInformation* control) const
{
    const Any& classesStr = control->GetControlPropertyValue(*refMember);
    if (classesStr.CanCast<String>())
    {
        Vector<String> classes;
        Split(classesStr.Cast<String>(), " ", classes);

        std::sort(classes.begin(), classes.end());

        return std::includes(classes.begin(), classes.end(), requiredClasses.begin(), requiredClasses.end());
    }
    else
    {
        return requiredClasses.empty();
    }
}
