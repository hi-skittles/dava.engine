#pragma once

#include "FileSystem/FileWatcher.h"

#if defined(__DAVAENGINE_WIN32__)
#include "Concurrency/Mutex.h"
#include "Concurrency/ConditionVariable.h"
#include "Functional/Signal.h"

#include <windows.h>
#include <memory>

namespace DAVA
{
class FileWatcherBackend final
{
public:
    FileWatcherBackend();
    ~FileWatcherBackend();

    void Add(const String& directory, bool recursive);
    void Remove(const String& directory);
    void RemoveAll();

    void Run();
    void Stop();
    Signal<const String&, FileWatcher::eWatchEvent> onWatchersChanged;

private:
    struct WatchNode;
    struct WatchNodeDeleter;
    void QueueDirectoryReading(WatchNode* node);

    Thread* workThread = nullptr;
    Mutex mutex;
    ConditionVariable condition;
    HANDLE ioCompletionPort = INVALID_HANDLE_VALUE;
    OVERLAPPED overlapped;
    Atomic<bool> watchThreadFinished = false;

    Vector<std::shared_ptr<WatchNode>> nodes;
};
} // namespace DAVA
#endif
