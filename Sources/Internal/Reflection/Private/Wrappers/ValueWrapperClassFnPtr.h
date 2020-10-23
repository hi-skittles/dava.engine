#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

#include "Reflection/Private/Wrappers/ValueWrapperDefault.h"

namespace DAVA
{
template <typename C, typename GetT, typename SetT>
class ValueWrapperClassFnPtr : public ValueWrapper
{
    using Getter = GetT (C::*)();
    using Setter = void (C::*)(SetT);

public:
    ValueWrapperClassFnPtr(Getter getter_, Setter setter_ = nullptr)
        : ValueWrapper()
        , getter(getter_)
        , setter(setter_)
    {
    }

    bool IsReadonly(const ReflectedObject& object) const override
    {
        return (nullptr == setter || object.IsConst());
    }

    const Type* GetType(const ReflectedObject& object) const override
    {
        return Type::Instance<GetT>();
    }

    Any GetValue(const ReflectedObject& object) const override
    {
        using UnrefGetT = typename std::remove_reference<GetT>::type;

        C* cls = object.GetPtr<C>();
        UnrefGetT v = (cls->*getter)();

        return Any(std::move(v));
    }

    bool SetValue(const ReflectedObject& object, const Any& value) const override
    {
        using UnrefSetT = typename std::decay<SetT>::type;

        if (!IsReadonly(object))
        {
            C* cls = object.GetPtr<C>();

            const SetT& v = value.Get<UnrefSetT>();
            (cls->*setter)(v);

            return true;
        }

        return false;
    }

    inline bool SetValueWithCast(const ReflectedObject& object, const Any& value) const override
    {
        using UnrefSetT = typename std::decay<SetT>::type;

        if (value.CanCast<UnrefSetT>())
        {
            return SetValue(object, value.Cast<UnrefSetT>());
        }

        return false;
    }

    ReflectedObject GetValueObject(const ReflectedObject& object) const override
    {
        auto is_pointer = std::integral_constant<bool, std::is_pointer<GetT>::value>();
        auto is_reference = std::integral_constant<bool, std::is_reference<GetT>::value>();

        return GetValueObjectImpl(object, is_pointer, is_reference);
    }

protected:
    Getter getter = nullptr;
    Setter setter = nullptr;

private:
    inline ReflectedObject GetValueObjectImpl(const ReflectedObject& object, std::false_type /* is_pointer */, std::false_type /* is_reference */) const
    {
        return ReflectedObject();
    }

    inline ReflectedObject GetValueObjectImpl(const ReflectedObject& object, std::true_type /* is_pointer */, std::false_type /* is_reference */) const
    {
        C* cls = object.GetPtr<C>();
        return ReflectedObject((cls->*getter)());
    }

    inline ReflectedObject GetValueObjectImpl(const ReflectedObject& object, std::false_type /* is_pointer */, std::true_type /* is_reference */) const
    {
        C* cls = object.GetPtr<C>();
        GetT v = (cls->*getter)();
        return ReflectedObject(&v);
    }
};

} // namespace DAVA
