#include "Reflection/Private/Metas.h"

#include <Debug/DVAssert.h>

namespace DAVA
{
namespace Metas
{
DisplayName::DisplayName(const String& displayName_)
    : displayName(displayName_)
{
}

Range::Range(const Any& minValue_, const Any& maxValue_, const Any& step_)
    : minValue(minValue_)
    , maxValue(maxValue_)
    , step(step_)
{
}

FloatNumberAccuracy::FloatNumberAccuracy(uint32 accuracy_)
    : accuracy(accuracy_)
{
}

MaxLength::MaxLength(uint32 length_)
    : length(length_)
{
}

Validator::Validator(const TValidationFn& fn_)
    : fn(fn_)
{
}

ValidationResult Validator::Validate(const Any& value, const Any& prevValue) const
{
    DVASSERT(fn != nullptr);
    return fn(value, prevValue);
}

File::File(const String& filters_, const String& dlgTitle_)
    : filters(filters_)
    , dlgTitle(dlgTitle_)
{
}

String File::GetDefaultPath() const
{
    return "";
}

String File::GetRootDirectory() const
{
    return "";
}

Group::Group(const char* groupName_)
    : groupName(groupName_)
{
}

ValueDescription::ValueDescription(const TValueDescriptorFn& fn_)
    : fn(fn_)
{
    DVASSERT(fn != nullptr);
}

String ValueDescription::GetDescription(const Any& v) const
{
    return fn(v);
}

Bindable::Bindable()
{
}

Tooltip::Tooltip(const String& tooltipFieldName_)
    : tooltipFieldName(tooltipFieldName_)
{
}

} // namespace Metas
} // namespace DAVA
