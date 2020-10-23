#ifndef __DAVAENGINE_SCOPED_PTR_H__
#define __DAVAENGINE_SCOPED_PTR_H__

#include "Base/BaseTypes.h"

namespace DAVA
{
template <typename BASE_OBJECT>
class ScopedPtr
{
public:
    explicit ScopedPtr(BASE_OBJECT* p);
    ScopedPtr();
    ScopedPtr(std::nullptr_t);
    ScopedPtr(const ScopedPtr&);

    ~ScopedPtr();

    BASE_OBJECT& operator*() const;
    BASE_OBJECT* operator->() const;
    operator BASE_OBJECT*() const;
    BASE_OBJECT* get() const;
    void reset(BASE_OBJECT* p = nullptr);
    explicit operator bool() const;

    //protection from 'delete ScopedObject'
    operator void*() const;

    const ScopedPtr& operator=(const ScopedPtr&);
    const ScopedPtr& operator=(BASE_OBJECT* p);

private:
    BASE_OBJECT* object = nullptr;
};

//implementation
template <typename BASE_OBJECT>
ScopedPtr<BASE_OBJECT>::ScopedPtr(BASE_OBJECT* p)
    : object(p)
{
}

template <typename BASE_OBJECT>
ScopedPtr<BASE_OBJECT>::ScopedPtr()
    : object(nullptr)
{
}

template <typename BASE_OBJECT>
ScopedPtr<BASE_OBJECT>::ScopedPtr(std::nullptr_t)
    : object(nullptr)
{
}

template <typename BASE_OBJECT>
ScopedPtr<BASE_OBJECT>::ScopedPtr(const ScopedPtr& scopedPtr)
{
    object = SafeRetain(scopedPtr.object);
}

template <typename BASE_OBJECT>
const ScopedPtr<BASE_OBJECT>& ScopedPtr<BASE_OBJECT>::operator=(const ScopedPtr& scopedPtr)
{
    if (this == &scopedPtr)
    {
        return *this;
    }

    if (object != scopedPtr.object)
    {
        SafeRelease(object);
        object = SafeRetain(scopedPtr.object);
    }

    return *this;
}

template <typename BASE_OBJECT>
const ScopedPtr<BASE_OBJECT>& ScopedPtr<BASE_OBJECT>::operator=(BASE_OBJECT* p)
{
    if (p != object)
    {
        SafeRelease(object);
        object = p;
    }

    return *this;
}

template <typename BASE_OBJECT>
ScopedPtr<BASE_OBJECT>::~ScopedPtr()
{
    SafeRelease(object);
}

template <typename BASE_OBJECT>
BASE_OBJECT& ScopedPtr<BASE_OBJECT>::operator*() const
{
    return *object;
}

template <typename BASE_OBJECT>
BASE_OBJECT* ScopedPtr<BASE_OBJECT>::operator->() const
{
    return object;
}

template <typename BASE_OBJECT>
ScopedPtr<BASE_OBJECT>::operator BASE_OBJECT*() const
{
    return object;
}

template <typename BASE_OBJECT>
BASE_OBJECT* ScopedPtr<BASE_OBJECT>::get() const
{
    return object;
}

template <typename BASE_OBJECT>
void ScopedPtr<BASE_OBJECT>::reset(BASE_OBJECT* p)
{
    if (p != object)
    {
        SafeRelease(object);
        object = p;
    }
}

template <typename BASE_OBJECT>
ScopedPtr<BASE_OBJECT>::operator bool() const
{
    return object != nullptr;
}

template <typename BASE_OBJECT>
ScopedPtr<BASE_OBJECT>::operator void*() const
{
    return object;
}
};

#endif //__DAVAENGINE_SCOPED_PTR_H__
