#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

#define IMPL__DAVA_REFLECTION(Cls) \
    template <typename FT__> \
    friend struct DAVA::ReflectionDetail::ReflectionInitializerRunner; \
    static void Dava__ReflectionInitializer() { Dava__ReflectionInitializerS(); } \
    static void Dava__ReflectionInitializerS()

#define IMPL__DAVA_VIRTUAL_REFLECTION(Cls, ...) \
    using Cls__BaseTypes = std::tuple<__VA_ARGS__>; \
    const DAVA::ReflectedType* Dava__GetReflectedType() const override; \
    static void Dava__ReflectionRegisterBases(); \
    static void Dava__ReflectionInitializerV(); \
    static void Dava__ReflectionInitializer() \
    { \
        static bool registred = false; \
        if (!registred) { \
            registred = true; \
            Dava__ReflectionRegisterBases(); \
            Dava__ReflectionInitializerV(); \
        } \
    } \
    template <typename FT__> \
    friend struct DAVA::ReflectionDetail::ReflectionInitializerRunner

namespace DAVA
{
namespace ReflectionDetail
{
template <typename T>
struct ReflectionInitializerRunner
{
protected:
    template <typename U, void (*)()>
    struct SFINAE
    {
    };

    template <typename U>
    static char Test(SFINAE<U, &U::Dava__ReflectionInitializer>*);

    template <typename U>
    static int Test(...);

    static const bool value = std::is_same<decltype(Test<T>(0)), char>::value;

    inline static void RunImpl(std::true_type)
    {
        // T has TypeInitializer function,
        // so we should run it
        T::Dava__ReflectionInitializer();
    }

    inline static void RunImpl(std::false_type)
    {
        // T don't have TypeInitializer function,
        // so nothing to do here
    }

public:
    static void Run()
    {
        using CheckType = typename std::conditional<std::is_class<T>::value, ReflectionInitializerRunner<T>, std::false_type>::type;
        RunImpl(std::integral_constant<bool, CheckType::value>());
    }
};
} // ReflectionDetail
} // namespace DAVA
