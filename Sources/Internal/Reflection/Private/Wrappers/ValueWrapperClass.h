#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

#include "Reflection/Private/Wrappers/ValueWrapperDefault.h"

namespace DAVA
{
template <typename C, typename T>
class ValueWrapperClass : public ValueWrapperDefault<T>
{
public:
    ValueWrapperClass(T C::*field_)
        : field(field_)
    {
    }

    inline Any GetValue(const ReflectedObject& object) const override
    {
        Any ret;

        C* cls = object.GetPtr<C>();
        ret.Set(cls->*field);

        return ret;
    }

    inline bool SetValue(const ReflectedObject& object, const Any& value) const override
    {
        C* cls = object.GetPtr<C>();
        T* ptr = &(cls->*field);

        return ValueWrapperDefault<T>::SetValueInternal(ptr, value);
    }

    inline bool SetValueWithCast(const ReflectedObject& object, const Any& value) const override
    {
        if (value.CanCast<T>())
        {
            return SetValue(object, value.Cast<T>());
        }

        return false;
    }

    inline ReflectedObject GetValueObject(const ReflectedObject& object) const override
    {
        C* cls = object.GetPtr<C>();
        T* ptr = &(cls->*field);

        return ReflectedObject(ptr);
    }

protected:
    T C::*field;
};

} // namespace DAVA
