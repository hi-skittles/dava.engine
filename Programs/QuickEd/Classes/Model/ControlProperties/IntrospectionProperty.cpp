#include "IntrospectionProperty.h"

#include "Model/ControlProperties/LocalizedTextValueProperty.h"
#include "Model/ControlProperties/FontValueProperty.h"
#include "Model/ControlProperties/VisibleValueProperty.h"

#include "Model/ControlProperties/PropertyVisitor.h"
#include "Model/ControlProperties/SubValueProperty.h"
#include "Model/ControlProperties/RootProperty.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/PackageHierarchy/PackageNode.h"

#include <Base/BaseMath.h>
#include <Base/RefPtrUtils.h>
#include <Reflection/Reflection.h>
#include <Reflection/ReflectedTypeDB.h>
#include <Reflection/ReflectedMeta.h>
#include <UI/DataBinding/UIDataBindingComponent.h>
#include <UI/Layouts/UILayoutSourceRectComponent.h>
#include <UI/Styles/UIStyleSheetPropertyDataBase.h>
#include <UI/UIControl.h>
#include <UI/UIScrollViewContainer.h>
#include <UI/UISlider.h>
#include <UI/UISwitch.h>
#include <UI/Text/UITextComponent.h>
#include <UI/UITextField.h>

using namespace DAVA;

namespace
{
const String INTROSPECTION_PROPERTY_NAME_SIZE("size");
const String INTROSPECTION_PROPERTY_NAME_POSITION("position");
const String INTROSPECTION_PROPERTY_NAME_TEXT("text");
const String INTROSPECTION_PROPERTY_NAME_FONT("font");
const String INTROSPECTION_PROPERTY_NAME_FONT_NAME("fontName");
const String INTROSPECTION_PROPERTY_NAME_CLASSES("classes");
const String INTROSPECTION_PROPERTY_NAME_VISIBLE("visible");
const String INTROSPECTION_PROPERTY_NAME_CONTROL_PROTOTYPE("prototype");
}

IntrospectionProperty::IntrospectionProperty(DAVA::BaseObject* anObject, const DAVA::Type* componentType_, const String& name, const DAVA::Reflection& ref, const IntrospectionProperty* prototypeProperty)
    : ValueProperty(name, ref.GetValueType())
    , object(SafeRetain(anObject))
    , reflection(ref)
    , flags(EF_CAN_RESET)
    , componentType(componentType_)
{
    int32 propertyIndex = UIStyleSheetPropertyDataBase::Instance()->FindStyleSheetProperty(componentType, FastName(name));
    SetStylePropertyIndex(propertyIndex);

    bindable = reflection.GetMeta<M::Bindable>() != nullptr;

    if (prototypeProperty)
    {
        AttachPrototypeProperty(prototypeProperty);
        SetDefaultValue(reflection.GetValue());
        reflection.SetValue(prototypeProperty->GetValue());
    }
    else
    {
        SetDefaultValue(reflection.GetValue());
    }

    GenerateBuiltInSubProperties();

    if (name == INTROSPECTION_PROPERTY_NAME_SIZE || name == INTROSPECTION_PROPERTY_NAME_POSITION)
    {
        UIControl* control = DynamicTypeCheck<UIControl*>(anObject);
        bool shouldAddSourceRectComponent = true;

        if (dynamic_cast<UIScrollViewContainer*>(control) != nullptr)
        {
            shouldAddSourceRectComponent = false;
        }
        else
        {
            if (control->GetName() == UISlider::THUMB_SPRITE_CONTROL_NAME ||
                control->GetName() == UISlider::MIN_SPRITE_CONTROL_NAME ||
                control->GetName() == UISlider::MAX_SPRITE_CONTROL_NAME ||
                control->GetName() == UISwitch::BUTTON_LEFT_NAME ||
                control->GetName() == UISwitch::BUTTON_RIGHT_NAME ||
                control->GetName() == UISwitch::BUTTON_TOGGLE_NAME)
            {
                shouldAddSourceRectComponent = false;
            }
        }

        if (shouldAddSourceRectComponent)
        {
            sourceRectComponent = control->GetOrCreateComponent<UILayoutSourceRectComponent>();

            if (prototypeProperty != nullptr && prototypeProperty->sourceRectComponent)
            {
                if (name == INTROSPECTION_PROPERTY_NAME_SIZE)
                {
                    sourceRectComponent->SetSize(prototypeProperty->sourceRectComponent->GetSize());
                }
                else
                {
                    sourceRectComponent->SetPosition(prototypeProperty->sourceRectComponent->GetPosition());
                }
            }
            else
            {
                SetLayoutSourceRectValue(reflection.GetValue());
            }
        }
    }

    // If "UITextField" has helper component "UITextComponent" mark all properties as "ReadOnly".
    UITextComponent* text = CastIfEqual<UITextComponent*>(anObject);
    if (text)
    {
        forceReadOnly = dynamic_cast<UITextField*>(text->GetControl()) != nullptr;
    }
}

