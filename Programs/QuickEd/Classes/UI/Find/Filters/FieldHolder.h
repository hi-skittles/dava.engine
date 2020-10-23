#pragma once

#include "Classes/UI/Find/PackageInformation/ControlInformation.h"

#include <Reflection/ReflectedTypeDB.h>

#include <UI/Components/UIComponent.h>
#include <UI/Components/UIComponentUtils.h>

template <typename Component, typename T>
class FieldHolder
{
public:
    FieldHolder(const char* nameStr)
    {
        field = FindField(nameStr);
    }

    T Get(const ControlInformation* control, T defaultValue) const
    {
        using namespace DAVA;

        const Type* componentType = Type::Instance<Component>();
        DVASSERT(UIComponentUtils::IsMultiple(componentType) == false);
        Any value = control->GetComponentPropertyValue(componentType, 0, *field);
        if (value.CanCast<T>())
        {
            return value.Cast<T>();
        }
        return defaultValue;
    }

private:
    const DAVA::ReflectedStructure::Field* FindField(const char* nameStr)
    {
        const DAVA::FastName name(nameStr);
        for (const auto& field : DAVA::ReflectedTypeDB::Get<Component>()->GetStructure()->fields)
        {
            if (field->name == name)
            {
                return field.get();
            }
        }
        DVASSERT("can not find field in component!");
        return nullptr;
    }

    const DAVA::ReflectedStructure::Field* field = nullptr;
};
