#pragma once

namespace DAVA
{
using SignalTokenProvider = TokenProvider<Signal<>>;

namespace SignalDetail
{
template <typename T, bool>
struct TrackedObjectCaster
{
    static TrackedObject* Cast(void* t)
    {
        return nullptr;
    }
};

template <typename T>
struct TrackedObjectCaster<T, true>
{
    static TrackedObject* Cast(T* obj)
    {
        return static_cast<TrackedObject*>(obj);
    }
};

template <typename T>
TrackedObject* GetTrackedObject(T* obj)
{
    return TrackedObjectCaster<T, std::is_base_of<TrackedObject, T>::value>::Cast(obj);
}
} // namespace SignalDetail

template <typename... Args>
Signal<Args...>::Signal()
{
    connectionsMediumPos = connections.end();
}

template <typename... Args>
Signal<Args...>::~Signal()
{
    DisconnectAll();
}

template <typename... Args>
template <typename Fn>
inline Token Signal<Args...>::Connect(const Fn& fn, Group group)
{
    Token token = SignalTokenProvider::Generate();

    Signal::Connection c;
    c.fn = ConnectionFn(fn);
    c.object = nullptr;
    c.token = token;
    c.tracked = nullptr;
    AddSlot(std::move(c), group);

    return token;
}

template <typename... Args>
template <typename Obj, typename Fn>
inline Token Signal<Args...>::Connect(Obj* obj, const Fn& fn, Group group)
{
    Token token = SignalTokenProvider::Generate();

    Signal::Connection c;
    c.fn = ConnectionFn(fn);
    c.object = obj;
    c.token = token;
    c.tracked = SignalDetail::GetTrackedObject(obj);
    AddSlot(std::move(c), group);

    return token;
}

template <typename... Args>
template <typename Obj, typename Cls>
inline Token Signal<Args...>::Connect(Obj* obj, void (Cls::*const& fn)(Args...), Group group)
{
    Token token = SignalTokenProvider::Generate();

    Signal::Connection c;
    c.fn = ConnectionFn(obj, fn);
    c.object = obj;
    c.token = token;
    c.tracked = SignalDetail::GetTrackedObject(obj);
    AddSlot(std::move(c), group);

    return token;
}

template <typename... Args>
template <typename Obj, typename Cls>
inline Token Signal<Args...>::Connect(Obj* obj, void (Cls::*const& fn)(Args...) const, Group group)
{
    Token token = SignalTokenProvider::Generate();

    Signal::Connection c;
    c.fn = ConnectionFn(obj, fn);
    c.object = obj;
    c.token = token;
    c.tracked = SignalDetail::GetTrackedObject(obj);
    AddSlot(std::move(c), group);

    return token;
}

template <typename... Args>
void Signal<Args...>::OnTrackedObjectDestroyed(TrackedObject* obj)
{
    Disconnect(obj);
}

template <typename... Args>
void Signal<Args...>::AddSlot(Connection&& c, Group group)
{
    if (nullptr != c.tracked)
    {
        Watch(c.tracked);
    }

    // now search a place for connection
    // depending on given group
    if (Group::High == group)
    {
        // for High priority just place it front
        connections.push_front(std::move(c));
    }
    else if (Group::Medium == group)
    {
        // for Medium insert in special tracked position
        connections.insert(connectionsMediumPos, std::move(c));
    }
    else
    {
        // for Low priority just place it back
        connections.push_back(std::move(c));

        // set new position for medium priority
        // it should be less than first low
        if (connectionsMediumPos == connections.end())
        {
            connectionsMediumPos--;
        }
    }
}

template <typename... Args>
typename Signal<Args...>::ConnectionIt Signal<Args...>::RemoveSlot(ConnectionIt& it)
{
    if (!it->flags.test(Connection::Deleted))
    {
        if (nullptr != it->tracked)
        {
            Unwatch(it->tracked);
            it->tracked = nullptr;
        }

        it->object = nullptr;
        it->flags.set(Connection::Deleted, true);
    }

    // We shouldn't really erase specified by 'it' connection
    // if it's in Emitting state. This must be done in order
    // not to break Emit() processing cycle with wrond iterator.
    // (avoiding crash when we are erasing list item by iterator
    // and that performing operator++() under the same iterator)
    if (!it->flags.test(Connection::Emiting))
    {
        // not in Emitting state: we can safely
        // erase it and return next one
        return connections.erase(it);
    }
    else
    {
        // in Emiting state: it was marked as Deleted
        // so just return next one
        return ++it;
    }
}

template <typename... Args>
void Signal<Args...>::Disconnect(Token token)
{
    DVASSERT(SignalTokenProvider::IsValid(token));

    auto it = connections.begin();
    auto end = connections.end();
    for (; it != end; it++)
    {
        if (it->token == token)
        {
            RemoveSlot(it);
            break;
        }
    }
}

template <typename... Args>
void Signal<Args...>::Disconnect(void* obj)
{
    DVASSERT(nullptr != obj);

    auto it = connections.begin();
    auto end = connections.end();
    for (; it != end;)
    {
        if (it->object == obj || it->tracked == obj)
        {
            it = RemoveSlot(it);
        }
        else
        {
            it++;
        }
    }
}

template <typename... Args>
void Signal<Args...>::DisconnectAll()
{
    auto it = connections.begin();
    auto end = connections.end();
    for (; it != end;)
    {
        it = RemoveSlot(it);
    }
}

template <typename... Args>
void Signal<Args...>::Track(Token token, TrackedObject* tracked)
{
    DVASSERT(SignalTokenProvider::IsValid(token));
    DVASSERT(nullptr != tracked);

    auto it = connections.rbegin();
    auto rend = connections.rend();
    for (; it != rend; ++it)
    {
        if (it->token == token && it->tracked != tracked)
        {
            if (nullptr != it->tracked)
                Unwatch(it->tracked);

            it->tracked = tracked;
            Watch(tracked);
        }
    }
}

template <typename... Args>
void Signal<Args...>::Block(Token token, bool block)
{
    DVASSERT(SignalTokenProvider::IsValid(token));

    for (auto& c : connections)
    {
        if (c.token == token)
        {
            c.flags.set(Connection::Blocked, block);
            break;
        }
    }
}

template <typename... Args>
void Signal<Args...>::Block(void* obj, bool block)
{
    for (auto& c : connections)
    {
        if (c.object == obj)
        {
            c.blocked = block;
        }
    }
}

template <typename... Args>
bool Signal<Args...>::IsBlocked(Token token) const
{
    DVASSERT(SignalTokenProvider::IsValid(token));

    for (auto& c : connections)
    {
        return c.flags.test(Connection::Blocked);
    }

    return false;
}

template <typename... Args>
void Signal<Args...>::Emit(Args... args)
{
    auto it = connections.begin();
    auto end = connections.end();

    while (it != end)
    {
        if (!it->flags.test(Connection::Blocked))
        {
            it->flags.set(Connection::Emiting, true);
            it->fn(args...);
            it->flags.set(Connection::Emiting, false);
        }

        if (it->flags.test(Connection::Deleted))
        {
            // erase slots that are already marked as 'deleted'
            // see RemoveSlot() function for more description
            it = RemoveSlot(it);
        }
        else
        {
            it++;
        }
    }
}
} // namespace DAVA
