#pragma once

#include "Base/BaseTypes.h"
#include "Base/Platform.h"
#include <type_traits>

#ifdef USE_CPP11_CONCURRENCY
#include <atomic> //for std::atomic
#endif

namespace DAVA
{
//-----------------------------------------------------------------------------
//Atomic template class. DEPRECATED use std::atomic<T>
//-----------------------------------------------------------------------------
template <typename T>
class Atomic
{
    static_assert(std::is_integral<T>::value ||
                  std::is_pointer<T>::value ||
                  std::is_enum<T>::value,
                  "Not valid type for atomic operations");

public:
    Atomic(T val = T()) DAVA_NOEXCEPT : value(val)
    {
    }

    Atomic(const Atomic& other) = delete;
    Atomic& operator=(const Atomic& other) = delete;

    Atomic& operator=(T val) DAVA_NOEXCEPT;
    operator T() DAVA_NOEXCEPT
    {
        return Get();
    }

    void Set(T val) DAVA_NOEXCEPT;
    T Get() const DAVA_NOEXCEPT;
    T GetRelaxed() const DAVA_NOEXCEPT;

    T Increment() DAVA_NOEXCEPT;
    T Decrement() DAVA_NOEXCEPT;

    T operator++() DAVA_NOEXCEPT
    {
        return Increment();
    }
    T operator++(int)DAVA_NOEXCEPT
    {
        return Increment() - 1;
    }
    T operator--() DAVA_NOEXCEPT
    {
        return Decrement();
    }
    T operator--(int)DAVA_NOEXCEPT
    {
        return Decrement() + 1;
    }

    T Swap(T desired) DAVA_NOEXCEPT;
    bool CompareAndSwap(T expected, T desired) DAVA_NOEXCEPT;

private:

#ifdef USE_CPP11_CONCURRENCY
    std::atomic<T> value;
#else

#ifdef __DAVAENGINE_WINDOWS__
    template <typename Y>
    T Cast(Y val);
    T value;
#else
    DAVA_ALIGNED(T value, sizeof(T));
#endif

#endif
};

//-----------------------------------------------------------------------------
//Common realization
//-----------------------------------------------------------------------------
template <typename T>
Atomic<T>& Atomic<T>::operator=(T val) DAVA_NOEXCEPT
{
    Set(val);
    return *this;
}

} //  namespace DAVA

//-----------------------------------------------------------------------------
//Specific platform realization
//-----------------------------------------------------------------------------
#if defined(__DAVAENGINE_WINDOWS__) && !defined(USE_CPP11_CONCURRENCY)
#include "Concurrency/AtomicWindows.h"
#elif defined(__GNUC__) && !defined(USE_CPP11_CONCURRENCY)
#include "Concurrency/AtomicGNU.h"
#elif defined(USE_CPP11_CONCURRENCY)

namespace DAVA
{
//-----------------------------------------------------------------------------
//Atomic template class realization using std::atomic
//-----------------------------------------------------------------------------
template <typename T>
void Atomic<T>::Set(T val) DAVA_NOEXCEPT
{
    value = val;
}

template <typename T>
T Atomic<T>::Get() const DAVA_NOEXCEPT
{
    return value;
}

template <typename T>
T Atomic<T>::GetRelaxed() const DAVA_NOEXCEPT
{
    return value.load(std::memory_order_relaxed);
}

template <typename T>
T Atomic<T>::Increment() DAVA_NOEXCEPT
{
    return ++value;
}

template <typename T>
T Atomic<T>::Decrement() DAVA_NOEXCEPT
{
    return --value;
}

template <typename T>
T Atomic<T>::Swap(T desired) DAVA_NOEXCEPT
{
    return value.exchange(desired);
}

template <typename T>
bool Atomic<T>::CompareAndSwap(T expected, T desired) DAVA_NOEXCEPT
{
    return value.compare_exchange_strong(expected, desired);
}

} //  namespace DAVA

#endif
