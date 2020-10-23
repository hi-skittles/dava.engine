#include "StyleSheetProperty.h"

#include "PropertyVisitor.h"
#include "IntrospectionProperty.h"
#include "VariantTypeProperty.h"

#include "Model/PackageHierarchy/StyleSheetNode.h"
#include "UI/Styles/UIStyleSheet.h"

#include "Reflection/ReflectionRegistrator.h"

using namespace DAVA;

class UIStyleSheetPropertyWrapper : public BaseObject, public UIStyleSheetProperty
{
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(UIStyleSheetPropertyWrapper, BaseObject)
    {
        ReflectionRegistrator<UIStyleSheetPropertyWrapper>::Begin()
        .Field("transition", static_cast<bool UIStyleSheetPropertyWrapper::*>(&UIStyleSheetPropertyWrapper::transition))
        .Field("transitionTime", static_cast<float32 UIStyleSheetPropertyWrapper::*>(&UIStyleSheetProperty::transitionTime))
        .Field("transitionFunction", static_cast<Interpolation::FuncType UIStyleSheetPropertyWrapper::*>(&UIStyleSheetPropertyWrapper::transitionFunction))
        [
        M::EnumT<Interpolation::FuncType>()
        ]
        .End();
    }

public:
    UIStyleSheetPropertyWrapper(const UIStyleSheetProperty& src)
        : UIStyleSheetProperty(src)
    {
    }
};

StyleSheetProperty::StyleSheetProperty(const DAVA::UIStyleSheetProperty& property_)
    : ValueProperty("prop", property_.value.GetType())
    , property(new UIStyleSheetPropertyWrapper(property_))
{
    const UIStyleSheetPropertyDescriptor& descr = UIStyleSheetPropertyDataBase::Instance()->GetStyleSheetPropertyByIndex(property->propertyIndex);
    SetName(String(descr.GetFullName().c_str()));
    SetOverridden(true);

    RefPtr<VariantTypeProperty> valueProp(new VariantTypeProperty("Value", property->value, descr.field));
    valueProp->SetValue(property->value);
    valueProp->SetParent(this);
    AddSubValueProperty(valueProp.Get());

    UIStyleSheetPropertyWrapper* pp = property.Get();
    Reflection ref = Reflection::Create(&pp);
    Vector<Reflection::Field> fields = ref.GetFields();
    for (const Reflection::Field& field : fields)
    {
        RefPtr<IntrospectionProperty> inspProp(IntrospectionProperty::Create(property.Get(), nullptr, field.key.Get<FastName>().c_str(), field.ref, nullptr));
        inspProp->SetValue(field.ref.GetValue());
        inspProp->SetParent(this);
        inspProp->DisableResetFeature();
        AddSubValueProperty(inspProp.Get());
    }
}

StyleSheetProperty::~StyleSheetProperty()
{
}

void StyleSheetProperty::Accept(PropertyVisitor* visitor)
{
    visitor->VisitStyleSheetProperty(this);
}

DAVA::uint32 StyleSheetProperty::GetFlags() const
{
    return EF_CAN_REMOVE;
}

StyleSheetProperty::ePropertyType StyleSheetProperty::GetType() const
{
    const UIStyleSheetPropertyDescriptor& descr = UIStyleSheetPropertyDataBase::Instance()->GetStyleSheetPropertyByIndex(property->propertyIndex);
    if (descr.field->meta)
    {
        const M::Enum* enumMeta = descr.field->meta->GetMeta<M::Enum>();
        if (enumMeta != nullptr)
        {
            return TYPE_ENUM;
        }

        const M::Flags* flagsMeta = descr.field->meta->GetMeta<M::Flags>();
        if (flagsMeta != nullptr)
        {
            return TYPE_FLAGS;
        }
    }
    return TYPE_VARIANT;
}

const EnumMap* StyleSheetProperty::GetEnumMap() const
{
    const UIStyleSheetPropertyDescriptor& descr = UIStyleSheetPropertyDataBase::Instance()->GetStyleSheetPropertyByIndex(property->propertyIndex);
    if (descr.field->meta)
    {
        const M::Enum* enumMeta = descr.field->meta->GetMeta<M::Enum>();
        if (enumMeta != nullptr)
        {
            return enumMeta->GetEnumMap();
        }

        const M::Flags* flagsMeta = descr.field->meta->GetMeta<M::Flags>();
        if (flagsMeta != nullptr)
        {
            return flagsMeta->GetFlagsMap();
        }
    }
    return nullptr;
}

Any StyleSheetProperty::GetValue() const
{
    return property->value;
}

void StyleSheetProperty::ApplyValue(const DAVA::Any& value)
{
    property->value = value;
}

Interpolation::FuncType StyleSheetProperty::GetTransitionFunction() const
{
    return property->transitionFunction;
}

void StyleSheetProperty::SetTransitionFunction(Interpolation::FuncType type)
{
    property->transitionFunction = type;
}

float32 StyleSheetProperty::GetTransitionTime() const
{
    return property->transitionTime;
}

void StyleSheetProperty::SetTransitionTime(DAVA::float32 transitionTime)
{
    property->transitionTime = transitionTime;
}

bool StyleSheetProperty::HasTransition() const
{
    return property->transition;
}

void StyleSheetProperty::SetTransition(bool transition)
{
    property->transition = transition;
}

uint32 StyleSheetProperty::GetPropertyIndex() const
{
    return property->propertyIndex;
}

const UIStyleSheetProperty& StyleSheetProperty::GetProperty() const
{
    return *property;
}
