#include "Base/Platform.h"
#ifndef USE_CPP11_CONCURRENCY

#include "Concurrency/Mutex.h"
#include "Debug/DVAssert.h"
#include "Logger/Logger.h"

namespace DAVA
{
Mutex::Mutex()
{
    int ret = pthread_mutex_init(&mutex, nullptr);
    if (ret != 0)
    {
        Logger::Error("Mutex::Mutex() error: %d", ret);
    }
}

RecursiveMutex::RecursiveMutex()
{
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);

    int ret = pthread_mutex_init(&mutex, &attributes);
    if (ret != 0)
    {
        Logger::Error("RecursiveMutex::RecursiveMutex() error: %d", ret);
    }
}

MutexBase::~MutexBase()
{
    int ret = pthread_mutex_destroy(&mutex);
    if (ret != 0)
    {
        Logger::Error("Mutex::~Mutex() error: %d", ret);
    }
}

void MutexBase::Lock()
{
    int ret = pthread_mutex_lock(&mutex);
    if (ret != 0)
    {
        Logger::Error("MutexBase::Lock() error: %d", ret);
    }
}

void MutexBase::Unlock()
{
    int ret = pthread_mutex_unlock(&mutex);
    if (ret != 0)
    {
        Logger::Error("MutexBase::Unlock() error: %d", ret);
    }
}

bool MutexBase::TryLock()
{
    return pthread_mutex_trylock(&mutex) == 0;
}

} // namespace DAVA

#endif // !USE_CPP11_CONCURRENCY