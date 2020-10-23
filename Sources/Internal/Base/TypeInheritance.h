#pragma once

#include "Base/Type.h"
#include "Base/Vector.h"

namespace DAVA
{
class TypeInheritance final
{
public:
    struct Info
    {
        const Type* type;
        std::ptrdiff_t ptrDiff;
    };

    const Vector<Info>& GetBaseTypes() const;
    const Vector<Info>& GetDerivedTypes() const;

    template <typename T, typename... Bases>
    static void RegisterBases();

    static bool CanUpCast(const Type* from, const Type* to);
    static bool CanDownCast(const Type* from, const Type* to);
    static bool CanCast(const Type* from, const Type* to);

    static bool UpCast(const Type* from, const Type* to, void* inPtr, void** outPtr);
    static bool DownCast(const Type* from, const Type* to, void* inPtr, void** outPtr);
    static bool Cast(const Type* from, const Type* to, void* inPtr, void** outPtr);

private:
    enum class CastType
    {
        UpCast, //!< from class to derived class
        DownCast //!< from class to base class
    };

    static bool TryCast(const Type* from, const Type* to, CastType castType, void* inPtr, void** outPtr);

    template <typename T, typename B>
    static bool AddBaseType();

    template <typename T, typename D>
    static bool AddDerivedType();

    void AddBaseType(const Type* baseType, std::ptrdiff_t baseDiff);
    void AddDerivedType(const Type* derivedType, std::ptrdiff_t derivedDiff);

    Vector<Info> baseTypesInfo;
    Vector<Info> derivedTypesInfo;
};
} // namespace DAVA

#define __Dava_TypeInheritance__
#include "Base/Private/TypeInheritance_impl.h"
