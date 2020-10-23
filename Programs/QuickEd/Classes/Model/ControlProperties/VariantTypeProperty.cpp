#include "VariantTypeProperty.h"

#include "Model/ControlProperties/PropertyVisitor.h"
#include "Model/ControlProperties/IntrospectionProperty.h"
#include "Model/ControlProperties/SubValueProperty.h"
#include "Model/PackageHierarchy/StyleSheetNode.h"

#include <UI/Styles/UIStyleSheet.h>
#include <Reflection/ReflectedMeta.h>

using namespace DAVA;

VariantTypeProperty::VariantTypeProperty(const String& name, Any& vt, const ReflectedStructure::Field* field)
    : ValueProperty(name, vt.GetType())
    , value(vt)
    , field(field)
{
    SetOverridden(true);
    SetDefaultValue(vt);

    GenerateBuiltInSubProperties();
}

VariantTypeProperty::~VariantTypeProperty()
{
}

void VariantTypeProperty::Accept(PropertyVisitor* visitor)
{
    // do nothing
}

bool VariantTypeProperty::IsReadOnly() const
{
    return GetParent() == nullptr ? true : GetParent()->IsReadOnly();
}

VariantTypeProperty::ePropertyType VariantTypeProperty::GetType() const
{
    if (field != nullptr && field->meta != nullptr)
    {
        const M::Enum* enumMeta = field->meta->GetMeta<M::Enum>();
        if (enumMeta != nullptr)
        {
            return TYPE_ENUM;
        }

        const M::Flags* flagsMeta = field->meta->GetMeta<M::Flags>();
        if (flagsMeta != nullptr)
        {
            return TYPE_FLAGS;
        }
    }
    return TYPE_VARIANT;
}

const EnumMap* VariantTypeProperty::GetEnumMap() const
{
    if (field != nullptr && field->meta != nullptr)
    {
        const M::Enum* enumMeta = field->meta->GetMeta<M::Enum>();
        if (enumMeta != nullptr)
        {
            return enumMeta->GetEnumMap();
        }

        const M::Flags* flagsMeta = field->meta->GetMeta<M::Flags>();
        if (flagsMeta != nullptr)
        {
            return flagsMeta->GetFlagsMap();
        }
    }
    return nullptr;
}

Any VariantTypeProperty::GetValue() const
{
    return value;
}

void VariantTypeProperty::ApplyValue(const DAVA::Any& newValue)
{
    value = newValue;
}
