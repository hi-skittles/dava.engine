#include "Base/AlignedAllocator.h"

namespace DAVA
{
void* AllocateAlignedMemory(uint32 size, uint32 align)
{
#if defined(__DAVAENGINE_WINDOWS__)

    return _aligned_malloc(size, align);

#elif defined(__DAVAENGINE_ANDROID__) // posix_memalign not supported in anrdoid api < 17

    return malloc(size);

#else // assuming POSIX for now

    void* result = nullptr;
    int32 success = posix_memalign(&result, align, size);
    DVASSERT(success == 0);
    return result;

#endif
}

void FreeAlignedMemory(void* ptr)
{
#if defined(__DAVAENGINE_WINDOWS__)

    _aligned_free(ptr);

#else // still assuming POSIX for now

    free(ptr);

#endif
}
}