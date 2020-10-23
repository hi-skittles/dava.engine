#pragma once

#ifndef __DAVA_Reflection_Registrator__
#include "Reflection/ReflectionRegistrator.h"
#endif

#define IMPL__DAVA_REFLECTION_IMPL(Cls) \
    void Cls::Dava__ReflectionInitializerS()

#define IMPL__DAVA_VIRTUAL_REFLECTION_IMPL(Cls) \
    const DAVA::ReflectedType* Cls::Dava__GetReflectedType() const { return DAVA::ReflectionRegistratorDetail::GetByThisPointer(this); } \
    void Cls::Dava__ReflectionRegisterBases() { DAVA::ReflectionRegistratorDetail::BasesRegistrator<Cls, Cls::Cls__BaseTypes>::Register(); } \
    void Cls::Dava__ReflectionInitializerV()

#define IMPL__DAVA_VIRTUAL_TEMPLATE_REFLECTION_IMPL(Cls) \
    template <typename T> const DAVA::ReflectedType* Cls<T>::Dava__GetReflectedType() const { return DAVA::ReflectionRegistratorDetail::GetByThisPointer(this); } \
    template <typename T> void Cls<T>::Dava__ReflectionRegisterBases() { DAVA::ReflectionRegistratorDetail::BasesRegistrator<Cls<T>, Cls<T>::Cls__BaseTypes>::Register(); } \
    template <typename T> void Cls<T>::Dava__ReflectionInitializerV()

#define IMPL__DAVA_VIRTUAL_TEMPLATE_SPECIALIZATION_REFLECTION_IMPL(Cls) \
    template <> const DAVA::ReflectedType* Cls::Dava__GetReflectedType() const { return DAVA::ReflectionRegistratorDetail::GetByThisPointer(this); } \
    template <> void Cls::Dava__ReflectionRegisterBases() { DAVA::ReflectionRegistratorDetail::BasesRegistrator<Cls, Cls::Cls__BaseTypes>::Register(); } \
    template <> void Cls::Dava__ReflectionInitializerV()

#define IMPL__DAVA_VIRTUAL_REFLECTION_IN_PLACE(Cls, ...) \
    template <typename FT__> \
    friend struct DAVA::ReflectionDetail::ReflectionInitializerRunner; \
    using Cls__BaseTypes = std::tuple<__VA_ARGS__>; \
    const DAVA::ReflectedType* Dava__GetReflectedType() const override { return DAVA::ReflectionRegistratorDetail::GetByThisPointer(this); } \
    static void Dava__ReflectionRegisterBases() { DAVA::ReflectionRegistratorDetail::BasesRegistrator<Cls, Cls__BaseTypes>::Register(); } \
    static void Dava__ReflectionInitializer() { Dava__ReflectionRegisterBases(); Dava__ReflectionInitializerV(); } \
    static void Dava__ReflectionInitializerV()

