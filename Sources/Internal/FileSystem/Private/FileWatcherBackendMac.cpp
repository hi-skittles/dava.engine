#include "FileSystem/Private/FileWatcherBackendMac.h"

#if defined(__DAVAENGINE_MACOS__)
#include "Base/Vector.h"
#include "Base/String.h"
#include "Base/Stack.h"
#include "Concurrency/UniqueLock.h"

#include <CoreServices/CoreServices.h>
#include <sys/stat.h>

namespace DAVA
{
struct FileWatcherBackend::WatchNode
{
    String directory;
    FSEventStreamRef stream = nullptr;
    bool pendingRemove = false;
};

struct FileWatcherBackend::ContextInfo
{
    ContextInfo()
    {
        counter = 1;
    }

    WatchNode* node = nullptr;
    FileWatcherBackend* backend = nullptr;

    Atomic<int32> counter;

    static const void* Retain(const void* obj)
    {
        ContextInfo* info = reinterpret_cast<ContextInfo*>(const_cast<void*>(obj));
        info->counter.Increment();
        return info;
    }

    static void Release(const void* obj)
    {
        ContextInfo* info = reinterpret_cast<ContextInfo*>(const_cast<void*>(obj));
        if (info->counter.Decrement() == 0)
        {
            delete info;
        }
    }
};

void FSEventsCallback(ConstFSEventStreamRef streamRef, void* clientCallBackInfo, size_t numEvents, void* eventPaths,
                      const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[])
{
    FileWatcherBackend::ContextInfo* info = reinterpret_cast<FileWatcherBackend::ContextInfo*>(clientCallBackInfo);

    for (size_t i = 0; i < numEvents; ++i)
    {
        const char* rawPath = reinterpret_cast<char**>(eventPaths)[i];
        String path = String(rawPath);

        FSEventStreamEventFlags flags = eventFlags[i];
        bool hasEvent = (flags & (kFSEventStreamEventFlagItemRenamed |
                                  kFSEventStreamEventFlagItemCreated |
                                  kFSEventStreamEventFlagItemRemoved |
                                  kFSEventStreamEventFlagItemModified)) != 0;

        if (hasEvent == false)
        {
            continue;
        }

        struct stat fileStat;
        int result = stat(rawPath, &fileStat);
        if (result != 0)
        {
            info->backend->onWatchersChanged.Emit(path, FileWatcher::FILE_REMOVED);
            continue;
        }

        if (fileStat.st_birthtimespec.tv_sec == fileStat.st_mtimespec.tv_sec)
        {
            info->backend->onWatchersChanged.Emit(path, FileWatcher::FILE_CREATED);
        }
        else
        {
            info->backend->onWatchersChanged.Emit(path, FileWatcher::FILE_MODIFIED);
        }
    }
}

FileWatcherBackend::FileWatcherBackend()
{
    updateWatchNodes = false;
}

FileWatcherBackend::~FileWatcherBackend()
{
    DVASSERT(nodes.empty() == true);
}

void FileWatcherBackend::Add(const String& directory, bool recursive)
{
    std::shared_ptr<WatchNode> node(new WatchNode());
    node->directory = directory;

    UniqueLock<Mutex> guard(mutex);
    nodes.push_back(node);
    updateWatchNodes = true;
}

void FileWatcherBackend::Remove(const String& directory)
{
    auto iter = nodes.begin();
    while (iter != nodes.end())
    {
        std::shared_ptr<WatchNode> node = *iter;
        if (node->directory == directory)
        {
            UniqueLock<Mutex> guard(mutex);
            node->pendingRemove = true;
            updateWatchNodes = true;
            condition.Wait(guard, [node]() {
                return node->pendingRemove == false;
            });
            nodes.erase(iter);
            break;
        }
    }
}

void FileWatcherBackend::RemoveAll()
{
    for (const std::shared_ptr<WatchNode>& node : nodes)
    {
        UniqueLock<Mutex> guard(mutex);
        node->pendingRemove = true;
        updateWatchNodes = true;
        condition.Wait(guard, [node]() {
            return node->pendingRemove == false;
        });
    }

    nodes.clear();
}

void FileWatcherBackend::Run()
{
    Thread* workThread = Thread::Current();

    while (true)
    {
        if (workThread->IsCancelling() == true)
        {
            break;
        }

        {
            UniqueLock<Mutex> guard(mutex);
            if (updateWatchNodes == true)
            {
                bool deletionDetected = false;
                for (const std::shared_ptr<WatchNode>& node : nodes)
                {
                    if (node->stream == nullptr)
                    {
                        CreateFSEvents(node.get());
                    }

                    if (node->pendingRemove == true)
                    {
                        DVASSERT(deletionDetected == false);
                        deletionDetected = true;
                        node->pendingRemove = false;
                        ReleaseFSEvents(node.get());
                        condition.NotifyOne();
                    }
                }
                updateWatchNodes = false;
            }
        }

        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1.0f, 0);
    }
}

void FileWatcherBackend::Stop()
{
}

void FileWatcherBackend::CreateFSEvents(WatchNode* node)
{
    DVASSERT(node->stream == nullptr);
    Vector<CFStringRef> dirs;
    dirs.push_back(CFStringCreateWithCString(nullptr, node->directory.c_str(), kCFStringEncodingUTF8));

    if (dirs.empty())
    {
        return;
    }

    CFArrayRef pathsToWatch = CFArrayCreate(nullptr, reinterpret_cast<const void**>(&dirs[0]),
                                            dirs.size(), &kCFTypeArrayCallBacks);

    FileWatcherBackend::ContextInfo* info = new FileWatcherBackend::ContextInfo();
    info->node = node;
    info->backend = this;

    FSEventStreamContext context;
    context.version = 0;
    context.info = info;
    context.retain = &FileWatcherBackend::ContextInfo::Retain;
    context.release = &FileWatcherBackend::ContextInfo::Release;
    context.copyDescription = nullptr;

    node->stream = FSEventStreamCreate(nullptr, &FSEventsCallback, &context, pathsToWatch, kFSEventStreamEventIdSinceNow,
                                       0.0f, kFSEventStreamCreateFlagFileEvents | kFSEventStreamCreateFlagNoDefer);
    FileWatcherBackend::ContextInfo::Release(info);

    if (!node->stream)
    {
        DAVA_THROW(Exception, "Event stream could not be created.");
    }

    // Fire the event loop
    CFRunLoopRef runLoop = CFRunLoopGetCurrent();
    if (!runLoop)
    {
        DAVA_THROW(Exception, "Run loop could not be retreived");
    }

    FSEventStreamScheduleWithRunLoop(node->stream, runLoop, kCFRunLoopDefaultMode);
    FSEventStreamStart(node->stream);
}

void FileWatcherBackend::ReleaseFSEvents(WatchNode* node)
{
    if (node->stream)
    {
        FSEventStreamStop(node->stream);
        FSEventStreamInvalidate(node->stream);
        FSEventStreamRelease(node->stream);

        node->stream = nullptr;
    }
}

} // namespace DAVA
#endif // __DAVAENGINE_MACOS__
