#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

namespace DAVA
{
class ValueWrapperObject : public ValueWrapper
{
public:
    ValueWrapperObject()
    {
    }

    bool IsReadonly(const ReflectedObject& object) const override
    {
        return object.IsConst();
    }

    const Type* GetType(const ReflectedObject& object) const override
    {
        const Type* objType = object.GetReflectedType()->GetType();
        return (nullptr != objType) ? objType->Pointer() : nullptr;
    }

    Any GetValue(const ReflectedObject& object) const override
    {
        Any ret;
        void* objPtr = object.GetVoidPtr();
        ret.LoadData(&objPtr, GetType(object));
        return ret;
    }

    bool SetValue(const ReflectedObject& object, const Any& value) const override
    {
        return false;
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
