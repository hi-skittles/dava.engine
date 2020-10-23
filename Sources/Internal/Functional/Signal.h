#pragma once

#include "Debug/DVAssert.h"
#include "Base/List.h"
#include "Base/Token.h"
#include "Functional/Function.h"
#include "Functional/TrackedObject.h"
#include "Functional/Private/SignalBase.h"

namespace DAVA
{
/**
    \ingroup functional
    Signal represents callback with multiple targets. Signal is connected to some set of slots,
    which are callback receivers (also called event targets or subscribers), which are called
    when the signal is "emitted."

    Signals and slots can be managed - can track connections and are capable of automatically
    disconnecting signal/slot connections when either is destroyed. This enables the user
    to make signal/slot connections without expending a great effort to manage the lifetimes
    of those connections with regard to the lifetimes of all objects involved.
    See TrackedObject for more info.


    ## Examples

    ### Hello World

    The following example writes "Hello, World!" using signals and slots. First, we create a signal `sig`,
    a signal that takes no arguments. Next, we connect the `HelloWorld::foo` function to the signal using the
    Signal::Connect() method. Finally, use the signal `sig` like a function to call the slots, which in
    turns invokes HelloWorld::operator() to print "Hello, World!".

    \code
    struct HelloWorld
    {
        void foo() const { std::cout << "Hello, World!\n"; }
    };

    void main()
    {
        // Signal with no arguments and a void return value
        DAVA::Signal<void ()> sig;

        // Connect a HelloWorld slot
        HelloWorld hello;
        sig.Connect(&hello, &HelloWorld::foo);

        // Call all of the slots
        sig.Emit();
    }
    \endcode


    ### Connecting Multiple Slots

    Calling a single slot from a signal isn't very interesting, so we can make the Hello, World program more
    interesting by splitting the work of printing "Hello, World!" into two completely separate slots.
    The first slot will print "Hello" and the second slot will print ", World!".

    \code
    struct Hello
    {
        void foo() const { std::cout << "Hello\n"; }
    };

    struct World
    {
        void foo() const { std::cout << "World\n"; }
    };

    void main()
    {
        Hello hello;
        World world;

        // Signal with no arguments and a void return value
        DAVA::Signal<void ()> sig;

        sig.Connect(&hello, &Hello::foo);
        sig.Connect(&world, &World::foo);

        // Call all of the slots
        sig.Emit();
    }
    \endcode

    One should know, that each Connect(...) function returns an unique slot identifier - Token. It can be used
    to control slot was that created by appropriate Connect() call. See "Disconnecting Slots" or "Blocking Slots" for
    examples.


    ### Ordering Slots

    Slots are free to have side effects, and that can mean that some slots will have to be called before others
    even if they are not connected in that order. The DAVA::Signal allows slots to be placed into groups that are
    ordered in some way. For our Hello, World program, we want "Hello" to be printed before ", World!",
    so we put "Hello" into a group that must be executed before the group that ", World!" is in.
    To do this, we can supply an extra parameter to the end of the connect call that specifies the group.
    Group value is one of the following three values: Signal::Group::High, Signal::Group::Medium, Signal::Group::Low.

    Here's how we construct Hello, World:
    \code
    void main()
    {
        ...

        DAVA::Signal<void ()> sig;

        sig.Connect(&world, &World::foo, Signal::Group::Low);
        sig.Connect(&hello, &Hello::foo, Signal::Group::High);

        // slots are invoked this order:
        // 1) slots from group Signal::Group::High
        // 2) slots from group Signal::Group::Medium
        // 3) slots from group Signal::Group::Low
        }
    \endcode

    Invoking the signal will correctly print "Hello World", because the `Hello::foo` method is in group Signal::Group::High,
    which precedes group Signal::Group::Low where the `World::foo` method resides. The `group` parameter is optional,
    Signal::Group::Medium will be used by default.

    ### Disconnecting Slots

    Slots aren't expected to exist indefinitely after they are connected. Often slots are only used to receive a few events
    and are then disconnected, and the programmer needs control to decide when a slot should no longer be connected.

    \code
    void main()
    {
        Hello hello;
        World world;

        // Signal with no arguments and a void return value
        DAVA::Signal<void ()> sig;

        sig.Connect(hello, &Hello::foo);

        sig.Connect(world, &World::foo);

        // connect to global function `void goo()`
        Token gooToken = sig.Connect(&goo);

        // Call all of the slots - Hello::foo, World::foo and goo
        sig.Emit();

        // disconnect all slots that were connected with &hello object
        sig.Disconnect(&hello);

        // Disconnect slot, that is identified by `token`
        sig.Disconnect(gooToken);

        // Call all of the remaining slots - World::foo
        sig.Emit();

        // Disconnect all connected slots
        sig.DisconnectAll();
    }
    \endcode


    ### Blocking Slots

    Slots can be temporarily "blocked", meaning that they will be ignored when the signal is emited but has not
    been permanently disconnected. This is typically used to prevent infinite recursion in cases where otherwise
    running a slot would cause the signal it is connected to be invoked again.
    Here is an example of blocking/unblocking slots:

    \code
    void main()
    {
        sig.Connect(&hello, &Hello::foo);
        sig.Connect(&world, &World::foo);
        sig.Emit(); // Prints "Hello" and "World"

        sig.Block(&hello, true);
        sig.Emit(); // Prints "World"

        sig.Block(&hello, false);
        sig.Emit(); // Prints "Hello" and "World"
    }
    \endcode


    ### Automatic Connection Management

    Signals can automatically track the lifetime of objects involved in signal/slot connections, including automatic
    disconnection of slots when objects involved in the slot call are destroyed. Let's consider a typical example
    when thing go wrong:

    \code
    struct Hello
    { ... };

    void main()
    {
        Hello *hello = new Hello();
        sig.Connect(hello, &Hello::foo);

        sig.Emit(); // OK
        delete hello;
        sig.Emit(); // <-- segmentation fault, `sig` don't know that `hello` object was destroyed
    }
    \endcode

    However, there is an easy way to avoid such issues. With Signal one may track any object which is
    inherited from special class - TrackedObject. A slot will automatically disconnect when any of its
    tracked objects expire.

    \code
    struct Hello : public TrackedObject
    { ... };
    \endcode

    \code
    sig.Emit(); // OK, Prints "Hello"
    delete hello;   // automatically disconnect from all signals
    sig.Emit(); // OK, Prints nothing
    \endcode

    Also one may use TrackedObject that isn't part of the slot.

    \code
    void main()
    {
        World world; // isn't derived from TrackedObject

        TrackedObject* to = new TrackedObject();

        sig.Connect(to, [](){ std::cout << "Lambda"; });
        sig.Connect(&world, &World::foo).Track(to);

        sig.Emit(); // Prints "Lambda World"

        delete to; // All connection tracked by `to` disconnect

        sig.Emit(); // OK, Prints nothing
    }
    \endcode
*/
template <typename... Args>
class Signal final : protected SignalBase
{
public:
    Signal();
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;
    ~Signal();

