#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

#include "Reflection/Private/Wrappers/StructureWrapperDefault.h"
#include "Reflection/Private/Wrappers/StructureWrapperPtr.h"
#include "Reflection/Private/Wrappers/StructureWrapperStdIdx.h"
#include "Reflection/Private/Wrappers/StructureWrapperStdMap.h"
#include "Reflection/Private/Wrappers/StructureWrapperStdSet.h"

namespace DAVA
{
namespace ReflectedTypeDBDetail
{
template <typename T>
const ReflectedType* GetVirtualReflectedTypeImpl(const T* ptr, std::false_type)
{
    return nullptr;
}

template <typename T>
const ReflectedType* GetVirtualReflectedTypeImpl(const T* ptr, std::true_type)
{
    static_assert(std::is_convertible<const T*, const ReflectionBase*>::value,
                  "T* can't be converted to ReflectionBase*. "
                  "It seems that there is inheritance Diamond Problem. "
                  "Try to use virtual inheritance!");

    return static_cast<const ReflectionBase*>(ptr)->Dava__GetReflectedType();
}

template <typename T>
const ReflectedType* GetVirtualReflectedType(const T* ptr)
{
    static const bool isRefBase = (std::is_base_of<ReflectionBase, T>::value || std::is_same<ReflectionBase, T>::value);
    return GetVirtualReflectedTypeImpl(ptr, std::integral_constant<bool, isRefBase>());
}
} // namespace ReflectedTypeDBDetail

template <typename T>
ReflectedType* ReflectedTypeDB::CreateStatic()
{
    static ReflectedType ret(nullptr);
    return &ret;
}

template <typename T>
ReflectedType* ReflectedTypeDB::Edit()
{
    using DecayT = Type::DecayT<T>;
    ReflectedType* ret = CreateStatic<DecayT>();

    // Newly created ReflectedType has `type == nullptr`,
    // so check if returned ReflectedType is still not initialized
    if (nullptr == ret->type)
    {
        ret->type = Type::Instance<DecayT>();
        ret->structureWrapper.reset(StructureWrapperCreator<DecayT>::Create());

        RegisterDBType(ret);
        ReflectionDetail::ReflectionInitializerRunner<DecayT>::Run();
    }

    return ret;
}

template <typename T>
const ReflectedType* ReflectedTypeDB::Get()
{
    return Edit<T>();
}

template <typename T>
const ReflectedType* ReflectedTypeDB::GetByPointer(const T* ptr)
{
    const ReflectedType* ret = nullptr;

    if (nullptr != ptr)
    {
        ret = ReflectedTypeDBDetail::GetVirtualReflectedType(ptr);
    }

    if (nullptr == ret)
    {
        ret = ReflectedTypeDB::Edit<T>();
    }

    return ret;
}

template <typename T, typename... Bases>
void ReflectedTypeDB::RegisterBases()
{
    TypeInheritance::RegisterBases<T, Bases...>();
    bool basesUnpack[] = { false, ReflectedTypeDB::Edit<Bases>() != nullptr... };
}

} // namespace DAVA
