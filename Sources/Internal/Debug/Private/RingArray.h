#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
template <class T>
class RingArray
{
public:
    RingArray() = default;

    RingArray(std::size_t _size)
    {
        elementsCount = _size;
        elements = new T[elementsCount];
    }
    ~RingArray()
    {
        SafeDeleteArray(elements);
    }
    RingArray(const RingArray& a)
    {
        elementsCount = a.elementsCount;
        head = a.head;
        if (elementsCount)
        {
            elements = new T[elementsCount];
            std::copy(a.elements, a.elements + elementsCount, elements);
        }
        else
        {
            elements = nullptr;
        }
    }
    RingArray(RingArray&& a)
    {
        elements = a.elements;
        a.elements = nullptr;

        elementsCount = a.elementsCount;
        head = a.head;
    }
    RingArray& operator=(RingArray&& a)
    {
        if (this != &a)
        {
            elements = a.elements;
            a.elements = nullptr;

            elementsCount = a.elementsCount;
            head = a.head;
        }
        return *this;
    }
    RingArray& operator=(const RingArray& a)
    {
        if (this != &a)
        {
            SafeDeleteArray(elements);
            new (this) RingArray(a);
        }
        return (*this);
    }

    class iterator;
    class reverse_iterator;
    class const_iterator;
    class const_reverse_iterator;

    inline T& next()
    {
        head = (head + 1) % elementsCount;
        return elements[head];
    }
    inline iterator begin()
    {
        return iterator(elements, (head + 1) % elementsCount, elementsCount, false);
    }
    inline iterator end()
    {
        return iterator(elements, (head + 1) % elementsCount, elementsCount, true);
    }
    inline reverse_iterator rbegin()
    {
        return reverse_iterator(elements, head, elementsCount, true);
    }
    inline reverse_iterator rend()
    {
        return reverse_iterator(elements, head, elementsCount, false);
    }

    inline const_iterator cbegin() const
    {
        return const_iterator(elements, (head + 1) % elementsCount, elementsCount, false);
    }
    inline const_iterator cend() const
    {
        return const_iterator(elements, (head + 1) % elementsCount, elementsCount, true);
    }
    inline const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(elements, head, elementsCount, true);
    }
    inline const_reverse_iterator crend() const
    {
        return const_reverse_iterator(elements, head, elementsCount, false);
    }

    inline const_iterator begin() const
    {
        return cbegin();
    }
    inline const_iterator end() const
    {
        return cend();
    }
    inline const_reverse_iterator rbegin() const
    {
        return crbegin();
    }
    inline const_reverse_iterator rend() const
    {
        return crend();
    }

    inline std::size_t size() const
    {
        return elementsCount;
    }

protected:
    class base_iterator
    {
    public:
        base_iterator() = default;
        ~base_iterator() = default;

        inline bool operator==(const base_iterator& it) const
        {
            return (index == it.index) && (arrayData == it.arrayData) && (looped == it.looped);
        }
        inline bool operator!=(const base_iterator& it) const
        {
            return !(*this == it);
        }
        inline T& ref() const
        {
            return arrayData[index];
        }
        inline T* ptr() const
        {
            return &arrayData[index];
        }

        inline void add(size_t n)
        {
            index += n;
            looped = looped ? true : index >= elementsCount;
            index %= elementsCount;
        }
        inline void radd(size_t n)
        {
            looped = looped ? (index >= n) : false;
            index += elementsCount - n;
            index %= elementsCount;
        }
        inline void sub(size_t n)
        {
            looped = looped ? true : (index < n);
            index += elementsCount - n;
            index %= elementsCount;
        }
        inline void rsub(size_t n)
        {
            index += n;
            looped = looped ? (index <= elementsCount) : false;
            index %= elementsCount;
        }

        base_iterator(T* data, std::size_t _index, std::size_t _count, bool _looped)
            : arrayData(data)
            , index(_index)
            , elementsCount(_count)
            , looped(_looped)
        {
        }

        T* arrayData = nullptr;
        std::size_t index = 0;
        std::size_t elementsCount = 0;
        bool looped = false;
    };

