#include "FileSystem/FileWatcher.h"

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)

#if defined(__DAVAENGINE_WIN32__)
#include "FileSystem/Private/FileWatcherBackendWin32.h"
#elif defined(__DAVAENGINE_MACOS__)
#include "FileSystem/Private/FileWatcherBackendMac.h"
#else
#error "FileWatcher is not implemented"
#endif

#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "Job/JobManager.h"

namespace DAVA
{
FileWatcher::FileWatcher()
{
    backend = new FileWatcherBackend();
    backend->onWatchersChanged.Connect(this, &FileWatcher::Notify);
    updateThread = Thread::Create(MakeFunction(backend, &FileWatcherBackend::Run));
    updateThread->SetName("DAVA.FileWatcher");
    updateThread->Start();
}

FileWatcher::~FileWatcher()
{
    Thread::Id workerThreadId = updateThread->GetId();

    backend->RemoveAll();
    backend->Stop();
    updateThread->Cancel();
    updateThread->Join();
    delete backend;
    updateThread->Release();

    GetEngineContext()->jobManager->WaitMainJobs(workerThreadId);
}

void FileWatcher::Add(const String& directory, bool recursive)
{
    backend->Add(directory, recursive);
}

void FileWatcher::Remove(const String& directory)
{
    backend->Remove(directory);
}

void FileWatcher::Notify(const String& filePath, eWatchEvent event)
{
    GetEngineContext()->jobManager->CreateMainJob([=] {
        onWatchersChanged.Emit(filePath, event);
    });
}

} // namespace DAVA

#endif
