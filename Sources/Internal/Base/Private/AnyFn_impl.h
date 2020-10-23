#pragma once

#ifndef __Dava_AnyFn__
#include "Base/AnyFn.h"
#endif

namespace DAVA
{
class AnyFnInvoker
{
public:
    virtual ~AnyFnInvoker()
    {
    }

    virtual bool IsStatic() const = 0;
    virtual Any Invoke(const AnyFn::AnyFnStorage&, bool castArguments) const = 0;
    virtual Any Invoke(const AnyFn::AnyFnStorage&, bool castArguments, const Any&) const = 0;
    virtual Any Invoke(const AnyFn::AnyFnStorage&, bool castArguments, const Any&, const Any&) const = 0;
    virtual Any Invoke(const AnyFn::AnyFnStorage&, bool castArguments, const Any&, const Any&, const Any&) const = 0;
    virtual Any Invoke(const AnyFn::AnyFnStorage&, bool castArguments, const Any&, const Any&, const Any&, const Any&) const = 0;
    virtual Any Invoke(const AnyFn::AnyFnStorage&, bool castArguments, const Any&, const Any&, const Any&, const Any&, const Any&) const = 0;
    virtual Any Invoke(const AnyFn::AnyFnStorage&, bool castArguments, const Any&, const Any&, const Any&, const Any&, const Any&, const Any&) const = 0;

    virtual AnyFn BindThis(const AnyFn::AnyFnStorage& storage, const void*) const = 0;
};

namespace AnyFnDetails
{
template <typename Fn, typename Ret, typename... Args>
struct StaticAnyFnInvoker : AnyFnInvoker
{
    template <bool, typename R, typename... A>
    struct FinalInvoker;

    bool IsStatic() const override
    {
        return true;
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments) const override
    {
        return FinalInvoker<0 == sizeof...(Args), Ret>::Invoke(storage, castArguments);
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& a1) const override
    {
        return FinalInvoker<1 == sizeof...(Args), Ret, Any>::Invoke(storage, castArguments, a1);
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& a1, const Any& a2) const override
    {
        return FinalInvoker<2 == sizeof...(Args), Ret, Any, Any>::Invoke(storage, castArguments, a1, a2);
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& a1, const Any& a2, const Any& a3) const override
    {
        return FinalInvoker<3 == sizeof...(Args), Ret, Any, Any, Any>::Invoke(storage, castArguments, a1, a2, a3);
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& a1, const Any& a2, const Any& a3, const Any& a4) const override
    {
        return FinalInvoker<4 == sizeof...(Args), Ret, Any, Any, Any, Any>::Invoke(storage, castArguments, a1, a2, a3, a4);
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& a1, const Any& a2, const Any& a3, const Any& a4, const Any& a5) const override
    {
        return FinalInvoker<5 == sizeof...(Args), Ret, Any, Any, Any, Any, Any>::Invoke(storage, castArguments, a1, a2, a3, a4, a5);
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& a1, const Any& a2, const Any& a3, const Any& a4, const Any& a5, const Any& a6) const override
    {
        return FinalInvoker<6 == sizeof...(Args), Ret, Any, Any, Any, Any, Any, Any>::Invoke(storage, castArguments, a1, a2, a3, a4, a5, a6);
    }

    AnyFn BindThis(const AnyFn::AnyFnStorage& storage, const void* this_) const override
    {
        DAVA_THROW(Exception, "AnyFn:: 'this' can't be binded to static function");
    }
};

template <typename Fn, typename Ret, typename... Args>
template <bool, typename R, typename... A>
struct StaticAnyFnInvoker<Fn, Ret, Args...>::FinalInvoker
{
    inline static Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const A&... args)
    {
        DAVA_THROW(Exception, "AnyFn:: can't be invoker with such arguments, type to count mismatch");
    }
};

template <typename Fn, typename Ret, typename... Args>
template <typename R, typename... A>
struct StaticAnyFnInvoker<Fn, Ret, Args...>::FinalInvoker<true, R, A...>
{
    inline static Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const A&... args)
    {
        Fn fn = storage.template GetAuto<Fn>();
        if (castArguments)
            return Any(fn(args.template Cast<Args>()...));

        return Any(fn(args.template Get<Args>()...));
    };
};

