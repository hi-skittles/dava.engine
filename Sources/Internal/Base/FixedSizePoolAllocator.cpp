#include "Base/FixedSizePoolAllocator.h"
#include "Base/TemplateHelpers.h"
#if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_IPHONE__)
#include <stdlib.h>
#endif

#include <cstdlib>

namespace DAVA
{
FixedSizePoolAllocator::FixedSizePoolAllocator(uint32 _blockSize, uint32 _blockArraySize)
{
    DVASSERT(_blockSize >= sizeof(uint8*));

    blockSize = _blockSize;
    blockArraySize = _blockArraySize;
    allocatedBlockArrays = nullptr;
    nextFreeBlock = nullptr;
#ifdef __DAVAENGINE_DEBUG__
    totalBlockCount = 0;
    freeItemCount = 0;
    maxItemCount = 0;
#endif
    CreateNewDataBlock();
}

void FixedSizePoolAllocator::CreateNewDataBlock()
{
    DVASSERT(blockSize >= sizeof(uint8*));
    void** block = static_cast<void**>(::malloc(blockArraySize * blockSize + sizeof(uint8*)));
    //Logger::FrameworkDebug("Allocated new data block: %p pointer size: %d", block, sizeof(uint8*));
    // insert to list
    *block = allocatedBlockArrays;
    allocatedBlockArrays = block;
#ifdef __DAVAENGINE_DEBUG__
    totalBlockCount++;
    freeItemCount += blockArraySize;
#endif

    InsertBlockToFreeNodes(block);
}

void FixedSizePoolAllocator::InsertBlockToFreeNodes(void* block)
{
    uint8* blockItem = OffsetPointer<uint8>(block, sizeof(uint8*) + blockSize * (blockArraySize - 1));
    for (uint32 k = 0; k < blockArraySize; ++k)
    {
        //Logger::FrameworkDebug("Free block added: %p", blockItem);

        *reinterpret_cast<uint8**>(blockItem) = static_cast<uint8*>(nextFreeBlock);
        nextFreeBlock = blockItem;
        blockItem -= blockSize;
    }
}

void FixedSizePoolAllocator::Reset()
{
    nextFreeBlock = 0;
    void* currentBlock = allocatedBlockArrays;
    while (currentBlock)
    {
        void* next = *static_cast<void**>(currentBlock);
        InsertBlockToFreeNodes(currentBlock);
        currentBlock = next;
    }
}

void FixedSizePoolAllocator::DeallocateMemory()
{
#ifdef __DAVAENGINE_DEBUG__
//	DVASSERT(freeItemCount == (totalBlockCount*blockArraySize));
#endif

    while (allocatedBlockArrays)
    {
        uint8* next = *static_cast<uint8**>(allocatedBlockArrays);
        ::free(allocatedBlockArrays);
#ifdef __DAVAENGINE_DEBUG__
        totalBlockCount--;
#endif
        //Logger::FrameworkDebug("Deallocated data block: %p pointer size: %d", allocatedBlockArrays, sizeof(uint8*));
        allocatedBlockArrays = next;
    }

#ifdef __DAVAENGINE_DEBUG__
    DVASSERT(0 == totalBlockCount);
    freeItemCount = 0;
#endif
}

FixedSizePoolAllocator::~FixedSizePoolAllocator()
{
    DeallocateMemory();
}

void* FixedSizePoolAllocator::New()
{
    void* object = 0;
    if (nextFreeBlock == nullptr)
    {
        CreateNewDataBlock();
    }
       
#ifdef __DAVAENGINE_DEBUG__
    freeItemCount--;
    maxItemCount = Max(maxItemCount, (blockArraySize * blockSize) - freeItemCount);
#endif
    object = nextFreeBlock;
    nextFreeBlock = *static_cast<void**>(nextFreeBlock);
    return object;
}

void FixedSizePoolAllocator::Delete(void* block)
{
    *static_cast<void**>(block) = nextFreeBlock;
    nextFreeBlock = block;

#ifdef __DAVAENGINE_DEBUG__
    freeItemCount++;
#endif
}

bool FixedSizePoolAllocator::CheckIsPointerValid(void* blockvoid)
{
    uint8* block = static_cast<uint8*>(blockvoid);
    uint8* currentAllocatedBlockArray = static_cast<uint8*>(allocatedBlockArrays);
    while (currentAllocatedBlockArray)
    {
        //uint8* next = *(uint8**)currentAllocatedBlockArray;
        uint8* next = *reinterpret_cast<uint8**>(currentAllocatedBlockArray);

        if ((block >= currentAllocatedBlockArray + sizeof(uint8*)) && (block < currentAllocatedBlockArray + sizeof(uint8*) + blockSize * blockArraySize))
        {
            // we are inside check is block correct.
            uint32 shift = static_cast<uint32>(block - (currentAllocatedBlockArray + sizeof(uint8*)));
            uint32 mod = shift % blockSize;
            if (mod == 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        currentAllocatedBlockArray = next;
    }
    return false;
}
}
