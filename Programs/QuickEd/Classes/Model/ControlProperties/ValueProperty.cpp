#include "ValueProperty.h"

#include "SubValueProperty.h"
#include <Base/BaseMath.h>
#include <Reflection/ReflectionRegistrator.h>

using namespace DAVA;

namespace SValueProperty
{
static const Vector<String> VECTOR2_COMPONENT_NAMES = { "X", "Y" };
static const Vector<String> COLOR_COMPONENT_NAMES = { "Red", "Green", "Blue", "Alpha" };
static const Vector<String> MARGINS_COMPONENT_NAMES = { "Left", "Top", "Right", "Bottom" };
}

DAVA_VIRTUAL_REFLECTION_IMPL(ValueProperty)
{
    ReflectionRegistrator<ValueProperty>::Begin()
    .End();
}

ValueProperty::ValueProperty(const String& propName, const DAVA::Type* type_)
    : name(propName)
    , valueType(type_ != nullptr ? type_->Decay() : nullptr)
{
}

ValueProperty::~ValueProperty()
{
    children.clear();

    prototypeProperty = nullptr; // weak
}

uint32 ValueProperty::GetCount() const
{
    return static_cast<int32>(children.size());
}

AbstractProperty* ValueProperty::GetProperty(int32 index) const
{
    if (0 <= index && index < static_cast<int32>(children.size()))
    {
        return children[index].Get();
    }
    else
    {
        DVASSERT(false);
        return nullptr;
    }
}

void ValueProperty::Refresh(int32 refreshFlags)
{
    if ((refreshFlags & REFRESH_DEFAULT_VALUE) != 0 && prototypeProperty)
        SetDefaultValue(prototypeProperty->GetValue());

    for (RefPtr<AbstractProperty>& prop : children)
        prop->Refresh(refreshFlags);
}

void ValueProperty::AttachPrototypeProperty(const ValueProperty* property)
{
    if (prototypeProperty == nullptr)
    {
        prototypeProperty = property;
    }
    else
    {
        DVASSERT(false);
    }
}

void ValueProperty::DetachPrototypeProperty(const ValueProperty* property)
{
    if (prototypeProperty == property)
    {
        prototypeProperty = nullptr;
    }
    else
    {
        DVASSERT(false);
    }
}

const ValueProperty* ValueProperty::GetPrototypeProperty() const
{
    return prototypeProperty;
}

AbstractProperty* ValueProperty::FindPropertyByPrototype(AbstractProperty* prototype)
{
    return prototype == prototypeProperty ? this : nullptr;
}

bool ValueProperty::HasChanges() const
{
    return IsOverriddenLocally();
}

const String& ValueProperty::GetName() const
{
    return name;
}

const DAVA::Type* ValueProperty::GetValueType() const
{
    return valueType;
}

void ValueProperty::SetValue(const Any& newValue)
{
    overridden = forceOverride || !IsEqual(defaultValue, newValue);
    ApplyValue(newValue);
}

Any ValueProperty::GetDefaultValue() const
{
    return defaultValue;
}

void ValueProperty::SetDefaultValue(const Any& newValue)
{
    const Type* valueType = GetValueType();
    DVASSERT(newValue.GetType() == valueType);

    defaultValue = newValue;
    if (!overridden)
        ApplyValue(newValue);
}

void ValueProperty::ResetValue()
{
    forceOverride = false;
    overridden = false;
    ApplyValue(defaultValue);
}

bool ValueProperty::IsOverridden() const
{
    bool overriddenLocally = IsOverriddenLocally();
    if (overriddenLocally || prototypeProperty == nullptr)
        return overriddenLocally;

    return prototypeProperty->IsOverridden();
}

bool ValueProperty::IsOverriddenLocally() const
{
    return overridden;
}

bool ValueProperty::IsForceOverride() const
{
    return forceOverride;
}

void ValueProperty::SetForceOverride(bool forceOverride_)
{
    forceOverride = forceOverride_;
    overridden = forceOverride || !IsEqual(defaultValue, GetValue());
}

const Type* ValueProperty::GetSubValueType(int32 index) const
{
    return GetValueTypeComponent(index);
}

Any ValueProperty::GetSubValue(int32 index) const
{
    return GetValueComponent(GetValue(), index);
}

void ValueProperty::SetSubValue(int32 index, const Any& newValue)
{
    SetValue(ChangeValueComponent(GetValue(), newValue, index));
}

Any ValueProperty::GetDefaultSubValue(int32 index) const
{
    return GetValueComponent(defaultValue, index);
}

void ValueProperty::SetDefaultSubValue(int32 index, const Any& newValue)
{
    SetDefaultValue(ChangeValueComponent(defaultValue, newValue, index));
}

