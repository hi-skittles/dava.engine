#include "Base/AllocatorFactory.h"
#include "Logger/Logger.h"

namespace DAVA
{
AllocatorFactory::AllocatorFactory()
{
}

AllocatorFactory::~AllocatorFactory()
{
#ifdef __DAVAENGINE_DEBUG__
    Dump();
#endif

    Map<String, FixedSizePoolAllocator*>::iterator itEnd = allocators.end();
    for (Map<String, FixedSizePoolAllocator*>::iterator it = allocators.begin(); it != itEnd; ++it)
    {
        delete ((*it).second);
    }
    allocators.clear();
}

void AllocatorFactory::Dump()
{
#ifdef __DAVAENGINE_DEBUG__
    Logger::FrameworkDebug("AllocatorFactory::Dump (Max item count) ================");
    Map<String, FixedSizePoolAllocator*>::iterator itEnd = allocators.end();
    for (Map<String, FixedSizePoolAllocator*>::iterator it = allocators.begin(); it != itEnd; ++it)
    {
        FixedSizePoolAllocator* alloc = (*it).second;
        Logger::FrameworkDebug("  %s: %u", it->first.c_str(), alloc->maxItemCount);
    }

    Logger::FrameworkDebug("End of AllocatorFactory::Dump ==========================");
#endif //__DAVAENGINE_DEBUG__
}

FixedSizePoolAllocator* AllocatorFactory::GetAllocator(const DAVA::String& className, DAVA::uint32 classSize, int32 poolLength)
{
    FixedSizePoolAllocator* alloc = allocators[className];
    if (0 == alloc)
    {
        alloc = new FixedSizePoolAllocator(classSize, poolLength);
        allocators[className] = alloc;
    }

    return alloc;
}
}