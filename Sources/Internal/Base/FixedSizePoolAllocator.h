#ifndef __DAVAENGINE_FIXED_SIZE_POOL_ALLOCATOR_H__
#define __DAVAENGINE_FIXED_SIZE_POOL_ALLOCATOR_H__

#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
/**
    \brief Fast pool based allocator almost without memory overhead. Can be used to allocate small objects.
    
    Example of usage: 
 */

//class MallocAllocator
//{
//public:
//    Allocator()
//    {
//    }
//
//    ~Allocator()
//    {
//    }
//
//    ITEM * Alloc()
//    {
//        void * p = ::malloc(sizeof(ITEM));
//        //ITEM * item = new (p) ITEM;
//        return (ITEM*)p;
//    }
//
//    void Dealloc(ITEM * node)
//    {
//        //node->~ITEM();
//        ::free(node);
//    }
//};

class FixedSizePoolAllocator
{
public:
    FixedSizePoolAllocator(uint32 _blockSize, uint32 _blockArraySize);
    ~FixedSizePoolAllocator();

    void* New();
    void Delete(void* _item);
    void Reset();

    bool CheckIsPointerValid(void* block);
    void InsertBlockToFreeNodes(void* block);
    void CreateNewDataBlock();
    void DeallocateMemory();
    uint32 blockSize;
    uint32 blockArraySize;
    void* allocatedBlockArrays; // ptr to last allocated block
    void* nextFreeBlock; // next free

    //debug stats
    uint32 totalBlockCount;
    uint32 freeItemCount;
    uint32 maxItemCount;
};
};

#endif // __DAVAENGINE_FIXED_SIZE_POOL_ALLOCATOR_H__