template <typename Fn, typename Ret, typename... Args>
template <typename... A>
struct StaticAnyFnInvoker<Fn, Ret, Args...>::FinalInvoker<true, void, A...>
{
    inline static Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const A&... args)
    {
        Fn fn = storage.GetAuto<Fn>();
        if (castArguments)
            fn(args.template Cast<Args>()...);
        else
            fn(args.template Get<Args>()...);
        return Any();
    };
};

template <typename Ret, typename C, typename... Args>
struct ClassAnyFnInvoker : AnyFnInvoker
{
    using Fn = Ret (C::*)(Args...);

    template <bool, typename R, typename... A>
    struct FinalInvoker;

    bool IsStatic() const override
    {
        return false;
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments) const override
    {
        return FinalInvoker<false, Ret>::Invoke(storage, castArguments, Any());
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& that) const override
    {
        return FinalInvoker<0 == sizeof...(Args), Ret>::Invoke(storage, castArguments, that);
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& that, const Any& a1) const override
    {
        return FinalInvoker<1 == sizeof...(Args), Ret, Any>::Invoke(storage, castArguments, that, a1);
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& that, const Any& a1, const Any& a2) const override
    {
        return FinalInvoker<2 == sizeof...(Args), Ret, Any, Any>::Invoke(storage, castArguments, that, a1, a2);
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& that, const Any& a1, const Any& a2, const Any& a3) const override
    {
        return FinalInvoker<3 == sizeof...(Args), Ret, Any, Any, Any>::Invoke(storage, castArguments, that, a1, a2, a3);
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& that, const Any& a1, const Any& a2, const Any& a3, const Any& a4) const override
    {
        return FinalInvoker<4 == sizeof...(Args), Ret, Any, Any, Any, Any>::Invoke(storage, castArguments, that, a1, a2, a3, a4);
    }

    Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& that, const Any& a1, const Any& a2, const Any& a3, const Any& a4, const Any& a5) const override
    {
        return FinalInvoker<5 == sizeof...(Args), Ret, Any, Any, Any, Any, Any>::Invoke(storage, castArguments, that, a1, a2, a3, a4, a5);
    }

    AnyFn BindThis(const AnyFn::AnyFnStorage& storage, const void* this_) const override
    {
        Fn fn = storage.GetAuto<Fn>();
        C* p = const_cast<C*>(static_cast<const C*>(this_));
        return AnyFn([p, fn](Args... args) -> Ret
                     {
                         return (p->*fn)(std::forward<Args>(args)...);
                     });
    }
};

template <typename Ret, typename C, typename... Args>
template <bool, typename R, typename... A>
struct ClassAnyFnInvoker<Ret, C, Args...>::FinalInvoker
{
    inline static Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& cls, const A&... args)
    {
        DAVA_THROW(Exception, "AnyFn:: can't be invoker with such arguments, type or count mismatch");
    }
};

template <typename Ret, typename C, typename... Args>
template <typename R, typename... A>
struct ClassAnyFnInvoker<Ret, C, Args...>::FinalInvoker<true, R, A...>
{
    inline static Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& cls, const A&... args)
    {
        Fn fn = storage.GetAuto<Fn>();
        C* p = const_cast<C*>(cls.Get<const C*>());
        if (castArguments)
            return Any((p->*fn)(args.template Cast<Args>()...));

        return Any((p->*fn)(args.template Get<Args>()...));
    }
};

template <typename Ret, typename C, typename... Args>
template <typename... A>
struct ClassAnyFnInvoker<Ret, C, Args...>::FinalInvoker<true, void, A...>
{
    inline static Any Invoke(const AnyFn::AnyFnStorage& storage, bool castArguments, const Any& cls, const A&... args)
    {
        Fn fn = storage.GetAuto<Fn>();
        C* p = const_cast<C*>(cls.Get<const C*>());
        if (castArguments)
            (p->*fn)(args.template Cast<Args>()...);
        else
            (p->*fn)(args.template Get<Args>()...);
        return Any();
    }
};