#define IMPL__DAVA_REFLECTION_REGISTER_PERMANENT_NAME(Cls) \
    DAVA::ReflectedTypeDB::RegisterPermanentName(DAVA::ReflectedTypeDB::Get<Cls>(), #Cls)

#define IMPL__DAVA_REFLECTION_REGISTER_CUSTOM_PERMANENT_NAME(Cls, Name) \
    DAVA::ReflectedTypeDB::RegisterPermanentName(DAVA::ReflectedTypeDB::Get<Cls>(), Name)

namespace DAVA
{
namespace ReflectionRegistratorDetail
{
template <typename T, typename Bases>
struct BasesRegistrator;

template <typename T, typename... Bases>
struct BasesRegistrator<T, std::tuple<Bases...>>
{
    static inline void Register()
    {
        ReflectedTypeDB::RegisterBases<T, Bases...>();
    }
};

template <typename T>
const ReflectedType* GetByThisPointer(const T* this_)
{
    return ReflectedTypeDB::Get<T>();
}

template <typename T>
struct FnRetFoReflectionRet
{
    using type = typename std::conditional<std::is_pointer<T>::value,
                                           typename std::remove_pointer<T>::type,
                                           typename std::conditional<std::is_reference<T>::value,
                                                                     typename std::remove_reference<T>::type,
                                                                     std::nullptr_t>::type>
    ::type;
};

template <typename GetF, typename SetF>
struct VwType;

template <typename GetT, typename Cls>
struct VwType<GetT (Cls::*)() const, std::nullptr_t>
{
    using ReT = typename FnRetFoReflectionRet<GetT>::type;
    using SetT = typename std::remove_reference<GetT>::type;
    using GetF = GetT (*)();
    using SetF = void (*)(SetT);
    using VwT = ValueWrapperStaticFnPtr<GetT, SetT>;
};

template <typename GetT, typename SetT, typename GetCls, typename SetCls>
struct VwType<GetT (GetCls::*)() const, void (SetCls::*)(SetT) const>
{
    using ReT = typename FnRetFoReflectionRet<GetT>::type;
    using GetF = GetT (*)();
    using SetF = void (*)(SetT);
    using VwT = ValueWrapperStaticFnPtr<GetT, SetT>;
};

template <typename C, typename GetT, typename Cls>
struct VwType<GetT (Cls::*)(C*) const, std::nullptr_t>
{
    using ReT = typename FnRetFoReflectionRet<GetT>::type;
    using SetT = typename std::remove_reference<GetT>::type;
    using GetF = GetT (*)(C*);
    using SetF = void (*)(C*, SetT);
    using VwT = ValueWrapperStaticFnPtrC<C, GetT, SetT>;
};

template <typename C, typename GetT, typename SetT, typename GetCls, typename SetCls>
struct VwType<GetT (GetCls::*)(C*) const, void (SetCls::*)(C*, SetT) const>
{
    using ReT = typename FnRetFoReflectionRet<GetT>::type;
    using GetF = GetT (*)(C*);
    using SetF = void (*)(C*, SetT);
    using VwT = ValueWrapperStaticFnPtrC<C, GetT, SetT>;
};

//
// Getter: lambda T []() { ... }
// Setter: lambda [](T) { ... }
//
template <typename C, typename LaGetF, typename LaSetF>
struct RFCreatorLambda
{
    using RetT = typename VwType<decltype(&LaGetF::operator()), decltype(&LaSetF::operator())>::ReT;
    using GetF = typename VwType<decltype(&LaGetF::operator()), decltype(&LaSetF::operator())>::GetF;
    using SetF = typename VwType<decltype(&LaGetF::operator()), decltype(&LaSetF::operator())>::SetF;
    using VwT = typename VwType<decltype(&LaGetF::operator()), decltype(&LaSetF::operator())>::VwT;

    static ReflectedStructure::Field* Create(LaGetF laGetter, LaSetF laSetter)
    {
        ReflectedStructure::Field* f = new ReflectedStructure::Field();

        GetF getter = laGetter;
        SetF setter = laSetter;

        f->valueWrapper.reset(new VwT(getter, setter));
        return f;
    }
};

//
// Getter: lambda T []() { ... }
// Setter: nullptr
//
template <typename C, typename LaGetF>
struct RFCreatorLambda<C, LaGetF, std::nullptr_t>
{
    using RetT = typename VwType<decltype(&LaGetF::operator()), std::nullptr_t>::ReT;
    using GetF = typename VwType<decltype(&LaGetF::operator()), std::nullptr_t>::GetF;
    using SetF = typename VwType<decltype(&LaGetF::operator()), std::nullptr_t>::SetF;
    using VwT = typename VwType<decltype(&LaGetF::operator()), std::nullptr_t>::VwT;

    static ReflectedStructure::Field* Create(LaGetF laGetter, std::nullptr_t)
    {
        ReflectedStructure::Field* f = new ReflectedStructure::Field();

        GetF getter = laGetter;

        f->valueWrapper.reset(new VwT(getter, nullptr));
        return f;
    }
};

//
// default RFCreator
//
template <typename C, typename GetF, typename SetF>
struct RFCreator
{
    static ReflectedStructure::Field* Create(GetF getter, SetF setter)
    {
        // if no RFCreator specialization choosen
        // last step is to check if setter/gerret are lambdas
        return RFCreatorLambda<C, GetF, SetF>::Create(getter, setter);
    }
};

//
// Getter: T fn()
// Setter: nullptr
//
template <typename C, typename GetT>
struct RFCreator<C, GetT (*)(), std::nullptr_t>
{
    using GetF = GetT (*)();
    using SetT = typename std::remove_reference<GetT>::type;
    using RetT = typename FnRetFoReflectionRet<GetT>::type;

    static ReflectedStructure::Field* Create(GetF getter, std::nullptr_t)
    {
        ReflectedStructure::Field* f = new ReflectedStructure::Field();

        f->valueWrapper.reset(new ValueWrapperStaticFnPtr<GetT, SetT>(getter, nullptr));
        return f;
    }
};

//
// Getter: T fn()
// Setter: void fn(T)
//
template <typename C, typename GetT, typename SetT>
struct RFCreator<C, GetT (*)(), void (*)(SetT)>
{
    using GetF = GetT (*)();
    using SetF = void (*)(SetT);
    using RetT = typename FnRetFoReflectionRet<GetT>::type;

    static ReflectedStructure::Field* Create(GetF getter, SetF setter)
    {
        ReflectedStructure::Field* f = new ReflectedStructure::Field();

        f->valueWrapper.reset(new ValueWrapperStaticFnPtr<GetT, SetT>(getter, setter));
        return f;
    }
};

//
// Getter: T fn(C*)
// Setter: nullptr
//
template <typename C, typename GetT>
struct RFCreator<C, GetT (*)(C*), std::nullptr_t>
{
    using GetF = GetT (*)(C*);
    using SetT = typename std::remove_reference<GetT>::type;
    using RetT = typename FnRetFoReflectionRet<GetT>::type;

    static ReflectedStructure::Field* Create(GetF getter, std::nullptr_t)
    {
        ReflectedStructure::Field* f = new ReflectedStructure::Field();

        f->valueWrapper.reset(new ValueWrapperStaticFnPtrC<C, GetT, SetT>(getter, nullptr));
        return f;
    }
};

//
// Getter: T fn(C*)
// Setter: void fn(C*, T)
//
template <typename C, typename GetT, typename SetT>
struct RFCreator<C, GetT (*)(C*), void (*)(C*, SetT)>
{
    using GetF = GetT (*)(C*);
    using SetF = void (*)(C*, SetT);
    using RetT = typename FnRetFoReflectionRet<GetT>::type;

    static ReflectedStructure::Field* Create(GetF getter, SetF setter)
    {
        ReflectedStructure::Field* f = new ReflectedStructure::Field();

        f->valueWrapper.reset(new ValueWrapperStaticFnPtrC<C, GetT, SetT>(getter, setter));
        return f;
    }
};

//
// Getter: T C::fn()
// Setter: nullptr
//
template <typename C, typename GetT>
struct RFCreator<C, GetT (C::*)(), std::nullptr_t>
{
    using GetF = GetT (C::*)();
    using SetT = typename std::remove_reference<GetT>::type;
    using RetT = typename FnRetFoReflectionRet<GetT>::type;

    static ReflectedStructure::Field* Create(GetF getter, std::nullptr_t)
    {
        ReflectedStructure::Field* f = new ReflectedStructure::Field();

        f->valueWrapper.reset(new ValueWrapperClassFnPtr<C, GetT, SetT>(getter, nullptr));
        return f;
    }
};

//
// Getter: T C::fn()
// Setter: void C::fn(T)
//
template <typename C, typename GetT, typename SetT>
struct RFCreator<C, GetT (C::*)(), void (C::*)(SetT)>
{
    using GetF = GetT (C::*)();
    using SetF = void (C::*)(SetT);
    using RetT = typename FnRetFoReflectionRet<GetT>::type;

    static ReflectedStructure::Field* Create(GetF getter, SetF setter)
    {
        ReflectedStructure::Field* f = new ReflectedStructure::Field();

        f->valueWrapper.reset(new ValueWrapperClassFnPtr<C, GetT, SetT>(getter, setter));
        return f;
    }
};

//
// Getter: T C::fn() const
// Setter: nullptr
//
template <typename C, typename GetT>
struct RFCreator<C, GetT (C::*)() const, std::nullptr_t>
{
    using GetF = GetT (C::*)() const;
    using SetT = typename std::remove_reference<GetT>::type;
    using RetT = typename FnRetFoReflectionRet<GetT>::type;

    static ReflectedStructure::Field* Create(GetF getter, std::nullptr_t)
    {
        ReflectedStructure::Field* f = new ReflectedStructure::Field();

        f->valueWrapper.reset(new ValueWrapperClassFnPtr<C, GetT, SetT>(reinterpret_cast<GetT (C::*)()>(getter), nullptr));
        return f;
    }
};

//
// Getter: T C::fn() const
// Setter: void C::fn(T)
//
template <typename C, typename GetT, typename SetT>
struct RFCreator<C, GetT (C::*)() const, void (C::*)(SetT)>
{
    using GetF = GetT (C::*)() const;
    using SetF = void (C::*)(SetT);
    using RetT = typename FnRetFoReflectionRet<GetT>::type;

    static ReflectedStructure::Field* Create(GetF getter, SetF setter)
    {
        ReflectedStructure::Field* f = new ReflectedStructure::Field();

        f->valueWrapper.reset(new ValueWrapperClassFnPtr<C, GetT, SetT>(reinterpret_cast<GetT (C::*)()>(getter), setter));
        return f;
    }
};
} // namespace ReflectionQualifierDetail

template <typename C>
ReflectionRegistrator<C>::ReflectionRegistrator(std::unique_ptr<StructureWrapper>&& customStructureWrapper)
{
    ReflectedType* type = ReflectedTypeDB::Edit<C>();

    if (type->structure == nullptr)
    {
        type->structure.reset(new ReflectedStructure());
        if (customStructureWrapper != nullptr)
        {
            type->structureWrapper = std::move(customStructureWrapper);
        }
        else
        {
            type->structureWrapper.reset(new StructureWrapperClass(Type::Instance<C>()));
        }
    }

    structure = type->structure.get();
    lastMeta = &structure->meta;
}

template <typename C>
ReflectionRegistrator<C>::~ReflectionRegistrator() = default;

template <typename C>
ReflectionRegistrator<C> ReflectionRegistrator<C>::Begin(std::unique_ptr<StructureWrapper>&& customStructureWrapper)
{
    return ReflectionRegistrator<C>(std::move(customStructureWrapper));
}

template <typename C>
template <typename... Args>
ReflectionRegistrator<C>& ReflectionRegistrator<C>::ConstructorByValue()
{
    AnyFn* ctor = new AnyFn([](Args... args)
                            {
                                return C(std::forward<Args>(args)...);
                            });

    structure->ctors.emplace_back(ctor);
    return *this;
}

template <typename C>
template <typename... Args>
ReflectionRegistrator<C>& ReflectionRegistrator<C>::ConstructorByPointer()
{
    AnyFn* ctor = new AnyFn([](Args... args)
                            {
                                return new C(std::forward<Args>(args)...);
                            });

    structure->ctors.emplace_back(ctor);
    return *this;
}

template <typename C>
template <typename... Args>
ReflectionRegistrator<C>& ReflectionRegistrator<C>::ConstructorByPointer(C* (*fn)(Args...))
{
    AnyFn* ctor = new AnyFn(fn);

    structure->ctors.emplace_back(ctor);
    return *this;
}

template <typename C>
ReflectionRegistrator<C>& ReflectionRegistrator<C>::DestructorByPointer()
{
    AnyFn* dtor = new AnyFn([](C* ptr)
                            {
                                delete ptr;
                            });

    structure->dtor.reset(dtor);
    return *this;
}

template <typename C>
ReflectionRegistrator<C>& ReflectionRegistrator<C>::DestructorByPointer(void (*fn)(C*))
{
    AnyFn* dtor = new AnyFn(fn);

    structure->dtor.reset(dtor);
    return *this;
}

template <typename C>
ReflectionRegistrator<C>& ReflectionRegistrator<C>::AddField(const char* name, ReflectedStructure::Field* f)
{
    f->name = ReflectedStructure::Key(name);
    lastMeta = &f->meta;
    structure->fields.emplace_back(f);

    return *this;
}

template <typename C>
template <typename T>
ReflectionRegistrator<C>& ReflectionRegistrator<C>::Field(const char* name, T* field)
{
    ReflectedStructure::Field* f = new ReflectedStructure::Field();
    f->valueWrapper.reset(new ValueWrapperStatic<T>(field));

    return AddField(name, f);
}

template <typename C>
template <typename T>
ReflectionRegistrator<C>& ReflectionRegistrator<C>::Field(const char* name, T C::*field)
{
    ReflectedStructure::Field* f = new ReflectedStructure::Field();
    f->valueWrapper.reset(new ValueWrapperClass<C, T>(field));

    return AddField(name, f);
}

template <typename C>
template <typename GetF, typename SetF>
ReflectionRegistrator<C>& ReflectionRegistrator<C>::Field(const char* name, GetF getter, SetF setter)
{
    ReflectedStructure::Field* f = ReflectionRegistratorDetail::RFCreator<C, GetF, SetF>::Create(getter, setter);
    f->name = ReflectedStructure::Key(name);

    lastMeta = &f->meta;
    structure->fields.emplace_back(f);

    return *this;
}

template <typename C>
template <typename F>
ReflectionRegistrator<C>& ReflectionRegistrator<C>::Method(const char* name, const F& fn)
{
    ReflectedStructure::Method* m = new ReflectedStructure::Method();
    m->name = ReflectedStructure::Key(name);
    m->fn = AnyFn(fn);

    lastMeta = &m->meta;
    structure->methods.emplace_back(m);

    return *this;
}

template <typename C>
ReflectionRegistrator<C>& ReflectionRegistrator<C>::BindMeta(ReflectedMeta&& meta)
{
    DVASSERT(nullptr != lastMeta, "Trying to set meta while meta is already set or there is nothing to set it on.");

    if (nullptr != lastMeta)
    {
        lastMeta->reset(new ReflectedMeta(std::move(meta)));
        lastMeta = nullptr;
    }

    return *this;
}

template <typename C>
ReflectionRegistrator<C>& ReflectionRegistrator<C>::operator[](ReflectedMeta&& meta)
{
    return BindMeta(std::move(meta));
}

template <typename C>
void ReflectionRegistrator<C>::End()
{
    ReflectedType* type = ReflectedTypeDB::Edit<C>();
    if (nullptr != type->structureWrapper)
    {
        type->structureWrapper->Update();
    }

    structure = nullptr;
    lastMeta = nullptr;
}

} // namespace DAVA
