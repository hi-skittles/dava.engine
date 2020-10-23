#include "Concurrency/Semaphore.h"

#include "Base/BaseTypes.h"
#include "Base/Platform.h"
#include "Debug/DVAssert.h"

#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_MACOS__)
#include <dispatch/dispatch.h>
#elif defined(__DAVAENGINE_ANDROID__) || defined(__DAVAENGINE_LINUX__)
#include <semaphore.h>
#endif //PLATFORMS

namespace DAVA
{

#if defined(__DAVAENGINE_WINDOWS__)
// HANDLE semaphore;
static_assert(sizeof(HANDLE) == sizeof(uintptr_t), "fix native semaphore type");
#elif defined(__DAVAENGINE_APPLE__)
// dispatch_semaphore_t semaphore;
static_assert(sizeof(dispatch_semaphore_t) == sizeof(uintptr_t), "fix native semaphore type");
#elif defined(__DAVAENGINE_ANDROID__)
// sem_t semaphore;
static_assert(sizeof(sem_t) == sizeof(uintptr_t), "fix native semaphore type");
#elif defined(__DAVAENGINE_LINUX__)
// For now use heap-allocated handle
#endif //PLATFORMS



#if defined(__DAVAENGINE_WINDOWS__)

// ##########################################################################################################
// Windows implementation
// ##########################################################################################################

Semaphore::Semaphore(uint32 count)
{
#ifdef __DAVAENGINE_WIN32__
    semaphore = reinterpret_cast<uintptr_t>(CreateSemaphoreW(nullptr, count, 0x0FFFFFFF, nullptr));
#else
    semaphore = reinterpret_cast<uintptr_t>(CreateSemaphoreEx(nullptr, count, 0x0FFFFFFF, nullptr, 0, SEMAPHORE_ALL_ACCESS));
#endif
    DVASSERT(0 != semaphore);
}

Semaphore::~Semaphore()
{
    CloseHandle(reinterpret_cast<HANDLE>(semaphore));
}

void Semaphore::Post(uint32 count)
{
    DVASSERT(count > 0);
    ReleaseSemaphore(reinterpret_cast<HANDLE>(semaphore), count, nullptr);
}

void Semaphore::Wait()
{
    WaitForSingleObjectEx(reinterpret_cast<HANDLE>(semaphore), INFINITE, FALSE);
}

#elif defined(__DAVAENGINE_APPLE__)

// ##########################################################################################################
// MacOS/IOS implementation
// ##########################################################################################################

Semaphore::Semaphore(uint32 count)
{
    semaphore = reinterpret_cast<uintptr_t>(dispatch_semaphore_create(count));
}

Semaphore::~Semaphore()
{
    dispatch_release(reinterpret_cast<dispatch_semaphore_t>(semaphore));
}

void Semaphore::Post(uint32 count)
{
    while (count-- > 0)
    {
        dispatch_semaphore_signal(reinterpret_cast<dispatch_semaphore_t>(semaphore));
    }
}

void Semaphore::Wait()
{
    dispatch_semaphore_wait(reinterpret_cast<dispatch_semaphore_t>(semaphore), DISPATCH_TIME_FOREVER);
}

#elif defined(__DAVAENGINE_ANDROID__)

// ##########################################################################################################
// Android implementation
// ##########################################################################################################
Semaphore::Semaphore(uint32 count)
{
    sem_init(reinterpret_cast<sem_t*>(&semaphore), 0, count);
}

Semaphore::~Semaphore()
{
    sem_destroy(reinterpret_cast<sem_t*>(&semaphore));
}

void Semaphore::Post(uint32 count)
{
    while (count-- > 0)
    {
        sem_post(reinterpret_cast<sem_t*>(&semaphore));
    }
}

void Semaphore::Wait()
{
    sem_wait(reinterpret_cast<sem_t*>(&semaphore));
}

#elif defined(__DAVAENGINE_LINUX__)

// ##########################################################################################################
// Linux implementation
// ##########################################################################################################

Semaphore::Semaphore(uint32 count)
    : semaphore(static_cast<void*>(new sem_t))
{
    sem_init(static_cast<sem_t*>(semaphore), 0, count);
}

Semaphore::~Semaphore()
{
    sem_destroy(static_cast<sem_t*>(semaphore));
    delete static_cast<sem_t*>(semaphore);
}

void Semaphore::Post(uint32 count)
{
    while (count-- > 0)
    {
        sem_post(static_cast<sem_t*>(semaphore));
    }
}

void Semaphore::Wait()
{
    sem_wait(static_cast<sem_t*>(semaphore));
}

#endif

} // end namespace DAVA
