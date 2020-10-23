#include "VisibleValueProperty.h"

#include <UI/UIControl.h>

using namespace DAVA;

VisibleValueProperty::VisibleValueProperty(DAVA::BaseObject* object, const String& name, const Reflection& ref, const IntrospectionProperty* prototypeProperty)
    : IntrospectionProperty(object, nullptr, name, ref, prototypeProperty)
{
}

void VisibleValueProperty::SetVisibleInEditor(bool visible)
{
    DynamicTypeCheck<UIControl*>(GetBaseObject())->SetHiddenForDebug(!visible);
}

bool VisibleValueProperty::GetVisibleInEditor() const
{
    return !DynamicTypeCheck<UIControl*>(GetBaseObject())->IsHiddenForDebug();
}
