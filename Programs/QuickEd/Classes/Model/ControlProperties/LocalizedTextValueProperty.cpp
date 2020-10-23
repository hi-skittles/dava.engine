#include "LocalizedTextValueProperty.h"

#include <FileSystem/LocalizationSystem.h>
#include <Reflection/Reflection.h>
#include <Reflection/ReflectedObject.h>
#include <Reflection/ReflectedTypeDB.h>

using namespace DAVA;

LocalizedTextValueProperty::LocalizedTextValueProperty(DAVA::BaseObject* anObject, const DAVA::Type* componentType, const DAVA::String& name, const DAVA::Reflection& ref, const IntrospectionProperty* prototypeProperty)
    : IntrospectionProperty(anObject, componentType, name, ref, prototypeProperty)
{
    ApplyValue(ref.GetValue());
}

LocalizedTextValueProperty::~LocalizedTextValueProperty()
{
}

void LocalizedTextValueProperty::Refresh(DAVA::int32 refreshFlags)
{
    IntrospectionProperty::Refresh(refreshFlags);
    if ((refreshFlags & REFRESH_LOCALIZATION) != 0 && !IsBound())
    {
        reflection.SetValue(LocalizedUtf8String(text));
    }
}

Any LocalizedTextValueProperty::GetValue() const
{
    return Any(text);
}

void LocalizedTextValueProperty::ApplyValue(const DAVA::Any& value)
{
    text = value.Get<String>();
    reflection.SetValue(LocalizedUtf8String(text));
}