IntrospectionProperty::~IntrospectionProperty()
{
    if (bindingComponent.Valid() && bindingComponent->GetControl())
    {
        bindingComponent->GetControl()->RemoveComponent(bindingComponent.Get());
        bindingComponent = nullptr;
    }
    SafeRelease(object);
}

IntrospectionProperty* IntrospectionProperty::Create(BaseObject* object, const DAVA::Type* componentType, const String& name, const Reflection& ref, const IntrospectionProperty* prototypeProperty)
{
    if (name == INTROSPECTION_PROPERTY_NAME_TEXT)
    {
        return new LocalizedTextValueProperty(object, componentType, name, ref, prototypeProperty);
    }
    else if (name == INTROSPECTION_PROPERTY_NAME_FONT || name == INTROSPECTION_PROPERTY_NAME_FONT_NAME)
    {
        return new FontValueProperty(object, componentType, name, ref, prototypeProperty);
    }
    else if (name == INTROSPECTION_PROPERTY_NAME_VISIBLE)
    {
        return new VisibleValueProperty(object, name, ref, prototypeProperty);
    }
    else
    {
        return new IntrospectionProperty(object, componentType, name, ref, prototypeProperty);
    }
}

void IntrospectionProperty::Accept(PropertyVisitor* visitor)
{
    visitor->VisitIntrospectionProperty(this);
}

uint32 IntrospectionProperty::GetFlags() const
{
    uint32 result = flags;
    if (GetPrototypeProperty() && !IsOverriddenLocally() && IsOverridden())
        result |= EF_INHERITED;
    return result;
}

IntrospectionProperty::ePropertyType IntrospectionProperty::GetType() const
{
    const M::Enum* enumMeta = reflection.GetMeta<M::Enum>();
    if (enumMeta)
    {
        return TYPE_ENUM;
    }

    const M::Flags* flagsMeta = reflection.GetMeta<M::Flags>();
    if (flagsMeta)
    {
        return TYPE_FLAGS;
    }

    return TYPE_VARIANT;
}

const EnumMap* IntrospectionProperty::GetEnumMap() const
{
    const M::Enum* enumMeta = reflection.GetMeta<M::Enum>();
    if (enumMeta != nullptr)
    {
        return enumMeta->GetEnumMap();
    }

    const M::Flags* flagsMeta = reflection.GetMeta<M::Flags>();
    if (flagsMeta != nullptr)
    {
        return flagsMeta->GetFlagsMap();
    }

    return nullptr;
}

const DAVA::String& IntrospectionProperty::GetDisplayName() const
{
    const M::DisplayName* displayName = reflection.GetMeta<M::DisplayName>();
    if (displayName)
    {
        return displayName->displayName;
    }
    return GetName();
}

Any IntrospectionProperty::GetValue() const
{
    return reflection.GetValue();
}

DAVA::Any IntrospectionProperty::GetSerializationValue() const
{
    if (sourceRectComponent.Valid())
    {
        if (GetName() == INTROSPECTION_PROPERTY_NAME_SIZE)
        {
            return sourceRectComponent->GetSize();
        }
        else if (GetName() == INTROSPECTION_PROPERTY_NAME_POSITION)
        {
            return sourceRectComponent->GetPosition();
        }
        else
        {
            DVASSERT(false);
        }
    }

    return GetValue();
}

void IntrospectionProperty::DisableResetFeature()
{
    flags &= ~EF_CAN_RESET;
}

void IntrospectionProperty::ResetValue()
{
    ValueProperty::ResetValue();
    ResetBindingExpression();
}

void IntrospectionProperty::Refresh(DAVA::int32 refreshFlags)
{
    ValueProperty::Refresh(refreshFlags);

    if (!IsOverriddenLocally())
    {
        if (GetPrototypeProperty() && GetPrototypeProperty()->IsBound())
        {
            SetBindingExpressionImpl(GetPrototypeProperty()->GetBindingExpression(), GetPrototypeProperty()->GetBindingUpdateMode());
        }
        else
        {
            ResetBindingExpression();
        }
    }
    else if (IsBound())
    {
        SetBindingExpressionImpl(bindingExpression, bindingMode);
    }
}

