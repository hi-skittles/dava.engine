#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

namespace DAVA
{
class ValueWrapperDirect : public ValueWrapper
{
public:
    ValueWrapperDirect()
    {
    }

    bool IsReadonly(const ReflectedObject& object) const override
    {
        return object.IsConst();
    }

    const Type* GetType(const ReflectedObject& object) const override
    {
        return object.GetReflectedType()->GetType();
    }

    Any GetValue(const ReflectedObject& object) const override
    {
        Any ret;

        if (!IsReadonly(object))
        {
            void* ptr = object.GetVoidPtr();
            ret.LoadData(ptr, object.GetReflectedType()->GetType());
        }

        return ret;
    }

    bool SetValue(const ReflectedObject& object, const Any& value) const override
    {
        bool ret = false;

        if (!IsReadonly(object) && object.IsValid())
        {
            void* ptr = object.GetVoidPtr();
            const Type* inType = object.GetReflectedType()->GetType();
            ret = value.StoreData(ptr, inType->GetSize());
        }

        return ret;
    }

    bool SetValueWithCast(const ReflectedObject& object, const Any& value) const override
    {
        return false;
    }

    ReflectedObject GetValueObject(const ReflectedObject& object) const override
    {
        return object;
    }
};

} // namespace DAVA
