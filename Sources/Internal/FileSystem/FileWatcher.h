#pragma once

#include "Base/String.h"
#include "Base/UnordererMap.h"
#include "Functional/Signal.h"
#include "Concurrency/Thread.h"

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)

namespace DAVA
{
class FileWatcherBackend;
class FileWatcher final
{
public:
    enum eWatchEvent
    {
        FILE_CREATED, // created or renamed
        FILE_REMOVED, // removed or renamed
        FILE_MODIFIED // content changed
    };

    FileWatcher();
    FileWatcher(const FileWatcher& other) = delete;
    FileWatcher(FileWatcher&& other) = delete;
    ~FileWatcher();

    FileWatcher& operator=(const FileWatcher& other) = delete;
    FileWatcher& operator=(FileWatcher&& other) = delete;

    // DAVA::Exception can be throwed
    void Add(const String& directory, bool recursive);
    void Remove(const String& directory);

    Signal<const String&, eWatchEvent> onWatchersChanged;

private:
    void Notify(const String&, eWatchEvent);

    FileWatcherBackend* backend = nullptr;
    Thread* updateThread = nullptr;
};
} // namespace DAVA

#endif
