#pragma once

#include "Debug/DVAssert.h"

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#include "Reflection/ReflectedObject.h"
#endif

namespace DAVA
{
inline ReflectedObject::ReflectedObject(void* ptr_, const ReflectedType* rtype_)
    : ptr(ptr_)
    , reflectedType(ReflectedTypeDB::GetByPointer(ptr_, rtype_->GetType()))
{
    if (nullptr == reflectedType)
    {
        reflectedType = rtype_;
    }
}

template <typename T>
inline ReflectedObject::ReflectedObject(T* ptr_, bool isConst_)
    : ptr(ptr_)
    , isConst(isConst_)
    , reflectedType(ReflectedTypeDB::GetByPointer(ptr_))
{
}

template <typename T>
inline ReflectedObject::ReflectedObject(const T* ptr_)
    : ptr(const_cast<T*>(ptr_))
    , reflectedType(ReflectedTypeDB::GetByPointer(ptr_))
    , isConst(true)
{
}

inline bool ReflectedObject::operator==(const ReflectedObject& other) const
{
    return ptr == other.ptr &&
    reflectedType == other.reflectedType &&
    isConst == other.isConst;
}

inline bool ReflectedObject::operator!=(const ReflectedObject& other) const
{
    return ptr != other.ptr ||
    reflectedType != other.reflectedType ||
    isConst != other.isConst;
}

inline const ReflectedType* ReflectedObject::GetReflectedType() const
{
    return reflectedType;
}

inline bool ReflectedObject::IsValid() const
{
    return ((nullptr != ptr) && (nullptr != reflectedType));
}

inline bool ReflectedObject::IsConst() const
{
    return isConst;
}

#if 0

// This is performance optimal GetPtr() implementation but it wouldn't
// work for classes with multiple inheritance or with virtual inheritance,
// so we are forced now to use other GetPtr implementation that is making
// appropriate pointer cast
template <typename T>
inline T* ReflectedObject::GetPtr() const
{
    DVASSERT(IsValid());
    DVASSERT(CanGet<T>());

    return static_cast<T*>(ptr);
}

#else

template <typename T>
inline T* ReflectedObject::GetPtr() const // <-- GetPtrWithCast
{
    DVASSERT(IsValid());

    if (CanGet<T>())
    {
        return static_cast<T*>(ptr);
    }

    void* tmp = nullptr;
    bool canCast = TypeInheritance::DownCast(reflectedType->GetType(), Type::Instance<T>(), ptr, &tmp);

#ifdef __DAVAENGINE_DEBUG__
    DVASSERT(canCast);
#endif

    return static_cast<T*>(tmp);
}

#endif

inline void* ReflectedObject::GetVoidPtr() const
{
    return ptr;
}

template <typename T>
inline bool ReflectedObject::CanGet() const
{
    const Type* reqType = Type::Instance<T>();
    const Type* curType = reflectedType->GetType();

    return ((reqType == curType) || (reqType->Decay() == curType));
}

} // namespace DAVA
