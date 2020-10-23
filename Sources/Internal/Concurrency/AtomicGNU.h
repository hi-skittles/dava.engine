#pragma once

#ifndef USE_CPP11_CONCURRENCY

#include "Concurrency/Atomic.h"

namespace DAVA
{

#if defined(__GNUC__)

//-----------------------------------------------------------------------------
//Atomic template class realization using built-in intrisics
//-----------------------------------------------------------------------------
template <typename T>
void Atomic<T>::Set(T val) DAVA_NOEXCEPT
{
    __atomic_store(&value, &val, __ATOMIC_SEQ_CST);
}

template <typename T>
T Atomic<T>::Get() const DAVA_NOEXCEPT
{
    return __atomic_load_n(&value, __ATOMIC_SEQ_CST);
}

template <typename T>
T Atomic<T>::GetRelaxed() const DAVA_NOEXCEPT
{
    return __atomic_load_n(&value, __ATOMIC_RELAXED);
}

template <typename T>
T Atomic<T>::Increment() DAVA_NOEXCEPT
{
    return __sync_add_and_fetch(&value, 1);
}

template <typename T>
T Atomic<T>::Decrement() DAVA_NOEXCEPT
{
    return __sync_sub_and_fetch(&value, 1);
}

template <typename T>
T Atomic<T>::Swap(T desired) DAVA_NOEXCEPT
{
    return __atomic_exchange_n(&value, desired, __ATOMIC_SEQ_CST);
}

template <typename T>
bool Atomic<T>::CompareAndSwap(T expected, T desired) DAVA_NOEXCEPT
{
    return __atomic_compare_exchange(&value, &expected, &desired,
                                     false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

#endif //  __GNUC__

} //  namespace DAVA

#endif //  !USE_CPP11_CONCURRENCY
