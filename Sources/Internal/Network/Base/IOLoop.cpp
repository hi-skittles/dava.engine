#include "Functional/Function.h"
#include "Debug/DVAssert.h"
#include "Concurrency/LockGuard.h"

#include "Network/Base/IOLoop.h"

namespace DAVA
{
namespace Net
{
IOLoop::IOLoop(bool useDefaultIOLoop)
{
#if !defined(DAVA_NETWORK_DISABLE)
    if (useDefaultIOLoop)
    {
        actualLoop = uv_default_loop();
    }
    else
    {
        const int loopInitResult = uv_loop_init(&uvloop);
        DVASSERT(loopInitResult == 0);
        actualLoop = &uvloop;
    }
    actualLoop->data = this;

    const int initResult = uv_async_init(actualLoop, &uvasync, &HandleAsyncThunk);
    DVASSERT(initResult == 0);
    uvasync.data = this;
#endif
}

IOLoop::~IOLoop()
{
#if !defined(DAVA_NETWORK_DISABLE)
    // We can close default loop too
    const int closeResult = uv_loop_close(actualLoop);
    DVASSERT(closeResult == 0);
#endif
}

int32 IOLoop::Run(eRunMode runMode)
{
#if !defined(DAVA_NETWORK_DISABLE)
    static const uv_run_mode modes[] = {
        UV_RUN_DEFAULT,
        UV_RUN_ONCE,
        UV_RUN_NOWAIT
    };
    DVASSERT(RUN_DEFAULT == runMode || RUN_ONCE == runMode || RUN_NOWAIT == runMode);
    return uv_run(actualLoop, modes[runMode]);
#else
    return 0;
#endif
}

void IOLoop::Post(UserHandlerType handler)
{
#if !defined(DAVA_NETWORK_DISABLE)
    {
        LockGuard<Mutex> lock(mutex);
        // TODO: maybe do not insert duplicates
        queuedHandlers.push_back(handler);
    }
    uv_async_send(&uvasync);
#endif
}

void IOLoop::PostQuit()
{
#if !defined(DAVA_NETWORK_DISABLE)
    if (false == quitFlag)
    {
        quitFlag = true;
        uv_async_send(&uvasync);
    }
#endif
}

void IOLoop::HandleAsync()
{
#if !defined(DAVA_NETWORK_DISABLE)
    {
        // Steal queued handlers for execution and release mutex
        // Main reason to do so is to avoid deadlocks if executed
        // handler will want to post another handler
        LockGuard<Mutex> lock(mutex);
        execHandlers.swap(queuedHandlers);
    }

    for (Vector<UserHandlerType>::const_iterator i = execHandlers.begin(), e = execHandlers.end(); i != e; ++i)
    {
        (*i)();
    }
    execHandlers.clear();

    if (true == quitFlag)
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&uvasync), NULL);
    }
#endif
}

void IOLoop::HandleAsyncThunk(uv_async_t* handle)
{
    IOLoop* self = static_cast<IOLoop*>(handle->data);
    self->HandleAsync();
}

} // namespace Net
} // namespace DAVA
