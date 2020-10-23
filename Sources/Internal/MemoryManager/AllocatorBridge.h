#ifndef __DAVAENGINE_ALLOCATORBRIDGE_H__
#define __DAVAENGINE_ALLOCATORBRIDGE_H__

#include "Base/BaseTypes.h"

#if defined(DAVA_MEMORY_PROFILING_ENABLE)

namespace DAVA
{
// Freestanding allocation functions that can be used where MemoryManager.h cannot be included (e.g. in allocators)

// For general use
void* TrackingAlloc(size_t size, int poolIndex);
void TrackingDealloc(void* ptr);

// For internal use by MemoryManager
void* InternalAlloc(size_t size);
void InternalDealloc(void* ptr);

} // namespace DAVA

#endif // defined(DAVA_MEMORY_PROFILING_ENABLE)
#endif // __DAVAENGINE_ALLOCATORBRIDGE_H__
