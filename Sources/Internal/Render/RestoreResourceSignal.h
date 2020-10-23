#pragma once 

#include "Concurrency/Mutex.h"
#include "Concurrency/LockGuard.h"
#include "Base/Token.h"
#include "Functional/Signal.h"

namespace DAVA
{
//thread-safe wrapper for restore-resources signal
class RestoreResourceSignal
{
public:
    template <typename Obj, typename Cls>
    inline Token Connect(Obj* obj, void (Cls::*const& fn)(), Signal<>::Group group = Signal<>::Group::Medium)
    {
        LockGuard<Mutex> guard(mutex);
        return signal.Connect<Obj, Cls>(obj, fn, group);
    }

    template <typename Obj, typename Cls>
    inline Token Connect(Obj* obj, void (Cls::*const& fn)() const, Signal<>::Group group = Signal<>::Group::Medium)
    {
        LockGuard<Mutex> guard(mutex);
        return signal.Connect<Obj, Cls>(obj, fn, group);
    }

    template <typename Fn>
    inline Token Connect(const Fn& fn, Signal<>::Group group = Signal<>::Group::Medium)
    {
        LockGuard<Mutex> guard(mutex);
        return signal.Connect<Fn>(fn, group);
    }

    inline void Disconnect(void* obj)
    {
        LockGuard<Mutex> guard(mutex);
        signal.Disconnect(obj);
    }

    inline void Disconnect(Token token)
    {
        LockGuard<Mutex> guard(mutex);
        signal.Disconnect(token);
    }

    inline void Emit()
    {
        signal.Emit();
    }

protected:
    Mutex mutex;
    Signal<> signal;
};

} //ns
