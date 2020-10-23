#include "AbstractProperty.h"
#include "Reflection/ReflectionRegistrator.h"

using namespace DAVA;

DAVA_VIRTUAL_REFLECTION_IMPL(AbstractProperty)
{
    ReflectionRegistrator<AbstractProperty>::Begin()
    .End();
}

AbstractProperty::AbstractProperty()
    : parent(NULL)
{
}

AbstractProperty::~AbstractProperty()
{
}

AbstractProperty* AbstractProperty::GetParent() const
{
    return parent;
}

void AbstractProperty::SetParent(AbstractProperty* parent)
{
    this->parent = parent;
}

int32 AbstractProperty::GetIndex(const AbstractProperty* property) const
{
    for (uint32 i = 0; i < GetCount(); i++)
    {
        if (GetProperty(i) == property)
            return static_cast<int32>(i);
    }
    return -1;
}

void AbstractProperty::Refresh(int32 refreshFlags)
{
}

AbstractProperty* AbstractProperty::FindPropertyByPrototype(AbstractProperty* prototype)
{
    for (uint32 i = 0; i < GetCount(); i++)
    {
        AbstractProperty* result = GetProperty(i)->FindPropertyByPrototype(prototype);
        if (result)
            return result;
    }
    return nullptr;
}

AbstractProperty* AbstractProperty::FindPropertyByStyleIndex(DAVA::int32 propertyIndex) const
{
    for (uint32 i = 0; i < GetCount(); i++)
    {
        AbstractProperty* child = GetProperty(i);
        if (child->GetStylePropertyIndex() == propertyIndex)
        {
            return child;
        }

        AbstractProperty* result = child->FindPropertyByStyleIndex(propertyIndex);
        if (result)
        {
            return result;
        }
    }

    return nullptr;
}

bool AbstractProperty::HasChanges() const
{
    for (uint32 i = 0; i < GetCount(); i++)
    {
        if (GetProperty(i)->HasChanges())
            return true;
    }
    return false;
}

const DAVA::String& AbstractProperty::GetDisplayName() const
{
    return GetName();
}

uint32 AbstractProperty::GetFlags() const
{
    return EF_NONE;
}

int32 AbstractProperty::GetStylePropertyIndex() const
{
    return -1;
}

bool AbstractProperty::IsReadOnly() const
{
    return parent ? parent->IsReadOnly() : true;
}

Any AbstractProperty::GetValue() const
{
    return Any();
}

DAVA::Any AbstractProperty::GetSerializationValue() const
{
    return GetValue();
}

void AbstractProperty::SetValue(const Any& /*newValue*/)
{
    // Do nothing by default
}

Any AbstractProperty::GetDefaultValue() const
{
    return Any();
}

void AbstractProperty::SetDefaultValue(const Any& newValue)
{
    // Do nothing by default
}

const EnumMap* AbstractProperty::GetEnumMap() const
{
    return nullptr;
}

void AbstractProperty::ResetValue()
{
    // Do nothing by default
}

bool AbstractProperty::IsOverridden() const
{
    return false; // false by default
}

bool AbstractProperty::IsBindable() const
{
    return false;
}

bool AbstractProperty::IsBound() const
{
    return false;
}

bool AbstractProperty::HasError() const
{
    return false;
}

String AbstractProperty::GetErrorString() const
{
    return "";
}

DAVA::int32 AbstractProperty::GetBindingUpdateMode() const
{
    return 0; // do nothing by default
}

DAVA::String AbstractProperty::GetBindingExpression() const
{
    return "";
}

void AbstractProperty::SetBindingExpression(const DAVA::String& expression, DAVA::int32 bindingUpdateMode)
{
    // do nothing by default
}

bool AbstractProperty::IsOverriddenLocally() const
{
    return false; // false by default
}

AbstractProperty* AbstractProperty::GetRootProperty()
{
    AbstractProperty* property = this;
    while (property->parent)
        property = property->parent;
    return property;
}

const AbstractProperty* AbstractProperty::GetRootProperty() const
{
    const AbstractProperty* property = this;
    while (property->parent)
        property = property->parent;
    return property;
}

AbstractProperty* AbstractProperty::FindPropertyByName(const String& name)
{
    if (GetName() == name)
    {
        return this;
    }
    for (DAVA::uint32 index = 0, count = GetCount(); index < count; ++index)
    {
        AbstractProperty* child = GetProperty(index);
        AbstractProperty* property = child->FindPropertyByName(name);
        if (property != nullptr)
        {
            return property;
        }
    }
    return nullptr;
}
