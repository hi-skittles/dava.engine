#pragma once

#include <tuple>
#include "Base/Type.h"
#include "Base/Any.h"

namespace DAVA
{
class AnyFnInvoker;
class AnyFn final
{
public:
    struct Params;
    using AnyFnStorage = AutoStorage<>;

    AnyFn();

    template <typename Fn>
    AnyFn(const Fn& fn);

    template <typename Ret, typename... Args>
    AnyFn(Ret (*fn)(Args...));

    template <typename Ret, typename Cls, typename... Args>
    AnyFn(Ret (Cls::*fn)(Args...) const);

    template <typename Ret, typename Cls, typename... Args>
    AnyFn(Ret (Cls::*fn)(Args...));

    bool IsValid() const;
    bool IsStatic() const;

    const Params& GetInvokeParams() const;

    template <typename... Args>
    Any Invoke(const Args&... args) const;

    template <typename... Args>
    Any InvokeWithCast(const Args&... args) const;

    AnyFn BindThis(const void* this_) const;

    struct Params
    {
        Params();
        bool operator==(const Params&) const;

        template <typename Ret, typename... Args>
        Params& Set();

        template <typename... Args>
        Params& SetArgs(const Type* retType = nullptr);

        template <typename Ret, typename... Args>
        static Params From();

        template <typename... Args>
        static Params FromArgs(const Type* retType = nullptr);

        const Type* retType;
        Vector<const Type*> argsType;
    };

private:
    AnyFnStorage anyFnStorage;
    AnyFnInvoker* invoker;
    Params invokeParams;
};

} // namespace DAVA

#define __Dava_AnyFn__
#include "Base/Private/AnyFn_impl.h"