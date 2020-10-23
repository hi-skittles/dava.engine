#ifndef __DAVA_REF_PTR_H__
#define __DAVA_REF_PTR_H__

#include "Base/BaseObject.h"
#include "Base/Any.h"

namespace DAVA
{
/// reference pointer wrapper for BaseObject refcounted classes.
template <class T>
class RefPtr
{
public:
    RefPtr()
    {
    }

    RefPtr(std::nullptr_t)
    {
    }

    explicit RefPtr(T* p)
        : _ptr(p)
    {
    }

    /// reinitializes pointer without incrementing reference
    void Set(T* p)
    {
        T* tmp_ptr = _ptr;
        _ptr = p;
        SafeRelease(tmp_ptr);
    }

    ~RefPtr()
    {
        SafeRelease(_ptr);
    }

    RefPtr(const RefPtr& rp)
    {
        _ptr = rp._ptr;

        SafeRetain(_ptr);
    }

    RefPtr(RefPtr&& rp)
        : _ptr(rp._ptr)
    {
        rp._ptr = nullptr;
    }

    template <class Other>
    RefPtr(const RefPtr<Other>& rp)
    {
        _ptr = rp.Get();

        SafeRetain(_ptr);
    }

    template <class Other>
    RefPtr(RefPtr<Other>&& rp)
        : _ptr(rp._ptr)
    {
        rp._ptr = nullptr;
    }

    static RefPtr<T> ConstructWithRetain(T* p)
    {
        static_assert(std::is_base_of<BaseObject, T>::value, "RefPtr works only with classes, derived from BaseObject!");
        SafeRetain(p);
        return RefPtr<T>(p);
    }

    T* Get() const
    {
        return _ptr;
    }

    bool Valid() const
    {
        return _ptr != nullptr;
    }

    RefPtr& operator=(const RefPtr& rp)
    {
        Assign(rp);
        return *this;
    }

    RefPtr& operator=(RefPtr&& rp)
    {
        Assign(std::move(rp));
        return *this;
    }

    template <class Other>
    RefPtr& operator=(const RefPtr<Other>& rp)
    {
        Assign(rp);
        return *this;
    }

    template <class Other>
    RefPtr& operator=(RefPtr<Other>&& rp)
    {
        Assign(std::move(rp));
        return *this;
    }

    RefPtr& operator=(T* ptr)
    {
        if (_ptr == ptr)
            return *this;

        T* tmp_ptr = _ptr;
        _ptr = ptr;
        SafeRetain(_ptr);
        SafeRelease(tmp_ptr);
        return *this;
    }

    RefPtr& operator=(std::nullptr_t)
    {
        RefPtr().Swap(*this);
        return *this;
    }

    bool operator<(const RefPtr& other) const
    {
        return _ptr < other._ptr;
    }

    /// implicit output conversion
    //operator T*() const { return _ptr; }

    T& operator*() const
    {
        return *_ptr;
    }
    T* operator->() const
    {
        return _ptr;
    }

    bool operator==(const RefPtr& rp) const
    {
        return _ptr == rp._ptr;
    }
    bool operator==(const T* ptr) const
    {
        return _ptr == ptr;
    }
    bool operator==(std::nullptr_t) const
    {
        return _ptr == nullptr;
    }
    friend bool operator==(const T* ptr, const RefPtr& rp)
    {
        return ptr == rp._ptr;
    }
    friend bool operator==(std::nullptr_t, const RefPtr& rp)
    {
        return nullptr == rp._ptr;
    }
    bool operator!=(const RefPtr& rp) const
    {
        return _ptr != rp._ptr;
    }
    bool operator!=(const T* ptr) const
    {
        return _ptr != ptr;
    }
    bool operator!=(std::nullptr_t) const
    {
        return _ptr != nullptr;
    }
    friend bool operator!=(const T* ptr, const RefPtr& rp)
    {
        return ptr != rp._ptr;
    }
    friend bool operator!=(std::nullptr_t, const RefPtr& rp)
    {
        return nullptr != rp._ptr;
    }
    bool operator!() const // Enables "if (!sp) ..."
    {
        return _ptr == nullptr;
    }

    template <typename... Arg>
    void ConstructInplace(Arg&&... arg)
    {
        Set(new T(std::forward<Arg>(arg)...));
    }

    void Swap(RefPtr& rp)
    {
        std::swap(_ptr, rp._ptr);
    }

private:
    class Tester
    {
        void operator delete(void*);
    };

public:
    operator Tester*() const
    {
        if (!_ptr)
            return nullptr;
        static Tester test;
        return &test;
    }

private:
    T* _ptr = nullptr;

    template <class Other>
    friend class RefPtr;

    template <class Other>
    void Assign(const RefPtr<Other>& rp)
    {
        if (_ptr == rp._ptr)
            return;

        T* tmp_ptr = _ptr;
        _ptr = rp.Get();
        SafeRetain(_ptr);
        SafeRelease(tmp_ptr);
    }

    template <class Other>
    void Assign(RefPtr<Other>&& rp)
    {
        if (_ptr == rp._ptr)
            return;

        RefPtr(std::move(rp)).Swap(*this);
    }
};

template <typename T>
struct AnyCompare<RefPtr<T>>
{
    static bool IsEqual(const Any& v1, const Any& v2)
    {
        const RefPtr<T>& s1 = v1.Get<RefPtr<T>>();
        const RefPtr<T>& s2 = v2.Get<RefPtr<T>>();
        return s1 == s2;
    }
};
} // ns

namespace std
{
template <class T>
void swap(DAVA::RefPtr<T>& left, DAVA::RefPtr<T>& right) noexcept
{
    left.Swap(right);
}
}

#endif // __DAVA_REF_PTR_H__