    enum class Group
    {
        High,
        Medium,
        Low

        // Warning!!!
        // If more groups should be added implementation of Signal::AddSlot()
        // have to be reviewed and changed!
    };

    /**
        Connect this signal to the incoming object `obj` and callback function `fn` - the slot.
        If `obj` points to TrackedObject than connection will be managed - it will be automatically
        disconnected if this signal or object `obj` is destroyed. Optional parameter `group` can be
        used to associate slot with the given group (see Emit() for more info about groups).

        Incoming `obj` is not a part of the callback. It is only used to link connection
        with specified object to be able to track connection lifetime or disconnect from this
        signal with Signal::Disconnect(void* obj) method.

        Return a Token that identify the newly-created connection to the specified slot `fn`.
    */
    template <typename Obj, typename Fn>
    Token Connect(Obj* obj, const Fn& fn, Group group = Group::Medium);

    /**
        Connect this signal to the incoming object `obj` and callback function `fn` - the slot.
        If `obj` points to TrackedObject than connection will be managed - it will be automatically
        disconnected if this signal or object `obj` is destroyed. Optional parameter `group` can be
        used to associate slot with the given group (see Emit() for more info about groups).

        Return a Token that identify the newly-created connection to the specified slot `fn`.
    */
    template <typename Obj, typename Cls>
    Token Connect(Obj* obj, void (Cls::*const& fn)(Args...), Group group = Group::Medium);

