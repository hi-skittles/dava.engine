#pragma once

#ifndef __DAVA_ReflectedType__
#include "Reflection/Reflection.h"
#endif

namespace DAVA
{
inline const Type* ReflectedType::GetType() const
{
    return type;
}

inline const String& ReflectedType::GetPermanentName() const
{
    return permanentName;
}

inline const ReflectedStructure* ReflectedType::GetStructure() const
{
    return structure.get();
}

inline ReflectedStructure* ReflectedType::EditStructure() const
{
    return structure.get();
}

inline const StructureWrapper* ReflectedType::GetStrucutreWrapper() const
{
    return structureWrapper.get();
}

template <typename... Args>
const AnyFn* ReflectedType::GetCtor(const Type* retType) const
{
    if (nullptr == retType)
    {
        retType = type;
    }

    auto params = DAVA::AnyFn::Params::FromArgs<Args...>(retType);
    for (auto& ctor : structure->ctors)
    {
        if (ctor->GetInvokeParams() == params)
        {
            return ctor.get();
        }
    }

    return nullptr;
}

template <typename... Args>
Any ReflectedType::CreateObject(CreatePolicy policy, Args... args) const
{
    const AnyFn* ctor = nullptr;

    if (policy == CreatePolicy::ByValue)
    {
        ctor = GetCtor<Args...>(type);
    }
    else
    {
        ctor = GetCtor<Args...>(type->Pointer());
    }

    if (nullptr != ctor)
    {
        return ctor->Invoke(std::forward<Args>(args)...);
    }

    DAVA_THROW(Exception, "There is no appropriate ctor.");
}
} // namespace DAVA
