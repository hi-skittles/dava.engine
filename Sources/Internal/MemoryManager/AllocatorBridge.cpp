#include "Base/BaseTypes.h"

#if defined(DAVA_MEMORY_PROFILING_ENABLE)

#include "MemoryManager/MemoryManager.h"

namespace DAVA
{
void* TrackingAlloc(size_t size, int poolIndex)
{
    return MemoryManager::Instance()->Allocate(size, poolIndex);
}

void TrackingDealloc(void* ptr)
{
    MemoryManager::Instance()->Deallocate(ptr);
}

void* InternalAlloc(size_t size)
{
    return MemoryManager::Instance()->InternalAllocate(size);
}

void InternalDealloc(void* ptr)
{
    MemoryManager::Instance()->InternalDeallocate(ptr);
}

} // namespace DAVA

#endif // defined(DAVA_MEMORY_PROFILING_ENABLE)