void ValueProperty::GenerateBuiltInSubProperties()
{
    const Vector<String>* componentNames = nullptr;
    Vector<SubValueProperty*> subProperties;
    const Type* valueType = GetValueType();
    if (valueType == Type::Instance<Vector2>())
    {
        componentNames = &SValueProperty::VECTOR2_COMPONENT_NAMES;
    }
    else if (valueType == Type::Instance<Color>())
    {
        componentNames = &SValueProperty::COLOR_COMPONENT_NAMES;
    }
    else if (valueType == Type::Instance<Vector4>())
    {
        componentNames = &SValueProperty::MARGINS_COMPONENT_NAMES;
    }
    else if (GetType() == TYPE_FLAGS)
    {
        const EnumMap* map = GetEnumMap();
        if (map != nullptr)
        {
            for (size_type i = 0; i < map->GetCount(); ++i)
            {
                int val = 0;
                const bool getSucceeded = map->GetValue(i, val);
                DVASSERT(getSucceeded);
                subProperties.push_back(new SubValueProperty(static_cast<int32>(i), map->ToString(val)));
            }
        }
    }

    if (componentNames != nullptr)
    {
        for (size_type i = 0; i < componentNames->size(); ++i)
            subProperties.push_back(new SubValueProperty(static_cast<int32>(i), componentNames->at(i)));
    }

    for (SubValueProperty* prop : subProperties)
    {
        prop->SetParent(this);
        AddSubValueProperty(prop);
        SafeRelease(prop);
    }
    subProperties.clear();
}

int32 ValueProperty::GetStylePropertyIndex() const
{
    return stylePropertyIndex;
}

void ValueProperty::ApplyValue(const Any& value)
{
}

void ValueProperty::SetName(const String& newName)
{
    name = newName;
}

void ValueProperty::SetOverridden(bool anOverridden)
{
    overridden = anOverridden;
}

void ValueProperty::SetStylePropertyIndex(int32 index)
{
    stylePropertyIndex = index;
}

void ValueProperty::AddSubValueProperty(AbstractProperty* prop)
{
    children.push_back(RefPtr<AbstractProperty>(SafeRetain(prop)));
}

Any ValueProperty::ChangeValueComponent(const Any& value, const Any& component, int32 index) const
{
    const Type* valueType = GetValueType();
    DVASSERT(defaultValue.GetType() == valueType);

    if (valueType == Type::Instance<Vector2>())
    {
        Vector2 val = value.Get<Vector2>();
        if (index == 0)
            val.x = component.Get<float32>();
        else
            val.y = component.Get<float32>();

        return val;
    }

    if (valueType == Type::Instance<Color>())
    {
        Color val = value.Get<Color>();
        if (0 <= index && index < 4)
        {
            val.color[index] = component.Get<float32>();
        }
        else
        {
            DVASSERT(false);
        }

        return val;
    }

    if (valueType == Type::Instance<Vector4>())
    {
        Vector4 val = value.Get<Vector4>();
        if (0 <= index && index < 4)
        {
            val.data[index] = component.Get<float32>();
        }
        else
        {
            DVASSERT(false);
        }
        return Any(val);
    }

    if (GetType() == TYPE_FLAGS)
    {
        const EnumMap* map = GetEnumMap();
        int32 intValue = value.Get<int32>();

        int val = 0;
        map->GetValue(index, val);
        if (component.Get<bool>())
            return Any(intValue | val);
        else
            return Any(intValue & (~val));
    }

    DVASSERT(false);
    return Any();
}

const Type* ValueProperty::GetValueTypeComponent(int32 index) const
{
    const Type* valueType = GetValueType();
    DVASSERT(defaultValue.GetType() == valueType);

    if (valueType == Type::Instance<Vector2>())
    {
        DVASSERT(index >= 0 && index < 2);
        return Type::Instance<float32>();
    }

    if (valueType == Type::Instance<Color>())
    {
        DVASSERT(index >= 0 && index < 4);
        return Type::Instance<float32>();
    }

    if (valueType == Type::Instance<Vector4>())
    {
        DVASSERT(index >= 0 && index < 4);
        return Type::Instance<float32>();
    }

    if (GetType() == TYPE_FLAGS)
    {
        return Type::Instance<bool>();
    }

    DVASSERT(false);
    return nullptr;
}

Any ValueProperty::GetValueComponent(const Any& value, int32 index) const
{
    const Type* valueType = GetValueType();
    DVASSERT(defaultValue.GetType() == valueType);

    if (valueType == Type::Instance<Vector2>())
    {
        DVASSERT(index >= 0 && index < 2);
        return Any(value.Get<Vector2>().data[index]);
    }

    if (valueType == Type::Instance<Color>())
    {
        DVASSERT(index >= 0 && index < 4);
        return Any(value.Get<Color>().color[index]);
    }

    if (valueType == Type::Instance<Vector4>())
    {
        DVASSERT(index >= 0 && index < 4);
        return Any(value.Get<Vector4>().data[index]);
    }

    if (GetType() == TYPE_FLAGS)
    {
        const EnumMap* map = GetEnumMap();
        int val = 0;
        map->GetValue(index, val);
        return Any((value.Get<int32>() & val) != 0);
    }
    else
    {
        DVASSERT(false);
        return Any();
    }

    DVASSERT(false);
    return Any();
}

bool ValueProperty::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2) const
{
    if (v1 == v2)
    {
        return true;
    }

    if (v1.IsEmpty() || v2.IsEmpty())
    {
        return false;
    }

    if ((v1.GetType()->IsEnum() || v1.CanGet<int32>()) && (v2.GetType()->IsEnum() || v2.CanGet<int32>()))
    {
        return v1.Cast<int32>() == v2.Cast<int32>();
    }

    return false;
}
