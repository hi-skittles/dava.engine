#pragma once

#ifndef __Dava_TypeInheritance__
#include "Base/TypeInheritance.h"
#endif

namespace DAVA
{
namespace TypeInheritanceDetail
{
static uint32_t stubData = 0xcccccccc;

template <typename From, typename To>
std::ptrdiff_t GetPtrDiff()
{
    const From* from = reinterpret_cast<const From*>(&stubData);
    const To* to = static_cast<const To*>(from);

    ptrdiff_t ret = reinterpret_cast<uintptr_t>(to) - reinterpret_cast<uintptr_t>(from);
    return ret;
}
} // namespace TypeInheritanceDetail

inline const Vector<TypeInheritance::Info>& TypeInheritance::GetBaseTypes() const
{
    return baseTypesInfo;
}

inline const Vector<TypeInheritance::Info>& TypeInheritance::GetDerivedTypes() const
{
    return derivedTypesInfo;
}

inline bool TypeInheritance::CanUpCast(const Type* from, const Type* to)
{
    void* out = nullptr;
    return TryCast(from, to, CastType::UpCast, nullptr, &out);
}

inline bool TypeInheritance::CanDownCast(const Type* from, const Type* to)
{
    void* out = nullptr;
    return TryCast(from, to, CastType::DownCast, nullptr, &out);
}

inline bool TypeInheritance::UpCast(const Type* from, const Type* to, void* inPtr, void** outPtr)
{
    return TryCast(from, to, CastType::UpCast, inPtr, outPtr);
}

inline bool TypeInheritance::DownCast(const Type* from, const Type* to, void* inPtr, void** outPtr)
{
    return TryCast(from, to, CastType::DownCast, inPtr, outPtr);
}

template <typename T, typename... Bases>
void TypeInheritance::RegisterBases()
{
    bool basesUnpack[] = { false, TypeInheritance::AddBaseType<T, Bases>()... };
    bool derivedUnpack[] = { false, TypeInheritance::AddDerivedType<Bases, T>()... };
}

template <typename T, typename B>
bool TypeInheritance::AddBaseType()
{
    TypeInheritance* inheritance = Type::Instance<T>()->EditInheritance();
    inheritance->AddBaseType(Type::Instance<B>(), TypeInheritanceDetail::GetPtrDiff<T, B>());

    return true;
}

template <typename T, typename D>
bool TypeInheritance::AddDerivedType()
{
    TypeInheritance* inheritance = Type::Instance<T>()->EditInheritance();
    inheritance->AddDerivedType(Type::Instance<D>(), TypeInheritanceDetail::GetPtrDiff<T, D>());

    return true;
}
} // namespace DAVA
