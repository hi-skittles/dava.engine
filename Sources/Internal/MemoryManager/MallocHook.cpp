#include "Base/BaseTypes.h"
#include "Base/Platform.h"

#if defined(DAVA_MEMORY_PROFILING_ENABLE)

#include <cstdlib>
#include <cassert>

#if defined(__DAVAENGINE_WIN32__)
#include <detours/detours.h>
#elif defined(__DAVAENGINE_WIN_UAP__)
#elif defined(__DAVAENGINE_ANDROID__)
#include <malloc.h>
#include <dlfcn.h>
#include <android/log.h>
#elif defined(__DAVAENGINE_APPLE__)
#include <dlfcn.h>
#include <malloc/malloc.h>
#elif defined(__DAVAENGINE_LINUX__)
#include <malloc.h>
#include <dlfcn.h>
#else
#error "Unknown platform"
#endif

#include "MallocHook.h"
#include "AllocPools.h"
#include "MemoryManager.h"

namespace
{
void* HookedMalloc(size_t size)
{
    return DAVA::MemoryManager::Instance()->Allocate(size, DAVA::ALLOC_POOL_DEFAULT);
}

void* HookedRealloc(void* ptr, size_t newSize)
{
    return DAVA::MemoryManager::Instance()->Reallocate(ptr, newSize);
}

void* HookedCalloc(size_t count, size_t elemSize)
{
    void* ptr = nullptr;
    if (count > 0 && elemSize > 0)
    {
        ptr = malloc(count * elemSize);
        if (ptr != nullptr)
        {
            memset(ptr, 0, count * elemSize);
        }
    }
    return ptr;
}

char* HookedStrdup(const char* src)
{
    char* dst = nullptr;
    if (src != nullptr)
    {
        dst = static_cast<char*>(malloc(strlen(src) + 1));
        if (dst != nullptr)
            strcpy(dst, src);
    }
    return dst;
}

void HookedFree(void* ptr)
{
    DAVA::MemoryManager::Instance()->Deallocate(ptr);
}

#if defined(__DAVAENGINE_WIN32__) && defined(_DEBUG)
void* HookedMallocDbg(size_t size, int, const char*, int)
{
    return HookedMalloc(size);
}

void* HookedReallocDbg(void* ptr, size_t newSize, int, const char*, int)
{
    return HookedRealloc(ptr, newSize);
}

void* HookedCallocDbg(size_t count, size_t elemSize, int, const char*, int)
{
    return HookedCalloc(count, elemSize);
}

char* HookedStrdupDbg(const char* src, int, const char*, int)
{
    return HookedStrdup(src);
}

void HookedFreeDbg(void* ptr, int)
{
    HookedFree(ptr);
}
#endif // __DAVAENGINE_WIN32__ && _DEBUG

} // unnamed namespace

