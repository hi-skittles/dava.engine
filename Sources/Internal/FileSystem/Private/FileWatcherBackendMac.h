#pragma once

#include "FileSystem/FileWatcher.h"

#if defined(__DAVAENGINE_MACOS__)
#include "Concurrency/Mutex.h"
#include "Concurrency/ConditionVariable.h"

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

    struct ContextInfo;

private:
    struct WatchNode;

    void CreateFSEvents(WatchNode* node);
    void ReleaseFSEvents(WatchNode* node);

    Vector<std::shared_ptr<WatchNode>> nodes;
    Mutex mutex;
    ConditionVariable condition;
    bool updateWatchNodes;
};
}
#endif // defined(__DAVAENGINE_MACOS__)
