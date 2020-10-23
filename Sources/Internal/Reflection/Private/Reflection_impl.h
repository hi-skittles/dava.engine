#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

#include "Reflection/Private/Reflection_pre_impl.h"
#include "Reflection/Private/Wrappers/ValueWrapperDefault.h"
#include "Reflection/Private/Wrappers/ValueWrapperObject.h"

namespace DAVA
{
namespace ReflectionDetail
{
}

inline bool Reflection::IsReadonly() const
{
    return valueWrapper->IsReadonly(object);
}

inline const Type* Reflection::GetValueType() const
{
    return valueWrapper->GetType(object);
}

inline ReflectedObject Reflection::GetValueObject() const
{
    return valueWrapper->GetValueObject(object);
}

inline ReflectedObject Reflection::GetDirectObject() const
{
    return object;
}

inline Any Reflection::GetValue() const
{
    return valueWrapper->GetValue(object);
}

inline bool Reflection::SetValue(const Any& value) const
{
    return valueWrapper->SetValue(object, value);
}

inline bool Reflection::SetValueWithCast(const Any& value) const
{
    return valueWrapper->SetValueWithCast(object, value);
}
inline bool Reflection::IsValid() const
{
    return (nullptr != valueWrapper && object.IsValid());
}

template <typename T>
inline const T* Reflection::GetMeta() const
{
    return static_cast<const T*>(GetMeta(Type::Instance<T>()));
}

template <typename T>
Reflection Reflection::Create(T* objectPtr, const ReflectedMeta* objectMeta)
{
    if (nullptr != objectPtr)
    {
        static ValueWrapperDefault<T> objectValueWrapper;
        return Reflection(ReflectedObject(objectPtr), &objectValueWrapper, nullptr, objectMeta);
    }

    return Reflection();
}

template <>
struct AnyCompare<Reflection>
{
    static bool IsEqual(const Any& v1, const Any& v2)
    {
        const Reflection r1 = v1.Get<Reflection>();
        const Reflection r2 = v2.Get<Reflection>();
        return r1.GetValueObject() == r2.GetValueObject();
    }
};

template <>
struct AnyCompare<Vector<Reflection>>
{
    static bool IsEqual(const Any& v1, const Any& v2)
    {
        const Vector<Reflection> r1 = v1.Get<Vector<Reflection>>();
        const Vector<Reflection> r2 = v2.Get<Vector<Reflection>>();
        if (r1.size() != r2.size())
        {
            return false;
        }

        for (size_t i = 0; i < r1.size(); ++i)
        {
            if (AnyCompare<Reflection>::IsEqual(r1[i], r2[i]) == false)
            {
                return false;
            }
        }
        return true;
    }
};
} // namespace DAVA
