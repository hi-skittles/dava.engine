#pragma once

#include "Concurrency/Mutex.h"
#include "Concurrency/UniqueLock.h"

namespace DAVA
{
//------------------------------------------------------------------------------
//Class for concurrent use of objects. Stored object protected by mutex
//------------------------------------------------------------------------------
template <typename T, typename MutexType = Mutex>
class ConcurrentObject
{
    friend class Accessor;

public:
    template <typename... Args>
    ConcurrentObject(Args&&... args)
        : object(std::forward<Args>(args)...)
    {
    }

    class Accessor
    {
    public:
        Accessor(ConcurrentObject& object)
            : objectRef(object.object)
            , guard(object.mutex)
        {
        }

        Accessor(Accessor&& other)
            : objectRef(other.objectRef)
            , guard(std::move(other.guard))
        {
        }

        T& operator*()
        {
            return objectRef;
        }
        T* operator->()
        {
            return &objectRef;
        }

    private:
        T& objectRef;
        UniqueLock<MutexType> guard;
    };

    Accessor GetAccessor()
    {
        return Accessor(*this);
    }

    T Load()
    {
        return *Accessor(*this);
    }
    void Store(const T& value)
    {
        *Accessor(*this) = value;
    }

private:
    T object;
    MutexType mutex;
};
}
