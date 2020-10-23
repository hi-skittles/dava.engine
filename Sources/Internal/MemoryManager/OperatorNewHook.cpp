#include "Base/BaseTypes.h"

#if defined(DAVA_MEMORY_PROFILING_ENABLE)

#include "MemoryManager.h"

/*
* http://en.cppreference.com/w/cpp/memory/new/operator_new
* The single-object version is called by the standard library implementations of all other versions,
* so replacing that one function is sufficient to handle all deallocations.	(since C++11)
*/

DAVA_NOINLINE void* operator new(size_t size)
{
    return DAVA::MemoryManager::Instance()->Allocate(size, DAVA::ALLOC_POOL_DEFAULT);
}

void operator delete(void* ptr) DAVA_NOEXCEPT
{
    DAVA::MemoryManager::Instance()->Deallocate(ptr);
}

DAVA_NOINLINE void* operator new[](size_t size)
{
    return DAVA::MemoryManager::Instance()->Allocate(size, DAVA::ALLOC_POOL_DEFAULT);
}

void operator delete[](void* ptr)DAVA_NOEXCEPT
{
    DAVA::MemoryManager::Instance()->Deallocate(ptr);
}

#endif // defined(DAVA_MEMORY_PROFILING_ENABLE)
