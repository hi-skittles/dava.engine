#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

#include "Reflection/Private/Wrappers/ValueWrapperDefault.h"

namespace DAVA
{
template <typename T>
class ValueWrapperStatic : public ValueWrapperDefault<T>
{
public:
    ValueWrapperStatic(T* field_)
        : field(field_)
    {
    }

    bool IsReadonly(const ReflectedObject& object) const override
    {
        return ValueWrapperDefault<T>::isConst;
    }

    Any GetValue(const ReflectedObject& object) const override
    {
        return Any(*field);
    }

    bool SetValue(const ReflectedObject& object, const Any& value) const override
    {
        return ValueWrapperDefault<T>::SetValueInternal(field, value);
    }

    inline bool SetValueWithCast(const ReflectedObject& object, const Any& value) const override
    {
        if (value.CanCast<T>())
        {
            return SetValue(object, value.Cast<T>());
        }

        return false;
    }

    ReflectedObject GetValueObject(const ReflectedObject& object) const override
    {
        return ReflectedObject(field);
    }

protected:
    T* field;
};

} // namespace DAVA
