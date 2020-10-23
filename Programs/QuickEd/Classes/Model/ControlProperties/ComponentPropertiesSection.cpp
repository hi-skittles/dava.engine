#include "ComponentPropertiesSection.h"

#include "IntrospectionProperty.h"
#include "PropertyVisitor.h"

#include <UI/UIControl.h>
#include <UI/Components/UIComponentUtils.h>
#include <UI/Layouts/UILayoutIsolationComponent.h>
#include <UI/Layouts/UILayoutSourceRectComponent.h>
#include <UI/Scene3D/UISceneComponent.h>
#include <UI/Scroll/UIScrollComponent.h>
#include <Utils/StringFormat.h>
#include <Reflection/ReflectedMeta.h>
#include <Reflection/ReflectedTypeDB.h>

#include <TArc/Utils/ReflectionHelpers.h>

using namespace DAVA;

ComponentPropertiesSection::ComponentPropertiesSection(DAVA::UIControl* control_, const DAVA::Type* type_, int32 index_, const ComponentPropertiesSection* prototypeSection_)
    : SectionProperty("")
    , control(SafeRetain(control_))
    , component(nullptr)
    , index(index_)
    , prototypeSection(prototypeSection_) // weak
{
    component = control->GetComponent(type_, index_);
    if (component != nullptr)
    {
        SafeRetain(component);
    }
    else
    {
        component = UIComponent::CreateByType(type_);
        componentWasCreated = true;
    }
    DVASSERT(component);

    name = GetComponentName();
    RefreshName();

    Reflection componentRef = Reflection::Create(&component);
    Vector<Reflection::Field> fields = componentRef.GetFields();
    for (const Reflection::Field& field : fields)
    {
        if (!field.ref.IsReadonly() && nullptr == field.ref.GetMeta<DAVA::M::ReadOnly>() && nullptr == field.ref.GetMeta<DAVA::M::HiddenField>())
        {
            String name = field.key.Get<FastName>().c_str();
            const IntrospectionProperty* sourceProp = prototypeSection == nullptr ? nullptr : prototypeSection->FindChildPropertyByName(name);
            IntrospectionProperty* prop = IntrospectionProperty::Create(component, type_, name.c_str(), field.ref, sourceProp);
            AddProperty(prop);
            SafeRelease(prop);
        }
    }
}

ComponentPropertiesSection::~ComponentPropertiesSection()
{
    SafeRelease(control);
    SafeRelease(component);
    prototypeSection = nullptr; // weak
}

UIComponent* ComponentPropertiesSection::GetComponent() const
{
    return component;
}

const DAVA::Type* ComponentPropertiesSection::GetComponentType() const
{
    return component->GetType();
}

const DAVA::String& ComponentPropertiesSection::GetDisplayName() const
{
    return displayName;
}

void ComponentPropertiesSection::AttachPrototypeSection(ComponentPropertiesSection* section)
{
    if (prototypeSection == nullptr)
    {
        prototypeSection = section;

        Reflection componentRef = Reflection::Create(&component);
        Vector<Reflection::Field> fields = componentRef.GetFields();

        for (const Reflection::Field& field : fields)
        {
            String name = field.key.Cast<String>();
            ValueProperty* value = FindChildPropertyByName(name);
            ValueProperty* prototypeValue = prototypeSection->FindChildPropertyByName(name);
            if (value != nullptr && prototypeValue != nullptr)
            {
                value->AttachPrototypeProperty(prototypeValue);
            }
            else
            {
                DVASSERT(value == nullptr && prototypeValue == nullptr);
            }
        }
    }
    else
    {
        DVASSERT(false);
    }
}

void ComponentPropertiesSection::DetachPrototypeSection(ComponentPropertiesSection* section)
{
    if (prototypeSection == section)
    {
        prototypeSection = nullptr; // weak
        for (uint32 i = 0; i < GetCount(); i++)
        {
            ValueProperty* value = GetProperty(i);
            if (value->GetPrototypeProperty())
            {
                DVASSERT(value->GetPrototypeProperty()->GetParent() == section);
                value->DetachPrototypeProperty(value->GetPrototypeProperty());
            }
            else
            {
                DVASSERT(false);
            }
        }
    }
    else
    {
        DVASSERT(false);
    }
}

bool ComponentPropertiesSection::HasChanges() const
{
    return SectionProperty::HasChanges() || ((GetFlags() & AbstractProperty::EF_INHERITED) == 0 && componentWasCreated);
}

uint32 ComponentPropertiesSection::GetFlags() const
{
    bool readOnly = IsReadOnly();

    uint32 flags = 0;

    if (!readOnly && prototypeSection == nullptr && componentWasCreated)
    {
        flags |= EF_CAN_REMOVE;
    }

    if (prototypeSection)
    {
        flags |= EF_INHERITED;
    }

    return flags;
}

void ComponentPropertiesSection::InstallComponent()
{
    if (componentWasCreated)
    {
        if (control->GetComponent(component->GetType(), 0) != component)
        {
            control->InsertComponentAt(component, index);
        }

        for (uint32 i = 0; i < GetCount(); i++)
        {
            GetProperty(i)->ComponentWithPropertyWasInstalled();
        }
    }
}

void ComponentPropertiesSection::UninstallComponent()
{
    if (componentWasCreated)
    {
        for (uint32 i = 0; i < GetCount(); i++)
        {
            GetProperty(i)->ComponentWithPropertyWasUninstalled();
        }

        UIComponent* installedComponent = control->GetComponent(component->GetType(), index);
        if (installedComponent)
        {
            DVASSERT(installedComponent == component);
            control->RemoveComponent(component);
        }
    }
}

int32 ComponentPropertiesSection::GetComponentIndex() const
{
    return index;
}

void ComponentPropertiesSection::RefreshIndex()
{
    if (component->GetControl() == control)
    {
        index = control->GetComponentIndex(component);
        RefreshName();
    }
}

void ComponentPropertiesSection::Accept(PropertyVisitor* visitor)
{
    visitor->VisitComponentSection(this);
}

String ComponentPropertiesSection::GetComponentName() const
{
    return ReflectedTypeDB::GetByType(component->GetType())->GetPermanentName();
}

void ComponentPropertiesSection::RefreshName()
{
    displayName = UIComponentUtils::GetDisplayName(component->GetType());
    if (UIComponentUtils::IsMultiple(component->GetType()))
        displayName += Format(" [%d]", index);
}
