#pragma once

#include <type_traits>
#include <cassert>

#include "Base/Platform.h"

namespace DAVA
{
/*
    template class ThreadLocalPtr - implementation of cross-platform thread local storage (TLS). Each instance of ThreadLocalPtr
    represents a pointer to an object of type T. Each thread has a distinct value.

    To obtain value for the current thread one can use Get() method, or -> and * pointer dereference operators. Initially pointer
    has nullptr value for each thread, to set value for the current thread one can use Reset() method.

    ThreadLocalPtr's interface is similar to interface of boost::thread_specific_ptr.

    C++ 11 supports thread_local keyword which does the same thing but not all compilers support it.
    Also, compiler specific stuff (__declspec(thread), __thread) does not work well between platforms.

    Restrictions:
        variables of type ThreadLocal can have only static storage duration (global or local static, and static data member)
        if you declare ThreadLocal as automatic object it's your own problems, so don't cry: Houston, we've got a problem

    TODO:
        integrate ThreadLocalPtr into DAVA::Thread to support automatic cleanup on thread exit. For now user is responsible for
        calling ThreadLocalPtr::Reset() to delete pointer
*/
template <typename T>
class ThreadLocalPtr final
{
#if defined(__DAVAENGINE_WINDOWS__)
    using KeyType = DWORD;
#elif defined(__DAVAENGINE_POSIX__)
    using KeyType = pthread_key_t;
#else
#error "ThreadLocalPtr: platform is unknown"
#endif

public:
    ThreadLocalPtr() DAVA_NOEXCEPT;
    ThreadLocalPtr(void (*deleter_)(T*)) DAVA_NOEXCEPT;
    ~ThreadLocalPtr() DAVA_NOEXCEPT;

    T* Get() const DAVA_NOEXCEPT;
    T* operator->() const DAVA_NOEXCEPT;
    T& operator*() const DAVA_NOEXCEPT;

    // Get current pointer and set nullptr without deleting
    T* Release() DAVA_NOEXCEPT;
    // Set new pointer, delete previous pointer if it is not same with new pointer
    void Reset(T* newValue = nullptr) DAVA_NOEXCEPT;

    // Method to test whether thread local storage has been successfully created by system
    bool IsCreated() const DAVA_NOEXCEPT;

private:
    ThreadLocalPtr(const ThreadLocalPtr&) = delete;
    ThreadLocalPtr& operator=(const ThreadLocalPtr&) = delete;

    // Platform spefific methods
    void CreateTlsKey() DAVA_NOEXCEPT;
    void DeleteTlsKey() const DAVA_NOEXCEPT;
    void SetTlsValue(void* rawValue) const DAVA_NOEXCEPT;
    void* GetTlsValue() const DAVA_NOEXCEPT;

    static void DefaultDeleter(T* ptr) DAVA_NOEXCEPT;

private:
    KeyType key;
    bool isCreated = false;
    void (*deleter)(T*);
};

//////////////////////////////////////////////////////////////////////////

template <typename T>
inline ThreadLocalPtr<T>::ThreadLocalPtr() DAVA_NOEXCEPT
: deleter(&DefaultDeleter)
{
    CreateTlsKey();
}

template <typename T>
inline ThreadLocalPtr<T>::ThreadLocalPtr(void (*deleter_)(T*)) DAVA_NOEXCEPT
: deleter(deleter_)
{
    CreateTlsKey();
}

template <typename T>
inline ThreadLocalPtr<T>::~ThreadLocalPtr() DAVA_NOEXCEPT
{
    DeleteTlsKey();
}

template <typename T>
inline T* ThreadLocalPtr<T>::Get() const DAVA_NOEXCEPT
{
    return static_cast<T*>(GetTlsValue());
}

template <typename T>
inline T* ThreadLocalPtr<T>::operator->() const DAVA_NOEXCEPT
{
    return Get();
}

template <typename T>
inline T& ThreadLocalPtr<T>::operator*() const DAVA_NOEXCEPT
{
    return *Get();
}

template <typename T>
inline T* ThreadLocalPtr<T>::Release() DAVA_NOEXCEPT
{
    T* ptr = Get();
    SetTlsValue(nullptr);
    return ptr;
}

template <typename T>
inline void ThreadLocalPtr<T>::Reset(T* newValue) DAVA_NOEXCEPT
{
    T* curValue = Get();
    if (curValue != newValue)
    {
        deleter(curValue);
        SetTlsValue(newValue);
    }
}

template <typename T>
inline bool ThreadLocalPtr<T>::IsCreated() const DAVA_NOEXCEPT
{
    return isCreated;
}

template <typename T>
void ThreadLocalPtr<T>::DefaultDeleter(T* ptr) DAVA_NOEXCEPT
{
    delete ptr;
}

// Windows implementation
#if defined(__DAVAENGINE_WINDOWS__)

template <typename T>
inline void ThreadLocalPtr<T>::CreateTlsKey() DAVA_NOEXCEPT
{
    key = TlsAlloc();
    isCreated = (key != TLS_OUT_OF_INDEXES);
    assert(isCreated);
}

template <typename T>
inline void ThreadLocalPtr<T>::DeleteTlsKey() const DAVA_NOEXCEPT
{
    TlsFree(key);
}

template <typename T>
inline void ThreadLocalPtr<T>::SetTlsValue(void* rawValue) const DAVA_NOEXCEPT
{
    TlsSetValue(key, rawValue);
}

template <typename T>
inline void* ThreadLocalPtr<T>::GetTlsValue() const DAVA_NOEXCEPT
{
    return TlsGetValue(key);
}

// POSIX implementation
#elif defined(__DAVAENGINE_POSIX__)

template <typename T>
inline void ThreadLocalPtr<T>::CreateTlsKey() DAVA_NOEXCEPT
{
    isCreated = (0 == pthread_key_create(&key, nullptr));
    assert(isCreated);
}

template <typename T>
inline void ThreadLocalPtr<T>::DeleteTlsKey() const DAVA_NOEXCEPT
{
    pthread_key_delete(key);
}

template <typename T>
inline void ThreadLocalPtr<T>::SetTlsValue(void* rawValue) const DAVA_NOEXCEPT
{
    pthread_setspecific(key, rawValue);
}

template <typename T>
inline void* ThreadLocalPtr<T>::GetTlsValue() const DAVA_NOEXCEPT
{
    return pthread_getspecific(key);
}

#endif

} // namespace DAVA
