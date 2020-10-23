#ifndef __DAVAENGINE_IOLOOP_H__
#define __DAVAENGINE_IOLOOP_H__

#include "Base/BaseTypes.h"
#include "Base/Platform.h"
#include <libuv/uv.h>

#include "Functional/Function.h"
#include "Base/Noncopyable.h"
#include "Concurrency/Mutex.h"

namespace DAVA
{
namespace Net
{
/*
 Class IOLoop provides event loop which polls for events and schedules handlers (callback) to be run.
 There should be at least one IOLoop instance in network application.
 Run is a core method of IOLoop, and must be called from at most one thread.
 Post is a special method to shedule user specified handler to be run in context of thread where Run
 method is running.
 To finish running IOLoop you should finish all network operations and call PostQuit method
*/
class IOLoop : private Noncopyable
{
public:
    // Behaviours of Run method
    enum eRunMode
    {
        RUN_DEFAULT = UV_RUN_DEFAULT, // Wait for events and execute handlers so long as there are active objects
        RUN_ONCE = UV_RUN_ONCE, // Wait for events, execute handlers and exit
        RUN_NOWAIT = UV_RUN_NOWAIT // Execute handlers if they are ready and immediatly exit
    };

    using UserHandlerType = Function<void()>;

public:
    IOLoop(bool useDefaultIOLoop = true);
    ~IOLoop();

    uv_loop_t* Handle() const
    {
        return actualLoop;
    }

    int32 Run(eRunMode runMode = RUN_DEFAULT);

    void Post(UserHandlerType handler);
    void PostQuit();

private:
    void HandleAsync();

    static void HandleAsyncThunk(uv_async_t* handle);

private:
    uv_loop_t* actualLoop = nullptr;
#if !defined(DAVA_NETWORK_DISABLE)
    uv_loop_t uvloop; // libuv loop handle itself

    bool quitFlag = false;
    uv_async_t uvasync; // libuv handle for calling callback from different threads
#endif
    Vector<UserHandlerType> queuedHandlers; // List of queued user handlers
    Vector<UserHandlerType> execHandlers; // List of executing user handlers
    Mutex mutex;
};

} // namespace Net
} // namespace DAVA

#endif // __DAVAENGINE_IOLOOP_H__