public:
    class iterator : public base_iterator
    {
    public:
        inline iterator operator+(uint32 n) const
        {
            iterator it(*this);
            it.add(n);
            return it;
        }
        inline iterator operator-(uint32 n) const
        {
            iterator it(*this);
            it.sub(n);
            return it;
        }
        inline iterator& operator++()
        {
            this->add(1);
            return *this;
        }
        inline iterator operator++(int)
        {
            iterator prev = *this;
            ++(*this);
            return prev;
        }
        inline iterator& operator--()
        {
            this->sub(1);
            return *this;
        }
        inline iterator operator--(int)
        {
            iterator prev = *this;
            --(*this);
            return prev;
        }
        inline T& operator*() const
        {
            return this->ref();
        }
        inline T* operator->() const
        {
            return this->ptr();
        }

    protected:
        iterator(T* data, std::size_t _index, std::size_t _count, bool _looped)
            : base_iterator(data, _index, _count, _looped)
        {
        }

        friend class RingArray;
    };

    class reverse_iterator : public base_iterator
    {
    public:
        inline reverse_iterator operator+(uint32 n) const
        {
            reverse_iterator it(*this);
            it.radd(n);
            return it;
        }
        inline reverse_iterator operator-(uint32 n) const
        {
            reverse_iterator it(*this);
            it.rsub(n);
            return it;
        }
        inline reverse_iterator& operator++()
        {
            this->radd(1);
            return *this;
        }
        inline reverse_iterator operator++(int)
        {
            reverse_iterator prev = *this;
            ++(*this);
            return prev;
        }
        inline reverse_iterator& operator--()
        {
            this->rsub(1);
            return *this;
        }
        inline reverse_iterator operator--(int)
        {
            reverse_iterator prev = *this;
            --(*this);
            return prev;
        }
        inline T& operator*() const
        {
            return this->ref();
        }
        inline T* operator->() const
        {
            return this->ptr();
        }

    protected:
        reverse_iterator(T* data, std::size_t _index, std::size_t _count, bool _looped)
            : base_iterator(data, _index, _count, _looped)
        {
        }

        friend class RingArray;
    };

    class const_iterator : public base_iterator
    {
    public:
        inline const_iterator operator+(uint32 n) const
        {
            const_iterator it(*this);
            it.add(n);
            return it;
        }
        inline const_iterator operator-(uint32 n) const
        {
            const_iterator it(*this);
            it.sub(n);
            return it;
        }
        inline const_iterator& operator++()
        {
            this->add(1);
            return *this;
        }
        inline const_iterator operator++(int)
        {
            const_iterator prev = *this;
            ++(*this);
            return prev;
        }
        inline const_iterator& operator--()
        {
            this->sub(1);
            return *this;
        }
        inline const_iterator operator--(int)
        {
            const_iterator prev = *this;
            --(*this);
            return prev;
        }
        inline const T& operator*() const
        {
            return this->ref();
        }
        inline const T* operator->() const
        {
            return this->ptr();
        }

    protected:
        const_iterator(T* data, std::size_t _index, std::size_t _count, bool _looped)
            : base_iterator(data, _index, _count, _looped)
        {
        }

        friend class RingArray;
    };

    class const_reverse_iterator : public base_iterator
    {
    public:
        inline const_reverse_iterator operator+(uint32 n) const
        {
            const_reverse_iterator it(*this);
            it.radd(n);
            return it;
        }
        inline const_reverse_iterator operator-(uint32 n) const
        {
            const_reverse_iterator it(*this);
            it.rsub(n);
            return it;
        }
        inline const_reverse_iterator& operator++()
        {
            this->radd(1);
            return *this;
        }
        inline const_reverse_iterator operator++(int)
        {
            const_reverse_iterator prev = *this;
            ++(*this);
            return prev;
        }
        inline const_reverse_iterator& operator--()
        {
            this->rsub(1);
            return *this;
        }
        inline const_reverse_iterator operator--(int)
        {
            const_reverse_iterator prev = *this;
            --(*this);
            return prev;
        }
        inline const T& operator*() const
        {
            return this->ref();
        }
        inline const T* operator->() const
        {
            return this->ptr();
        }

    protected:
        const_reverse_iterator(T* data, std::size_t _index, std::size_t _count, bool _looped)
            : base_iterator(data, _index, _count, _looped)
        {
        }

        friend class RingArray;
    };

protected:
    T* elements = nullptr;
    std::size_t elementsCount = 0;
    std::size_t head = 0;
};

}; //ns
