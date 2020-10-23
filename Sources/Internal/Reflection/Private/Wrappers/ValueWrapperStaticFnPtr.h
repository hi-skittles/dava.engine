#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

namespace DAVA
{
template <typename GetT, typename SetT>
class ValueWrapperStaticFnPtr : public ValueWrapper
{
    using Getter = GetT (*)();
    using Setter = void (*)(SetT);

public:
    ValueWrapperStaticFnPtr(Getter getter_, Setter setter_ = nullptr)
        : ValueWrapper()
        , getter(getter_)
        , setter(setter_)
    {
    }

    bool IsReadonly(const ReflectedObject& object) const override
    {
        return (nullptr == setter);
    }

    const Type* GetType(const ReflectedObject& object) const override
    {
        return Type::Instance<GetT>();
    }

    Any GetValue(const ReflectedObject& object) const override
    {
        using UnrefGetT = typename std::remove_reference<GetT>::type;

        UnrefGetT v = (*getter)();

        return Any(std::move(v));
    }

    bool SetValue(const ReflectedObject& object, const Any& value) const override
    {
        using UnrefSetT = typename std::remove_reference<SetT>::type;

        if (nullptr != setter)
        {
            const SetT& v = value.Get<UnrefSetT>();
            (*setter)(v);

            return true;
        }

        return false;
    }

    inline bool SetValueWithCast(const ReflectedObject& object, const Any& value) const override
    {
        using UnrefSetT = typename std::remove_reference<SetT>::type;

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
    inline ReflectedObject GetValueObjectImpl(const ReflectedObject& ptr, std::false_type /* is_pointer */, std::false_type /* is_reference */) const
    {
        return ReflectedObject();
    }

    inline ReflectedObject GetValueObjectImpl(const ReflectedObject& ptr, std::true_type /* is_pointer */, std::false_type /* is_reference */) const
    {
        return ReflectedObject((*getter)());
    }

    inline ReflectedObject GetValueObjectImpl(const ReflectedObject& ptr, std::false_type /* is_pointer */, std::true_type /* is_reference */) const
    {
        GetT v = (*getter)();
        return ReflectedObject(&v);
    }
};

} // namespace DAVA
