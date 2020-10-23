#ifndef __DAVAENGINE_INTERNALALLOCATOR_H__
#define __DAVAENGINE_INTERNALALLOCATOR_H__

#include "Base/BaseTypes.h"

#if defined(DAVA_MEMORY_PROFILING_ENABLE)

#include <cstddef>
#include <limits>

#include "MemoryManager/AllocatorBridge.h"

namespace DAVA
{
// Allocator for internal data MemoryManager's structures
template <typename T>
class InternalAllocator
{
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    template <typename U>
    struct rebind
    {
        typedef InternalAllocator<U> other;
    };

    InternalAllocator() = default;
    InternalAllocator(const InternalAllocator&) = default;
    template <typename U>
    InternalAllocator(const InternalAllocator<U>&) DAVA_NOEXCEPT
    {
    }
    ~InternalAllocator() = default;

    size_type max_size() const DAVA_NOEXCEPT
    {
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    pointer address(reference ref) const DAVA_NOEXCEPT
    {
        return std::addressof(ref);
    }

    const_pointer address(const_reference cref) const DAVA_NOEXCEPT
    {
        return std::addressof(cref);
    }

    pointer allocate(size_type n, std::allocator<void>::const_pointer hint = 0)
    {
        void* ptr = InternalAlloc(n * sizeof(T));
        if (ptr != nullptr)
        {
            return static_cast<pointer>(ptr);
        }
        throw std::bad_alloc();
    }

    void deallocate(pointer ptr, size_type n)
    {
        InternalDealloc(ptr);
    }

    void construct(pointer ptr, const_reference value)
    {
        ::new (static_cast<void*>(ptr)) T(value);
    }

    template <typename U, typename... Args>
    void construct(U* ptr, Args&&... args)
    {
        ::new (static_cast<void*>(ptr)) U(std::forward<Args>(args)...);
    }

    void destroy(pointer ptr)
    {
        ptr->~T();
    }

    template <typename U>
    void destroy(U* ptr)
    {
        ptr->~U();
    }
};

//////////////////////////////////////////////////////////////////////////
template <typename T1, typename T2>
inline bool operator==(const InternalAllocator<T1>&, const InternalAllocator<T2>&)
{
    return true; // InternalAllocator is stateless so two allocators are always equal
}

template <typename T1, typename T2>
inline bool operator!=(const InternalAllocator<T1>&, const InternalAllocator<T2>&)
{
    return false;
}

} // namespace DAVA

#endif // defined(DAVA_MEMORY_PROFILING_ENABLE)

#endif // __DAVAENGINE_INTERNALALLOCATOR_H__
