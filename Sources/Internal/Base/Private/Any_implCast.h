#pragma once

#ifndef __Dava_Any__
#include "Base/Any.h"
#endif

#include "Base/UnordererMap.h"

namespace DAVA
{
namespace AnyDetail
{
enum DummyEnum
{
};

template <typename T, bool>
struct EnumHelper
{
    static inline bool CanCast(const Any& any)
    {
        return false;
    }

    static inline T Cast(const Any& any)
    {
        DAVA_THROW(Exception, "Any:: can't be casted into specified enum T");
    }
};

template <typename T>
struct EnumHelper<T, true>
{
    static bool CanCast(const Any& any)
    {
        if (std::is_enum<T>::value)
        {
            return any.GetType() == Type::Instance<int>();
        }
        else
        {
            const Type* t = any.GetType();
            return (t->IsEnum() && sizeof(DummyEnum) == t->GetSize());
        }
    }

    static T Cast(const Any& any)
    {
        if (std::is_enum<T>::value)
        {
            return static_cast<T>(any.Get<int>());
        }
        else
        {
            return *static_cast<const T*>(any.GetData());
        }
    }
};

template <typename T>
struct AnyCastHolder
{
    using CastFn = T (*)(const Any&);

    static UnorderedMap<const Type*, CastFn> castFns;

    static CastFn GetCastFn(const Type* fromType)
    {
        auto it = castFns.find(fromType);
        if (it != castFns.end())
        {
            return (*it).second;
        }

        return nullptr;
    }
};

template <typename T>
UnorderedMap<const Type*, typename AnyCastHolder<T>::CastFn> AnyCastHolder<T>::castFns;

template <typename T>
struct AnyCastImpl
{
    using EnumHelperT = EnumHelper<T, std::is_enum<T>::value || std::is_same<T, int>::value>;

    static bool CanCast(const Any& any)
    {
        return (nullptr != AnyCastHolder<T>::GetCastFn(any.GetType()) || EnumHelperT::CanCast(any));
    }

    static T Cast(const Any& any)
    {
        auto fn = AnyCastHolder<T>::GetCastFn(any.GetType());

        if (nullptr != fn)
        {
            return (*fn)(any);
        }

        if (EnumHelperT::CanCast(any))
        {
            return EnumHelperT::Cast(any);
        }

        DAVA_THROW(Exception, "Any:: can't be casted into specified T");
    }

    template <typename U>
    static T Cast(const Any& any, const U& def)
    {
        auto fn = AnyCastHolder<T>::GetCastFn(any.GetType());

        if (nullptr != fn)
        {
            return (*fn)(any);
        }

        if (EnumHelperT::CanCast(any))
        {
            return EnumHelperT::Cast(any);
        }

        return static_cast<T>(def);
    }
};

template <typename T>
struct AnyCastImpl<T*>
{
    static bool CanCast(const Any& any)
    {
        using P = Type::DecayT<T*>;
        return TypeInheritance::CanCast(any.GetType(), Type::Instance<P>());
    }

    static T* Cast(const Any& any)
    {
        using P = Type::DecayT<T*>;

        void* inPtr = any.Get<void*>();
        void* outPtr = nullptr;

        if (TypeInheritance::Cast(any.GetType(), Type::Instance<P>(), inPtr, &outPtr))
        {
            return static_cast<T*>(outPtr);
        }

        DAVA_THROW(Exception, "Any:: can't be casted into specified T*");
    }

    template <typename U>
    static T* Cast(const Any& any, const U& def)
    {
        using P = Type::DecayT<T*>;

        void* inPtr = any.Get<void*>();
        void* outPtr = nullptr;

        if (TypeInheritance::Cast(any.GetType(), Type::Instance<P>(), inPtr, &outPtr))
        {
            return static_cast<T*>(outPtr);
        }

        return static_cast<T*>(def);
    }
};
} // AnyDetail

template <typename From, typename To>
void AnyCast<From, To>::Register(To (*fn)(const Any&))
{
    AnyDetail::AnyCastHolder<To>::castFns[Type::Instance<From>()] = fn;
}

template <typename From, typename To>
void AnyCast<From, To>::RegisterDefault()
{
    AnyDetail::AnyCastHolder<To>::castFns[Type::Instance<From>()] = [](const Any& any) {
        return static_cast<To>(any.Get<From>());
    };
}

} // namespace DAVA
