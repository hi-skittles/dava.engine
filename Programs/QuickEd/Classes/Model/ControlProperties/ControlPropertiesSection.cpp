#include "ControlPropertiesSection.h"

#include "PropertyVisitor.h"
#include "ValueProperty.h"
#include "IntrospectionProperty.h"

#include <Reflection/ReflectedMeta.h>
#include <UI/UIControl.h>

#include <Reflection/ReflectedTypeDB.h>

using namespace DAVA;

ControlPropertiesSection::ControlPropertiesSection(const DAVA::String& name, DAVA::UIControl* control_, const DAVA::Type* type, const Vector<Reflection::Field>& fields, const ControlPropertiesSection* prototypeProperty)
    : SectionProperty(name)
    , control(SafeRetain(control_))
{
    for (const Reflection::Field& field : fields)
    {
        if (field.ref.GetMeta<M::HiddenField>() != nullptr)
        {
            continue;
        }

        if (field.inheritFrom->GetType() == type)
        {
            String name = field.key.Cast<String>();
            IntrospectionProperty* sourceProperty = nullptr == prototypeProperty ? nullptr : prototypeProperty->FindChildPropertyByName(name);
            IntrospectionProperty* prop = IntrospectionProperty::Create(control, nullptr, name, field.ref, sourceProperty);
            AddProperty(prop);
            SafeRelease(prop);
        }
    }

    const ReflectedType* refType = ReflectedTypeDB::GetByType(type);
    const ReflectedStructure* structure = refType->GetStructure();
    if (structure && structure->meta)
    {
        displayName = structure->meta->GetMeta<M::DisplayName>()->displayName;
    }
    else
    {
        displayName = refType->GetPermanentName();
    }
}

ControlPropertiesSection::~ControlPropertiesSection()
{
    SafeRelease(control);
}

void ControlPropertiesSection::Accept(PropertyVisitor* visitor)
{
    visitor->VisitControlSection(this);
}

const DAVA::String& ControlPropertiesSection::GetDisplayName() const
{
    return displayName;
}
