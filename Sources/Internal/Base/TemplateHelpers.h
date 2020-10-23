#pragma once

#include "Base/BaseTypes.h"
#include "Base/NullType.h"
#include "Base/TypeList.h"

#include <typeinfo>
#include <type_traits>
#include <utility>
#include <cassert>
#include <cstdlib>

namespace DAVA
{
// Template for integral constant types
template <typename T, T Value>
struct IntegralConstant
{
    static const T value = Value;
};

// Specializations for boolean constants
using FalseType = IntegralConstant<bool, false>;
using TrueType = IntegralConstant<bool, true>;

template <bool C, typename T = void>
struct EnableIf
{
    using type = T;
};

template <typename T>
struct EnableIf<false, T>
{
};

template <int v>
struct Int2Type
{
    enum
    {
        value = v
    };
};

template <class T>
struct Type2Type
{
    using OriginalType = T;
};

template <bool flag, typename T, typename U>
struct Select
{
    using Result = T;
};
template <typename T, typename U>
struct Select<false, T, U>
{
    using Result = U;
};

template <bool, unsigned int index_A, unsigned int index_B>
struct SelectIndex
{
    enum
    {
        result = index_A
    };
};

template <unsigned int index_A, unsigned int index_B>
struct SelectIndex<false, index_A, index_B>
{
    enum
    {
        result = index_B
    };
};

template <typename U>
struct PointerTraits
{
    enum
    {
        result = false
    };
    using PointerType = NullType;
};
template <typename U>
struct PointerTraits<U*>
{
    enum
    {
        result = true
    };
    using PointerType = U;
};

template <typename U>
struct ReferenceTraits
{
    enum
    {
        result = false
    };
    using ReferenceType = NullType;
};
template <typename U>
struct ReferenceTraits<U&>
{
    enum
    {
        result = true
    };
    using ReferenceType = U;
};

template <class U>
struct P2MTraits
{
    enum
    {
        result = false
    };
};

template <class R, class V>
struct P2MTraits<R V::*>
{
    enum
    {
        result = true
    };
};

template <typename T1, typename T2>
struct IsSame
{
    enum
    {
        result = false
    };
};

template <typename T>
struct IsSame<T, T>
{
    enum
    {
        result = true
    };
};

namespace TemplateHelper
{
using StdUnsignedInts = DAVA_TYPELIST_5(unsigned char, unsigned short int, unsigned int, unsigned long int, unsigned long long);
using StdSignedInts = DAVA_TYPELIST_5(signed char, short int, int, long int, long long);
using StdOtherInts = DAVA_TYPELIST_3(bool, char, wchar_t);
using StdFloats = DAVA_TYPELIST_3(float, double, long double);
};

template <typename T>
class TypeTraits
{
public:
    enum
    {
        isStdUnsignedInt = (TL::IndexOf<TemplateHelper::StdUnsignedInts, T>::value >= 0)
    };
    enum
    {
        isStdSignedInt = (TL::IndexOf<TemplateHelper::StdSignedInts, T>::value >= 0)
    };
    enum
    {
        isStdIntegral = (isStdUnsignedInt || isStdSignedInt || TL::IndexOf<TemplateHelper::StdOtherInts, T>::value >= 0)
    };
    enum
    {
        isStdFloat = (TL::IndexOf<TemplateHelper::StdFloats, T>::value >= 0)
    };
    enum
    {
        isStdArith = isStdIntegral || isStdFloat
    };
    enum
    {
        isStdFundamental = isStdArith || isStdFloat || IsSame<T, void>::result
    };

    enum
    {
        isPointer = PointerTraits<T>::result
    };
    enum
    {
        isReference = ReferenceTraits<T>::result
    };
    enum
    {
        isPointerToMemberFunction = P2MTraits<T>::result
    };

    using ParamType = typename Select<isPointer || isReference, T, const T&>::Result;
    using NonRefType = typename Select<isReference, typename ReferenceTraits<T>::ReferenceType, T>::Result;
};

template <class TO, class FROM>
class Conversion
{
    using Small = char;
    struct Big
    {
        char value[2];
    };

    static Small Test(FROM);
    static Big Test(...);

    static TO MakeTO();

public:
    enum
    {
        exists =
        sizeof(Test(MakeTO())) == sizeof(Small)
    };

