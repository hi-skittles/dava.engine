#include "Classes/Model/ControlProperties/NameProperty.h"

#include "Classes/Model/ControlProperties/PropertyVisitor.h"
#include "Classes/Model/PackageHierarchy/ControlNode.h"

#include <UI/UIControl.h>
#include <UI/UIControlHelpers.h>

NameProperty::NameProperty(ControlNode* controlNode_, const NameProperty* prototypeProperty)
    : ValueProperty("Name", DAVA::Type::Instance<DAVA::FastName>())
    , controlNode(controlNode_)
{
    using namespace DAVA;

    FastName name;
    if (prototypeProperty != nullptr)
    {
        name = prototypeProperty->GetValue().Cast<FastName>();

        if (controlNode->GetCreationType() == ControlNode::CREATED_FROM_PROTOTYPE_CHILD)
        {
            AttachPrototypeProperty(prototypeProperty);
        }
    }
    else
    {
        name = controlNode->GetControl()->GetName();
    }

    if (name.IsValid() == false)
    {
        name = FastName("");
    }

    SetDefaultValue(FastName(""));
    value = name;
    if (UIControlHelpers::IsControlNameValid(name))
    {
        controlNode->GetControl()->SetName(name);
    }
    else
    {
        controlNode->GetControl()->SetName("generated");
    }
}

void NameProperty::Refresh(DAVA::int32 refreshFlags)
{
    ValueProperty::Refresh(refreshFlags);

    if ((refreshFlags & REFRESH_DEFAULT_VALUE) != 0 && GetPrototypeProperty())
        ApplyValue(GetDefaultValue());
}

void NameProperty::Accept(PropertyVisitor* visitor)
{
    visitor->VisitNameProperty(this);
}

bool NameProperty::IsReadOnly() const
{
    return controlNode->GetCreationType() == ControlNode::CREATED_FROM_PROTOTYPE_CHILD || ValueProperty::IsReadOnly();
}

NameProperty::ePropertyType NameProperty::GetType() const
{
    return TYPE_VARIANT;
}

DAVA::Any NameProperty::GetValue() const
{
    return value;
}

bool NameProperty::IsOverriddenLocally() const
{
    return controlNode->GetCreationType() != ControlNode::CREATED_FROM_PROTOTYPE_CHILD;
}

ControlNode* NameProperty::GetControlNode() const
{
    return controlNode;
}

void NameProperty::ApplyValue(const DAVA::Any& newValue)
{
    using namespace DAVA;

    if (newValue.CanCast<FastName>())
    {
        FastName name = newValue.Cast<FastName>();
        value = Any(name);
        if (UIControlHelpers::IsControlNameValid(name))
        {
            controlNode->GetControl()->SetName(name);
        }
    }
    else
    {
        DVASSERT(false);
    }
}
