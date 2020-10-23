#include "FontValueProperty.h"

#include <Base/Any.h>
#include <Reflection/Reflection.h>
#include <Reflection/ReflectedObject.h>
#include <Reflection/ReflectedTypeDB.h>

using namespace DAVA;

FontValueProperty::FontValueProperty(DAVA::BaseObject* object, const DAVA::Type* componentType, const DAVA::String& name, const DAVA::Reflection& ref, const IntrospectionProperty* prototypeProperty)
    : IntrospectionProperty(object, componentType, name, ref, prototypeProperty)
{
    ApplyValue(ref.GetValue());
}

FontValueProperty::~FontValueProperty()
{
}

void FontValueProperty::Refresh(DAVA::int32 refreshFlags)
{
    IntrospectionProperty::Refresh(refreshFlags);

    if (refreshFlags & REFRESH_FONT)
    {
        reflection.SetValue(presetName);
    }
}

Any FontValueProperty::GetValue() const
{
    return reflection.GetValue();
}

void FontValueProperty::ApplyValue(const DAVA::Any& value)
{
    presetName = value.Cast<String>();
    reflection.SetValue(presetName);
}