namespace DAVA
{
void* (*MallocHook::RealMalloc)(size_t) = nullptr;
void* (*MallocHook::RealRealloc)(void*, size_t) = nullptr;
void (*MallocHook::RealFree)(void*) = nullptr;
#if defined(__DAVAENGINE_ANDROID__)
size_t (*MallocHook::RealMallocSize)(void*) = nullptr;
#endif

#if defined(__DAVAENGINE_WIN32__) && defined(_DEBUG)
void* (*MallocHook::RealMallocDbg)(size_t, int, const char*, int) = nullptr;
void* (*MallocHook::RealReallocDbg)(void*, size_t, int, const char*, int) = nullptr;
void (*MallocHook::RealFreeDbg)(void*, int) = nullptr;
#endif

void* MallocHook::Malloc(size_t size)
{
    static bool isHooked = false;
    if (!isHooked)
    {
        isHooked = true;
        Install();
    }
#if defined(__DAVAENGINE_WIN32__) && defined(_DEBUG)
    return RealMallocDbg(size, _NORMAL_BLOCK, nullptr, 0);
#else
    return RealMalloc(size);
#endif
}

void* MallocHook::Realloc(void* ptr, size_t newSize)
{
#if defined(__DAVAENGINE_WIN32__) && defined(_DEBUG)
    return RealReallocDbg(ptr, newSize, _NORMAL_BLOCK, nullptr, 0);
#else
    return RealRealloc(ptr, newSize);
#endif
}

void MallocHook::Free(void* ptr)
{
#if defined(__DAVAENGINE_WIN32__) && defined(_DEBUG)
    RealFreeDbg(ptr, _NORMAL_BLOCK);
#else
    RealFree(ptr);
#endif
}

size_t MallocHook::MallocSize(void* ptr)
{
#if defined(__DAVAENGINE_WIN32__)
    return _msize(ptr);
#elif defined(__DAVAENGINE_WIN_UAP__)
    return _msize(ptr);
#elif defined(__DAVAENGINE_APPLE__)
    return malloc_size(ptr);
#elif defined(__DAVAENGINE_ANDROID__)
    return RealMallocSize != nullptr ? RealMallocSize(ptr) : 0;
#elif defined(__DAVAENGINE_LINUX__)
    return malloc_usable_size(ptr);
#else
#error "Unknown platform"
#endif
}

void MallocHook::Install()
{
/*
     Explanation of allocation flow:
        app calls malloc --> HookedMalloc --> MemoryManager::Allocate --> MallocHook::Malloc --> original malloc
     Same flow with little differences is applied to other functions
     Such a long chain is necessary for keeping as mush common code among different platforms as possible.
     To be able to call HookedMalloc instead of malloc I use some technique to intercept/replace functions.

     On Win32 I use Microsoft Detours library which modifies function prologue code with call to so called
     trampoline function. This trampoline function makes call to address which was specified by me (HookedMalloc).

     On *nix platforms (Android, iOS, Mac OS X, etc) I simply define my own implementation of malloc. In glibc
     malloc is a weak symbol which means that it can be overridden by an application. Additionally I get original
     address of malloc using dlsym function.
    */
#if defined(__DAVAENGINE_WIN32__)
    auto detours = [](PVOID* what, PVOID hook) -> void {
        LONG result = 0;
        result = DetourTransactionBegin();
        assert(0 == result);
        result = DetourUpdateThread(GetCurrentThread());
        assert(0 == result);
        result = DetourAttach(what, hook);
        assert(0 == result);
        result = DetourTransactionCommit();
        assert(0 == result);
    };

// Separate debug and release implementations on Win32:
//  now Win32 applications link to dynamic runtime and memory allocation can occur inside application
//  but deallocation may occur inside Microsoft CRT dynamic library. Debug version of that library
//  uses allocation routines with `_dbg` suffix and can calls them directly (not through malloc or free,
//  but _malloc_dbg and _free_dbg).

#if defined(_DEBUG)
    RealMallocDbg = &_malloc_dbg;
    RealReallocDbg = &_realloc_dbg;
    RealFreeDbg = &_free_dbg;
    void* (*realCallocDbg)(size_t, size_t, int, const char*, int) = &_calloc_dbg;
    char* (*realStrdupDbg)(const char*, int, const char*, int) = &_strdup_dbg;

    detours(reinterpret_cast<PVOID*>(&RealMallocDbg), reinterpret_cast<PVOID>(&HookedMallocDbg));
    detours(reinterpret_cast<PVOID*>(&RealReallocDbg), reinterpret_cast<PVOID>(&HookedReallocDbg));
    detours(reinterpret_cast<PVOID*>(&realCallocDbg), reinterpret_cast<PVOID>(&HookedCallocDbg));
    detours(reinterpret_cast<PVOID*>(&realStrdupDbg), reinterpret_cast<PVOID>(&HookedStrdupDbg));
    detours(reinterpret_cast<PVOID*>(&RealFreeDbg), reinterpret_cast<PVOID>(&HookedFreeDbg));
#else
    RealMalloc = &::malloc;
    RealRealloc = &::realloc;
    RealFree = &::free;
    void* (*realCalloc)(size_t, size_t) = &calloc;
    char* (*realStrdup)(const char*) = &_strdup;

    // On detours error you will see assert message
    detours(reinterpret_cast<PVOID*>(&RealMalloc), reinterpret_cast<PVOID>(&HookedMalloc));
    detours(reinterpret_cast<PVOID*>(&RealRealloc), reinterpret_cast<PVOID>(&HookedRealloc));
    detours(reinterpret_cast<PVOID*>(&realCalloc), reinterpret_cast<PVOID>(&HookedCalloc));
    detours(reinterpret_cast<PVOID*>(&realStrdup), reinterpret_cast<PVOID>(&HookedStrdup));
    detours(reinterpret_cast<PVOID*>(&RealFree), reinterpret_cast<PVOID>(&HookedFree));
#endif

#elif defined(__DAVAENGINE_WIN_UAP__)
    RealMalloc = &malloc;
    RealRealloc = &realloc;
    RealFree = &free;
#elif defined(__DAVAENGINE_POSIX__)
    void* fptr = nullptr;

// RTLD_DEFAULT tells to find the next occurrence of the desired symbol
// in the search order after the current library
// RTLD_NEXT tells to find the next occurrence of the desired symbol
// in the search order after the current library

// On Android use RTLD_DEFAULT as on RTLD_NEXT dlsym returns null
// On Mac OS and iOS use RTLD_NEXT to not call malloc recursively
#if defined(__DAVAENGINE_ANDROID__)
    void* handle = RTLD_DEFAULT;
#else
    void* handle = RTLD_NEXT;
#endif
    fptr = dlsym(handle, "malloc");
    RealMalloc = reinterpret_cast<void* (*)(size_t)>(fptr);
    assert(fptr != nullptr && "Failed to get 'malloc'");

    fptr = dlsym(handle, "realloc");
    RealRealloc = reinterpret_cast<void* (*)(void*, size_t)>(fptr);
    assert(fptr != nullptr && "Failed to get 'realloc'");

    fptr = dlsym(handle, "free");
    RealFree = reinterpret_cast<void (*)(void*)>(fptr);
    assert(fptr != nullptr && "Failed to get 'free'");

#if defined(__DAVAENGINE_ANDROID__)
    // Get address of malloc_usable_size as it isn't exported on android
    fptr = dlsym(RTLD_DEFAULT, "malloc_usable_size");
    if (nullptr == fptr)
    {
        void* libc = dlopen("libc.so", 0);
        if (libc != nullptr)
        {
            fptr = dlsym(libc, "malloc_usable_size");
        }
    }

    RealMallocSize = reinterpret_cast<size_t (*)(void*)>(fptr);
    if (nullptr == RealMallocSize)
    { // DAVA::Logger in not available yet
        __android_log_print(ANDROID_LOG_ERROR, "DAVA", "!!! malloc_usable_size is not available");
    }
#endif

#else
#error "Unknown platform"
#endif
}

} // namespace DAVA

#if defined(__DAVAENGINE_POSIX__)

void* malloc(size_t size)
{
    return HookedMalloc(size);
}

void free(void* ptr)
{
    HookedFree(ptr);
}

void* realloc(void* ptr, size_t newSize)
{
    return HookedRealloc(ptr, newSize);
}

void* calloc(size_t count, size_t elemSize)
{
    return HookedCalloc(count, elemSize);
}

char* strdup(const char* src)
{
    return HookedStrdup(src);
}

#endif // defined(__DAVAENGINE_POSIX__)

#endif // defined(DAVA_MEMORY_PROFILING_ENABLE)
