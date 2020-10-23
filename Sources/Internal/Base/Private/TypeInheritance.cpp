#include "Base/TemplateHelpers.h"
#include "Base/TypeInheritance.h"

namespace DAVA
{
bool TypeInheritance::TryCast(const Type* from, const Type* to, CastType castType, void* inPtr, void** outPtr)
{
    if (to == from)
    {
        *outPtr = inPtr;
        return true;
    }

    bool fromIsPointer = from->IsPointer();
    bool toIsPointer = to->IsPointer();

    // both types are pointers or not-pointers
    if (fromIsPointer == toIsPointer)
    {
        // if pointer - deref them first
        if (fromIsPointer)
        {
            to = to->Deref();
            from = from->Deref();
        }

        // now decay both types
        to = to->Decay();
        from = from->Decay();
    }
    else
    {
        return false;
    }

    if (to == from)
    {
        *outPtr = inPtr;
        return true;
    }

    const TypeInheritance* inheritance = from->GetInheritance();
    if (nullptr != inheritance)
    {
        const Vector<Info>* typesInfo = &inheritance->baseTypesInfo;

        if (castType == CastType::UpCast)
        {
            typesInfo = &inheritance->derivedTypesInfo;
        }

        for (const Info& info : *typesInfo)
        {
            if (info.type == to)
            {
                *outPtr = OffsetPointer<void*>(inPtr, info.ptrDiff);
                return true;
            }
        }

        for (const Info& info : *typesInfo)
        {
            if (TryCast(info.type, to, castType, OffsetPointer<void*>(inPtr, info.ptrDiff), outPtr))
            {
                return true;
            }
        }
    }

    return false;
}

bool TypeInheritance::CanCast(const Type* from, const Type* to)
{
    void* out = nullptr;
    return (TryCast(from, to, CastType::DownCast, nullptr, &out) || TryCast(from, to, CastType::UpCast, nullptr, &out));
}

bool TypeInheritance::Cast(const Type* from, const Type* to, void* inPtr, void** outPtr)
{
    if (TryCast(from, to, CastType::DownCast, inPtr, outPtr))
    {
        return true;
    }
    else if (TryCast(from, to, CastType::UpCast, inPtr, outPtr))
    {
        return true;
    }

    return false;
}

void TypeInheritance::AddBaseType(const Type* baseType, std::ptrdiff_t baseDiff)
{
    baseTypesInfo.emplace_back(Info{ baseType, baseDiff });
}

void TypeInheritance::AddDerivedType(const Type* derivedType, std::ptrdiff_t derivedDiff)
{
    derivedTypesInfo.emplace_back(Info{ derivedType, derivedDiff });
}

} // namespace DAVA
