#include "CustomClassProperty.h"

#include "PropertyVisitor.h"
#include "../PackageHierarchy/ControlNode.h"

#include "UI/UIControl.h"

using namespace DAVA;

CustomClassProperty::CustomClassProperty(ControlNode* aControl, const CustomClassProperty* prototypeProperty)
    : ValueProperty("Custom Class", Type::Instance<String>())
    , control(aControl) // weak
{
    if (prototypeProperty)
    {
        AttachPrototypeProperty(prototypeProperty);
        SetDefaultValue(prototypeProperty->GetValue());
        customClass = prototypeProperty->customClass;
    }
    else
    {
        SetDefaultValue(String(""));
    }
}

CustomClassProperty::~CustomClassProperty()
{
    control = nullptr; //weak
}

void CustomClassProperty::Accept(PropertyVisitor* visitor)
{
    visitor->VisitCustomClassProperty(this);
}

bool CustomClassProperty::IsReadOnly() const
{
    return control->GetCreationType() == ControlNode::CREATED_FROM_PROTOTYPE_CHILD || ValueProperty::IsReadOnly();
}

CustomClassProperty::ePropertyType CustomClassProperty::GetType() const
{
    return TYPE_VARIANT;
}

Any CustomClassProperty::GetValue() const
{
    return Any(customClass);
}

const String& CustomClassProperty::GetCustomClassName() const
{
    return customClass;
}

void CustomClassProperty::ApplyValue(const DAVA::Any& value)
{
    customClass = value.Cast<String>();
}
