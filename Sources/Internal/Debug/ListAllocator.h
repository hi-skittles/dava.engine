#ifndef __DAVAENGINE_LIST_ALLOCATOR_H__
#define __DAVAENGINE_LIST_ALLOCATOR_H__

#include "Base/BaseTypes.h"
#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)
#include <stdlib.h>
#endif

#include <cstdlib>

namespace DAVA
{
template <class ITEM>
class Allocator
{
public:
    Allocator()
    {
    }

    ~Allocator()
    {
    }

    ITEM* Alloc()
    {
        void* p = ::malloc(sizeof(ITEM));
        return reinterpret_cast<ITEM*>(p);
    }

    void Dealloc(ITEM* node)
    {
        //node->~ITEM();
        ::free(node);
    }
};
};

#endif // __DAVAENGINE_LIST_ALLOCATOR_H__