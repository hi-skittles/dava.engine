#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

namespace DAVA
{
class ValueWrapperAny : public ValueWrapper
{
public:
    ValueWrapperAny()
    {
    }

    bool IsReadonly(const ReflectedObject& object) const override
    {
        return true;
    }

    const Type* GetType(const ReflectedObject& object) const override
    {
        Any* anyPtr = object.GetPtr<Any>();
        return anyPtr->GetType();
    }

    Any GetValue(const ReflectedObject& object) const override
    {
        Any* anyPtr = object.GetPtr<Any>();
        return *anyPtr;
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