    enum
    {
        sameType = false
    };
};

template <bool>
struct IsEnumImpl
{
};

template <>
struct IsEnumImpl<true>
{
    enum
    {
        result = true
    };
};

template <>
struct IsEnumImpl<false>
{
    enum
    {
        result = false
    };
};

template <class T>
struct IsEnum : public IsEnumImpl<__is_enum(T)>
{
};

template <class T>
class Conversion<T, T>
{
public:
    enum
    {
        exists = true
    };
    enum
    {
        sameType = true
    };
};
    
#define SUPERSUBCLASS(SUPER, SUB) (Conversion<const SUB*, const SUPER*>::exists && !Conversion<const SUPER*, const void*>::sameType)

/**
    \brief Works like dynamic_cast for Debug and like a static_cast for release.
    */
template <class C, class O>
C DynamicTypeCheck(O* pObject)
{
#ifdef __DAVAENGINE_DEBUG__
    if (!pObject)
        return static_cast<C>(pObject);

    C c = dynamic_cast<C>(pObject);
    assert(c != nullptr);
    return c;
#else
    return static_cast<C>(pObject);
#endif
}

/**
    \brief Returns true if object pointer is a pointer to the exact class.
    */
template <class C, class O>
bool IsPointerToExactClass(const O* pObject)
{
    if (pObject)
    {
        static_assert(!std::is_pointer<C>::value, "IsPointerToExactClass doesn't operate on pointers");
        return typeid(*pObject) == typeid(C);
    }
    return false;
}

/**
Return specified 'object' casted to specified type 'C' if 'object' has type 'C'. Return nullptr otherwise. The behavior is undefined until 'C' is not a pointer.
*/
template <class C, class O>
C CastIfEqual(O* object)
{
    if (object)
    {
        static_assert(std::is_pointer<C>::value, "CastIfEqual operates on pointers");
        if (typeid(*object) == typeid(typename PointerTraits<C>::PointerType))
        {
            return static_cast<C>(object);
        }
    }
    return nullptr;
}

//ScopeGuard is borrowed from https://github.com/facebook/folly
template <typename FunctionType>
class ScopeGuardImpl
{
public:
    explicit ScopeGuardImpl(const FunctionType& fn)
        : function_(fn)
    {
    }

    explicit ScopeGuardImpl(FunctionType&& fn)
        : function_(std::move(fn))
    {
    }

    ScopeGuardImpl(ScopeGuardImpl&& other)
        : function_(std::move(other.function_))
    {
    }

    ~ScopeGuardImpl()
    {
        execute();
    }

private:
    void* operator new(std::size_t) = delete;
    void execute()
    {
        function_();
    }
    FunctionType function_;
};

enum class ScopeGuardOnExit
{
};

template <typename FunctionType>
ScopeGuardImpl<typename std::decay<FunctionType>::type>
operator+(ScopeGuardOnExit, FunctionType&& fn)
{
    return ScopeGuardImpl<typename std::decay<FunctionType>::type>(std::forward<FunctionType>(fn));
}
};

#ifndef DF_ANONYMOUS_VARIABLE
#define DF_CONCATENATE_IMPL(s1, s2) s1##s2
#define DF_CONCATENATE(s1, s2) DF_CONCATENATE_IMPL(s1, s2)
#ifdef __COUNTER__
#define DF_ANONYMOUS_VARIABLE(str) DF_CONCATENATE(str, __COUNTER__)
#else
#define DF_ANONYMOUS_VARIABLE(str) DF_CONCATENATE(str, __LINE__)
#endif
#endif

#define SCOPE_EXIT auto DF_ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE) = ::DAVA::ScopeGuardOnExit() + [&]()

template <typename T, size_t N>
DAVA_CONSTEXPR size_t COUNT_OF(T(&)[N]) DAVA_NOEXCEPT
{
    return N;
}

/*
Useful functions to offset pointer by specified number of bytes without long cast sequences.
*/
template <typename T>
inline T* OffsetPointer(void* ptr, ptrdiff_t offset)
{
    return reinterpret_cast<T*>(static_cast<uint8_t*>(ptr) + offset);
}

template <typename T>
inline const T* OffsetPointer(const void* ptr, ptrdiff_t offset)
{
    return reinterpret_cast<const T*>(static_cast<const uint8_t*>(ptr) + offset);
}