    /**
        Connect this signal to the incoming object `obj` and callback function `fn` - the slot.
        If `obj` points to TrackedObject than connection will be managed - it will be automatically
        disconnected if this signal or object `obj` is destroyed. Optional parameter `group` can be
        used to associate slot with the given group (see Emit() for more info about groups).

        Return a Token that identify the newly-created connection to the specified slot `fn`.
    */
    template <typename Obj, typename Cls>
    Token Connect(Obj* obj, void (Cls::*const& fn)(Args...) const, Group group = Group::Medium);

    /**
        Connects this signal to the incoming callback function `fn` - the slot. Optional parameter `group`
        can be used to associate slot with the given group (see Emit() for more info about groups).

        The connection isn't linked to any object (it is detached) so the only way to break this
        connection is to call Signal::Disconnect(Token) method, passing returned token into it.

        Return a Token that identify the newly-created connection to the specified slot `fn`.
    */
    template <typename Fn>
    Token Connect(const Fn& fn, Group group = Group::Medium);

    /** Disconnects any slot that is linked to the specified object `obj`. */
    void Disconnect(void* obj);

    /** Disconnects slot with specified connection token `token`. */
    void Disconnect(Token token) override;

    /** Disconnects all slots connected to the signal. */
    void DisconnectAll();

    /**
        Make connection with specified token `token` managed by specified TrackedObject `trackedObject`.
        Such connection will be automatically disconnected if this signal or specified `trackedObject` is destroyed.
    */
    void Track(Token token, TrackedObject* trackedObject) override;

    /**
        Sets connection slot with specified token `token` to be temporarily "blocked".
        Blocked slot means that it will be ignored when the signal is emitted but has not been permanently
        disconnected. This is typically used to prevent infinite recursion in cases where otherwise running
        a slot would cause the signal it is connected to be invoked again.

        By default every newly created connection is unblocked.
    */
    void Block(Token token, bool block);

    /**
        Sets every connection that is linked with specified object `obj` to be temporarily "blocked".
        Blocked slot means that it will be ignored when the signal is emitted but has not been permanently
        disconnected. This is typically used to prevent infinite recursion in cases where otherwise running
        a slot would cause the signal it is connected to be invoked again.

        By default every newly created connection is unblocked.
    */
    void Block(void* obj, bool block);

    /* Checks if connection with specified token `token` in temporarily "blocked". */
    bool IsBlocked(Token token) const;

    /**
        Invokes the sequence of calls to the slots connected to signal *this. Every slot will be invoked
        with the given set of parameters `args...`.

        The order of invocation depends on slot connection group:
        1. Signal::Group::High, while order within the group is not defined
        2. Signal::Group::Medium, while order within the group corresponds to the connection order
        3. Signal::Group::Low, while order within the group corresponds to the connection order

        This method will skip slots, that are blocked with Signal::Block() method.
    */
    void Emit(Args... args);

private:
    using ConnectionFn = Function<void(Args...)>;

    /** Internal structure that contains info about connected slot */
    struct Connection
    {
        void* object; //< object that is used with Connect(...) call
        Token token; //< connection unique token
        TrackedObject* tracked; //< TrackedObject, that is try-casted from `object`
        ConnectionFn fn; //< slot function

        std::bitset<3> flags;

        enum Flags
        {
            Emiting,
            Blocked,
            Deleted
        };
    };

    using ConnectionIt = typename List<Connection>::iterator;

    List<Connection> connections;
    ConnectionIt connectionsMediumPos;

    void AddSlot(Connection&& slot, Group group);
    ConnectionIt RemoveSlot(ConnectionIt& it);

    void OnTrackedObjectDestroyed(TrackedObject* object) override;
};

} // namespace DAVA

#define __DAVA_Signal__
#include "Functional/Private/Signal_impl.h"
