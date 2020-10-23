#include "FileSystem/Private/FileWatcherBackendWin32.h"

#if defined(__DAVAENGINE_WIN32__)
#include "Concurrency/Thread.h"
#include "Concurrency/Atomic.h"
#include "Utils/UTF8Utils.h"
#include "Time/SystemTimer.h"

namespace DAVA
{
struct FileWatcherBackend::WatchNode
{
    HANDLE directoryHandle;
    uint8 buffer[4096];
    String directory;
    bool isRecursive = false;
    bool pendingRemove = false;
    OVERLAPPED overlapped;
};

struct FileWatcherBackend::WatchNodeDeleter
{
    void operator()(FileWatcherBackend::WatchNode* node)
    {
        CloseHandle(node->directoryHandle);
        node->directoryHandle = INVALID_HANDLE_VALUE;
        delete node;
    }
};

FileWatcherBackend::FileWatcherBackend()
{
    ioCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
    ::ZeroMemory(&overlapped, sizeof(overlapped));
}

void FileWatcherBackend::QueueDirectoryReading(WatchNode* node)
{
    DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE;
    ReadDirectoryChangesW(node->directoryHandle, node->buffer, 4096, node->isRecursive, filter, NULL, &node->overlapped, NULL);
}

FileWatcherBackend::~FileWatcherBackend()
{
    DVASSERT(nodes.empty() == true);
    CloseHandle(ioCompletionPort);
}

void FileWatcherBackend::Add(const String& directory, bool recursive)
{
    WideString wideDir = UTF8Utils::EncodeToWideString(directory);

    std::shared_ptr<WatchNode> newNode = std::shared_ptr<WatchNode>(new WatchNode(), WatchNodeDeleter());
    newNode->directory = directory;
    newNode->isRecursive = recursive;
    newNode->directoryHandle = CreateFileW(wideDir.c_str(), GENERIC_READ | FILE_LIST_DIRECTORY,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                                           NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                           NULL);

    if (newNode->directoryHandle == INVALID_HANDLE_VALUE)
    {
        String msg;
        DWORD errorMessageID = ::GetLastError();
        if (errorMessageID != 0)
        {
            LPWSTR messageBuffer = nullptr;
            size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                         NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

            WideString wideMsg(messageBuffer, size);
            LocalFree(messageBuffer);

            msg = UTF8Utils::EncodeToUTF8(wideMsg);
        }
        else
        {
            msg = Format("Directory %s can't be opened for watch, but there were no errors recorded", directory.c_str());
        }
        DAVA_THROW(Exception, msg);
    }

#if defined(__DAVAENGINE_DEBUG__)
    for (const std::shared_ptr<WatchNode>& n : nodes)
    {
        DVASSERT(n->directory != directory);
    }
#endif
    nodes.push_back(newNode);
    ioCompletionPort = CreateIoCompletionPort(newNode->directoryHandle, ioCompletionPort, reinterpret_cast<ULONG_PTR>(newNode.get()), 0);
    QueueDirectoryReading(newNode.get());
}

void FileWatcherBackend::Remove(const String& directory)
{
    for (auto iter = nodes.begin(); iter != nodes.end(); ++iter)
    {
        std::shared_ptr<WatchNode> node = *iter;
        if (node->directory == directory)
        {
            UniqueLock<Mutex> lock(mutex);
            node->pendingRemove = true;
            PostQueuedCompletionStatus(ioCompletionPort, sizeof(WatchNode), reinterpret_cast<ULONG_PTR>(node.get()), &overlapped);
            condition.Wait(lock, [node]() {
                return node->pendingRemove == false;
            });
            nodes.erase(iter);
            break;
        }
    }
}

void FileWatcherBackend::RemoveAll()
{
    for (auto iter = nodes.begin(); iter != nodes.end(); ++iter)
    {
        std::shared_ptr<WatchNode> node = *iter;
        UniqueLock<Mutex> lock(mutex);
        node->pendingRemove = true;
        PostQueuedCompletionStatus(ioCompletionPort, sizeof(WatchNode), reinterpret_cast<ULONG_PTR>(node.get()), &overlapped);
        condition.Wait(lock, [node]() {
            return node->pendingRemove == false;
        });
    }

    nodes.clear();
}

void FileWatcherBackend::Run()
{
    workThread = Thread::Current();

    WideString prevFileName;
    DWORD prevAction = 0;
    int64 lastActionTime = SystemTimer::GetMs();
    while (true)
    {
        DWORD numBytes = 0;
        LPOVERLAPPED overlapped = nullptr;
        WatchNode* node = nullptr;
        GetQueuedCompletionStatus(ioCompletionPort, &numBytes, reinterpret_cast<PULONG_PTR>(&node), &overlapped, INFINITE);
        {
            UniqueLock<Mutex> lock(mutex);
            // Stop was called
            if (node == nullptr)
            {
                watchThreadFinished = true;
                condition.NotifyOne();
                break;
            }

            if (node->pendingRemove == true)
            {
                node->pendingRemove = false;
                condition.NotifyOne();
                continue;
            }
        }

        if (numBytes == 0)
        {
            continue;
        }

        FILE_NOTIFY_INFORMATION* pNotify = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&node->buffer);
        int32 offset = 0;
        do
        {
            WideString originalFileName(pNotify->FileName, pNotify->FileNameLength / 2);
            int64 currentTime = SystemTimer::GetMs();
            if (originalFileName != prevFileName || prevAction != pNotify->Action || (currentTime - lastActionTime) > 100)
            {
                prevAction = pNotify->Action;
                prevFileName = originalFileName;
                String fileName = node->directory + UTF8Utils::EncodeToUTF8(originalFileName);
                std::replace(fileName.begin(), fileName.end(), '\\', '/');

                switch (pNotify->Action)
                {
                case FILE_ACTION_ADDED:
                case FILE_ACTION_RENAMED_NEW_NAME:
                    onWatchersChanged.Emit(fileName, FileWatcher::FILE_CREATED);
                    break;
                case FILE_ACTION_REMOVED:
                case FILE_ACTION_RENAMED_OLD_NAME:
                    onWatchersChanged.Emit(fileName, FileWatcher::FILE_REMOVED);
                    break;
                case FILE_ACTION_MODIFIED:
                    onWatchersChanged.Emit(fileName, FileWatcher::FILE_MODIFIED);
                    break;
                default:
                    break;
                }
            }

            lastActionTime = currentTime;
            offset = pNotify->NextEntryOffset;
            pNotify = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<uint8*>(pNotify) + offset);
        } while (offset != 0);

        QueueDirectoryReading(node);
    }
}

void FileWatcherBackend::Stop()
{
    UniqueLock<Mutex> lock(mutex);
    PostQueuedCompletionStatus(ioCompletionPort, 0, NULL, NULL);
    condition.Wait(lock, [this]() {
        return watchThreadFinished == true;
    });
}

} // namespace DAVA
#endif // __DAVAENGINE_WINT32__
