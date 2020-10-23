#pragma once

#include "Base/BaseTypes.h"
#include "Math/MathHelpers.h"
#include "Debug/DVAssert.h"
#include <atomic>

namespace DAVA
{
//////////////////////////////////////////////////////////////////////////
//'pseudo-thread-safe' ring array. It's provide only thread-safe obtaining
// of reference to element. So, don't write to array in parallel with
// array iterating.
// It's made for performance reasons.
//////////////////////////////////////////////////////////////////////////

template <class T>
class ProfilerRingArray
{
public:
    ProfilerRingArray(uint32 _size)
    {
        DVASSERT(IsPowerOf2(_size) && "Size of RingArray should be pow of two");
        elementsCount = _size;
        mask = elementsCount - 1;
        elements = new T[elementsCount];
    }
    ~ProfilerRingArray()
    {
        SafeDeleteArray(elements);
    }
    ProfilerRingArray(const ProfilerRingArray& a)
    {
        elementsCount = a.elementsCount;
        elements = new T[elementsCount];
        memcpy(elements, a.elements, elementsCount * sizeof(T));
        mask = a.mask;
        head = a.head.load();
    }
    ProfilerRingArray& operator=(const ProfilerRingArray& a)
    {
        if (this != &a)
        {
            SafeDeleteArray(elements);
            new (this) ProfilerRingArray(a);
        }
        return (*this);
    }

    class iterator;
    class const_iterator;
    class reverse_iterator;
    class const_reverse_iterator;

    T& next()
    {
        return elements[head++ & mask];
    }
    iterator begin()
    {
        return iterator(elements, (head & mask), mask);
    }
    const_iterator begin() const
    {
        return const_iterator(elements, (head & mask), mask);
    }
    iterator end()
    {
        return iterator(elements, (head & mask) | (mask + 1), mask);
    }
    const_iterator end() const
    {
        return const_iterator(elements, (head & mask) | (mask + 1), mask);
    }
    reverse_iterator rbegin()
    {
        return reverse_iterator(elements, ((head - 1) & mask) | (mask + 1), mask);
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(elements, ((head - 1) & mask) | (mask + 1), mask);
    }
    reverse_iterator rend()
    {
        return reverse_iterator(elements, (head - 1) & mask, mask);
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(elements, (head - 1) & mask, mask);
    }
    size_t size() const
    {
        return elementsCount;
    }

private:
    template <typename U>
    class base_iterator
    {
    public:
        base_iterator() = default;
        base_iterator(U* data, uint32 _index, uint32 _mask)
            : arrayData(data)
            , index(_index)
            , mask(_mask)
        {
        }
        ~base_iterator() = default;

        bool operator==(const base_iterator& it) const
        {
            return (index == it.index) && (arrayData == it.arrayData);
        }
        bool operator!=(const base_iterator& it) const
        {
            return !(*this == it);
        }
        U& operator*() const
        {
            return arrayData[index & mask];
        }
        U* operator->() const
        {
            return &arrayData[index & mask];
        }

        U* arrayData = nullptr;
        uint32 index = 0;
        uint32 mask = 0;
    };

public:
    class iterator final : public base_iterator<T>
    {
    public:
        iterator(T* data, uint32 _index, uint32 _mask)
            : base_iterator<T>(data, _index, _mask)
        {
        }
        iterator(const reverse_iterator& it)
        {
            this->arrayData = it.arrayData;
            this->mask = it.mask;
            this->index = it.index;
        }
        operator reverse_iterator() const
        {
            return reverse_iterator(this->arrayData, this->index, this->mask);
        }
        iterator operator+(uint32 n) const
        {
            iterator it(*this);
            it.index += n;
            return it;
        }
        iterator operator-(uint32 n) const
        {
            iterator it(*this);
            it.index -= n;
            return it;
        }
        iterator& operator++()
        {
            ++(this->index);
            return *this;
        }
        iterator operator++(int)
        {
            iterator prev = *this;
            ++(*this);
            return prev;
        }
        iterator& operator--()
        {
            --(this->index);
            return *this;
        }
        iterator operator--(int)
        {
            iterator prev = *this;
            --(*this);
            return prev;
        }
    };

