#ifndef __DAVAENGINE_ALIGNED_ALLOCATOR_H__
#define __DAVAENGINE_ALIGNED_ALLOCATOR_H__

#include <stdlib.h>
#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
void* AllocateAlignedMemory(uint32 size, uint32 align);
void FreeAlignedMemory(void*);

template <class C, uint32 align, class... Args>
C* CreateObjectAligned(Args&&... a)
{
    auto ptr = AllocateAlignedMemory(static_cast<uint32>(sizeof(C)), align);
    return new (ptr) C(std::forward<Args>(a)...);
}

template <class C>
void DestroyObjectAligned(C* c)
{
    DVASSERT(nullptr != c);
    c->~C();
    FreeAlignedMemory(c);
}
}

#endif //__DAVAENGINE_ALIGNED_ALLOCATOR_H__