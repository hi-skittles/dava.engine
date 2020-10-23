#pragma once

#include <memory>

namespace DAVA
{
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
// FIX: replace DefaultSTLAllocator with TrackingAllocator after fixing framework and game codebases
template <typename T>
using DefaultSTLAllocator = std::allocator<T>;
//using DefaultSTLAllocator = TrackingAllocator<T, ALLOC_POOL_DEFAULT>;
#else
template <typename T>
using DefaultSTLAllocator = std::allocator<T>;
#endif
}