bool IntrospectionProperty::IsBindable() const
{
    return bindable;
}

bool IntrospectionProperty::IsBound() const
{
    return bound;
}

DAVA::int32 IntrospectionProperty::GetBindingUpdateMode() const
{
    return bindingMode;
}

String IntrospectionProperty::GetBindingExpression() const
{
    return bindingExpression;
}

void IntrospectionProperty::SetBindingExpression(const String& expression, DAVA::int32 bindingUpdateMode)
{
    ValueProperty::SetBindingExpression(expression, bindingUpdateMode);
    SetOverridden(true);
    SetBindingExpressionImpl(expression, bindingUpdateMode);
}

DAVA::String IntrospectionProperty::GetFullFieldName() const
{
    String name;
    if (componentType != nullptr)
    {
        name = ReflectedTypeDB::GetByType(componentType)->GetPermanentName();
        name += ".";
    }
    name += GetName();
    return name;
}

bool IntrospectionProperty::HasError() const
{
    return false;
}

DAVA::String IntrospectionProperty::GetErrorString() const
{
    return "";
}

bool IntrospectionProperty::IsReadOnly() const
{
    return forceReadOnly || ValueProperty::IsReadOnly();
}

void IntrospectionProperty::ComponentWithPropertyWasInstalled()
{
    if (bindingComponent.Valid())
    {
        GetLinkedControl()->AddComponent(bindingComponent.Get());
    }
}

void IntrospectionProperty::ComponentWithPropertyWasUninstalled()
{
    if (bindingComponent.Valid())
    {
        GetLinkedControl()->RemoveComponent(bindingComponent.Get());
    }
}

void IntrospectionProperty::SetBindingExpressionImpl(const DAVA::String& expression, DAVA::int32 bindingUpdateMode)
{
    UIControl* control = GetLinkedControl();

    bindingExpression = expression;
    bindingMode = bindingUpdateMode;
    bound = true;

    DVASSERT(reflection.GetMeta<M::Bindable>() != nullptr);

    if (bindingComponent == nullptr)
    {
        for (uint32 i = 0; i < control->GetComponentCount<UIDataBindingComponent>(); i++)
        {
            UIDataBindingComponent* candidate = control->GetComponent<UIDataBindingComponent>(i);
            if (candidate->GetControlFieldName() == GetName())
            {
                bindingComponent = candidate;
                break;
            }
        }

        if (bindingComponent == nullptr)
        {
            bindingComponent = MakeRef<UIDataBindingComponent>();
            bindingComponent->SetControlFieldName(GetFullFieldName());
            control->AddComponent(bindingComponent.Get());
        }
    }

    if (bindingComponent)
    {
        bindingComponent->SetBindingExpression(bindingExpression);
        bindingComponent->SetUpdateMode(static_cast<UIDataBindingComponent::UpdateMode>(bindingUpdateMode));
    }
    else
    {
        DVASSERT(false);
    }
}

void IntrospectionProperty::ResetBindingExpression()
{
    bound = false;
    bindingExpression = "";

    if (bindingComponent.Valid())
    {
        bindingComponent->GetControl()->RemoveComponent(bindingComponent.Get());
        bindingComponent = nullptr;
    }
}

void IntrospectionProperty::ApplyValue(const DAVA::Any& value)
{
    reflection.SetValueWithCast(value);

    if (sourceRectComponent.Valid())
    {
        SetLayoutSourceRectValue(value);
    }
}

void IntrospectionProperty::SetLayoutSourceRectValue(const DAVA::Any& value)
{
    DVASSERT(sourceRectComponent.Valid());
    if (GetName() == INTROSPECTION_PROPERTY_NAME_SIZE)
    {
        sourceRectComponent->SetSize(value.Get<Vector2>());
    }
    else if (GetName() == INTROSPECTION_PROPERTY_NAME_POSITION)
    {
        UIControl* control = DynamicTypeCheck<UIControl*>(object);
        Vector2 p = value.Get<Vector2>();
        sourceRectComponent->SetPosition(p);
    }
    else
    {
        DVASSERT(false);
    }
}

DAVA::UIControl* IntrospectionProperty::GetLinkedControl()
{
    if (componentType != nullptr)
    {
        return DynamicTypeCheck<UIComponent*>(object)->GetControl();
    }
    return DynamicTypeCheck<UIControl*>(object);
}
