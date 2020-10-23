#include "ClassProperty.h"

#include "PropertyVisitor.h"
#include "../PackageHierarchy/ControlNode.h"

#include "UI/UIControl.h"

using namespace DAVA;

ClassProperty::ClassProperty(ControlNode* aControl)
    : ValueProperty("Class", Type::Instance<String>())
    , control(aControl) // weak
{
}

ClassProperty::~ClassProperty()
{
    control = nullptr; // weak
}

void ClassProperty::Accept(PropertyVisitor* visitor)
{
    visitor->VisitClassProperty(this);
}

bool ClassProperty::IsReadOnly() const
{
    return true;
}

ClassProperty::ePropertyType ClassProperty::GetType() const
{
    return TYPE_VARIANT;
}

Any ClassProperty::GetValue() const
{
    return Any(control->GetControl()->GetClassName());
}

const String& ClassProperty::GetClassName() const
{
    return control->GetControl()->GetClassName();
}

ControlNode* ClassProperty::GetControlNode() const
{
    return control;
}