template <typename Fn, typename T>
struct FunctorTraits;

template <typename Fn, typename Ret, typename Cls, typename... Args>
struct FunctorTraits<Fn, Ret (Cls::*)(Args...) const>
{
    using InvokerType = StaticAnyFnInvoker<Fn, Ret, Args...>;

    static void InitInvokeParams(AnyFn::Params& params)
    {
        params.Set<Ret, Args...>();
    }
};

} // namespace AnyFnDetails

inline AnyFn::AnyFn() //-V730 no need to init invoceParams
: invoker(nullptr)
{
}

template <typename Fn>
inline AnyFn::AnyFn(const Fn& fn)
{
    using FunctorTraits = AnyFnDetails::FunctorTraits<Fn, decltype(&Fn::operator())>;
    static typename FunctorTraits::InvokerType staticFnInvoker;

    anyFnStorage.SetAuto(fn);
    invoker = &staticFnInvoker;
    FunctorTraits::InitInvokeParams(invokeParams);
}

template <typename Ret, typename... Args>
inline AnyFn::AnyFn(Ret (*fn)(Args...))
    : invokeParams()
{
    static AnyFnDetails::StaticAnyFnInvoker<Ret (*)(Args...), Ret, Args...> staticFnInvoker;

    anyFnStorage.SetAuto(fn);
    invoker = &staticFnInvoker;
    invokeParams.Set<Ret, Args...>();
}

template <typename Ret, typename Cls, typename... Args>
inline AnyFn::AnyFn(Ret (Cls::*fn)(Args...) const)
    : AnyFn(reinterpret_cast<Ret (Cls::*)(Args...)>(fn))
{
}

template <typename Ret, typename Cls, typename... Args>
inline AnyFn::AnyFn(Ret (Cls::*fn)(Args...))
{
    static AnyFnDetails::ClassAnyFnInvoker<Ret, Cls, Args...> classFnInvoker;

    anyFnStorage.SetAuto(fn);
    invoker = &classFnInvoker;
    invokeParams.Set<Ret, Cls*, Args...>();
}

inline bool AnyFn::IsValid() const
{
    return (nullptr != invoker);
}

inline bool AnyFn::IsStatic() const
{
    return invoker->IsStatic();
}

inline const AnyFn::Params& AnyFn::GetInvokeParams() const
{
    return invokeParams;
}

template <typename... Args>
inline Any AnyFn::Invoke(const Args&... args) const
{
    return invoker->Invoke(anyFnStorage, false, args...);
}

template <typename... Args>
inline Any AnyFn::InvokeWithCast(const Args&... args) const
{
    return invoker->Invoke(anyFnStorage, true, args...);
}

inline AnyFn AnyFn::BindThis(const void* this_) const
{
    return invoker->BindThis(anyFnStorage, this_);
}

inline AnyFn::Params::Params()
    : retType(Type::Instance<void>())
{
}

template <typename Ret, typename... Args>
AnyFn::Params& AnyFn::Params::Set()
{
    return SetArgs<Args...>(Type::Instance<Ret>());
}

template <typename... Args>
AnyFn::Params& AnyFn::Params::SetArgs(const Type* retType_)
{
    retType = retType_;
    argsType.reserve(sizeof...(Args));

    auto args_push_back = [this](const Type* t) {
        argsType.push_back(t);
        return true;
    };

    bool unpack[] = { true, args_push_back(Type::Instance<Args>())... };

    return *this;
}

template <typename Ret, typename... Args>
inline AnyFn::Params AnyFn::Params::From()
{
    return Params().Set<Ret, Args...>();
}

template <typename... Args>
inline AnyFn::Params AnyFn::Params::FromArgs(const Type* retType_)
{
    return Params().SetArgs<Args...>(retType_);
}

} // namespace DAVA