    class const_iterator final : public base_iterator<const T>
    {
    public:
        const_iterator(const T* data, uint32 _index, uint32 _mask)
            : base_iterator<const T>(data, _index, _mask)
        {
        }
        const_iterator(const const_reverse_iterator& it)
        {
            this->arrayData = it.arrayData;
            this->mask = it.mask;
            this->index = it.index;
        }
        operator const_reverse_iterator() const
        {
            return const_reverse_iterator(this->arrayData, this->index, this->mask);
        }
        const_iterator operator+(uint32 n) const
        {
            const_iterator it(*this);
            it.index += n;
            return it;
        }
        const_iterator operator-(uint32 n) const
        {
            const_iterator it(*this);
            it.index -= n;
            return it;
        }
        const_iterator& operator++()
        {
            ++(this->index);
            return *this;
        }
        const_iterator operator++(int)
        {
            const_iterator prev = *this;
            ++(*this);
            return prev;
        }
        const_iterator& operator--()
        {
            --(this->index);
            return *this;
        }
        const_iterator operator--(int)
        {
            const_iterator prev = *this;
            --(*this);
            return prev;
        }
    };

    class reverse_iterator final : public base_iterator<T>
    {
    public:
        reverse_iterator(T* data, uint32 _index, uint32 _mask)
            : base_iterator<T>(data, _index, _mask)
        {
        }
        reverse_iterator(const iterator& it)
        {
            this->arrayData = it.arrayData;
            this->mask = it.mask;
            this->index = it.index;
        }
        operator iterator() const
        {
            return iterator(this->arrayData, this->index, this->mask);
        }
        reverse_iterator operator+(uint32 n) const
        {
            reverse_iterator it(*this);
            it.index -= n;
            return it;
        }
        reverse_iterator operator-(uint32 n) const
        {
            reverse_iterator it(*this);
            it.index += n;
            return it;
        }
        reverse_iterator& operator++()
        {
            --(this->index);
            return *this;
        }
        reverse_iterator operator++(int)
        {
            reverse_iterator prev = *this;
            ++(*this);
            return prev;
        }
        reverse_iterator& operator--()
        {
            ++(this->index);
            return *this;
        }
        reverse_iterator operator--(int)
        {
            reverse_iterator prev = *this;
            --(*this);
            return prev;
        }
    };

    class const_reverse_iterator final : public base_iterator<const T>
    {
    public:
        const_reverse_iterator(const T* data, uint32 _index, uint32 _mask)
            : base_iterator<const T>(data, _index, _mask)
        {
        }
        const_reverse_iterator(const iterator& it)
        {
            this->arrayData = it.arrayData;
            this->mask = it.mask;
            this->index = it.index;
        }
        operator const_iterator() const
        {
            return const_iterator(this->arrayData, this->index, this->mask);
        }
        const_reverse_iterator operator+(uint32 n) const
        {
            const_reverse_iterator it(*this);
            it.index -= n;
            return it;
        }
        const_reverse_iterator operator-(uint32 n) const
        {
            reverse_iterator it(*this);
            it.index += n;
            return it;
        }
        const_reverse_iterator& operator++()
        {
            --(this->index);
            return *this;
        }
        const_reverse_iterator operator++(int)
        {
            reverse_iterator prev = *this;
            ++(*this);
            return prev;
        }
        const_reverse_iterator& operator--()
        {
            ++(this->index);
            return *this;
        }
        const_reverse_iterator operator--(int)
        {
            const_reverse_iterator prev = *this;
            --(*this);
            return prev;
        }
    };

private:
    T* elements = nullptr;
    uint32 elementsCount = 0;
    uint32 mask = 0;
    std::atomic<uint32> head = { 0 };
};

} // end namespace DAVA
