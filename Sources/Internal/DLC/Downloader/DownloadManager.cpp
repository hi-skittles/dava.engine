#include "Functional/Function.h"
#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"
#include "Concurrency/Thread.h"
#include "Concurrency/Mutex.h"

#include "DownloadManager.h"
#include "Downloader.h"

#include "Engine/Engine.h"

#include <atomic>

namespace DAVA
{
static std::atomic<uint32> prevId{ 1 };

DownloadManager::CallbackData::CallbackData(uint32 _id, DownloadStatus _status)
    : id(_id)
    , status(_status)
{
}

Mutex DownloadManager::currentTaskMutex;

DownloadManager::DownloadManager(Engine* e)
    : engine(e)
{
    engine->update.Connect(this, &DownloadManager::Update);
    engine->backgroundUpdate.Connect(this, &DownloadManager::Update);
}

DownloadManager::~DownloadManager()
{
    engine->update.Disconnect(this);
    engine->backgroundUpdate.Disconnect(this);

    isThreadStarted = false;

    if (currentTask)
    {
        Interrupt();
        Wait(currentTask->id);
    }

    ClearAll();

    SafeRelease(thisThread);
    SafeDelete(downloader);
}

void DownloadManager::SetDownloader(Downloader* _downloader)
{
    DVASSERT(NULL != _downloader);

    while (NULL != currentTask)
    {
        Thread::Sleep(10);
        Update();
    }

    SafeDelete(downloader);
    downloader = _downloader;
    downloader->SetProgressNotificator(MakeFunction(this, &DownloadManager::OnCurrentTaskProgressChanged));
}

Downloader* DownloadManager::GetDownloader()
{
    return downloader;
}

void DownloadManager::StartProcessingThread()
{
    // thread should be stopped
    DVASSERT(!isThreadStarted);
    DVASSERT(NULL == thisThread);

    thisThread = Thread::Create(MakeFunction(this, &DownloadManager::ThreadFunction));
    isThreadStarted = true;
    thisThread->Start();
}

void DownloadManager::StopProcessingThread()
{
    isThreadStarted = false;
    thisThread->Join();

    SafeRelease(thisThread);
}

void DownloadManager::Update(float32 frameDelta)
{
    if (!currentTask)
    {
        if (!pendingTaskQueue.empty())
        {
            currentTaskMutex.Lock();
            currentTask = pendingTaskQueue.front();
            DVASSERT(0 < currentTask->id);
            pendingTaskQueue.pop_front();
            currentTaskMutex.Unlock();

            if (!isThreadStarted)
                StartProcessingThread();
        }
    }
    else
    {
        // if task is done and we have no pending tasks - stop processing thread.
        if (DL_FINISHED == currentTask->status)
        {
            PlaceToQueue(doneTaskQueue, currentTask);

            if (pendingTaskQueue.empty())
                StopProcessingThread();

            currentTaskMutex.Lock();
            currentTask = nullptr;
            currentTaskMutex.Unlock();
        }
    }

    callbackMutex.Lock();
    if (!callbackMessagesQueue.empty())
    {
        for (Deque<CallbackData>::iterator it = callbackMessagesQueue.begin(); it != callbackMessagesQueue.end();)
        {
            CallbackData cbData = (*it);
            it = callbackMessagesQueue.erase(it);
            downloadTaskStateChanged.Emit(cbData.id, cbData.status);
        }
    }
    callbackMutex.Unlock();
}

uint32 DownloadManager::Download(const String& srcUrl,
                                 const FilePath& storeToFilePath,
                                 const DownloadType downloadMode,
                                 const int16 partsCount,
                                 int32 timeout,
                                 int32 retriesCount,
                                 uint64 downloadOffset,
                                 uint64 downloadSize)
{
    int16 usePartsCount = (-1 == partsCount) ? (preferredDownloadThreadsCount) : partsCount;
    DVASSERT(usePartsCount > 0);
    DownloadTaskDescription* task = new DownloadTaskDescription(srcUrl,
                                                                storeToFilePath,
                                                                downloadMode,
                                                                timeout,
                                                                retriesCount,
                                                                static_cast<uint8>(usePartsCount),
                                                                downloadOffset,
                                                                downloadSize);

    task->id = prevId.fetch_add(1);

    PlaceToQueue(pendingTaskQueue, task);

    task->status = DL_PENDING;

    return task->id;
}

uint32 DownloadManager::DownloadRange(const String& srcUrl,
                                      const FilePath& storeToFilePath,
                                      uint64 downloadOffset,
                                      uint64 downloadSize,
                                      DownloadType downloadMode,
                                      int16 partsCount,
                                      int32 timeout,
                                      int32 retriesCount)
{
    return Download(srcUrl, storeToFilePath, downloadMode, -1, 30, 3, downloadOffset, downloadSize);
}

uint32 DownloadManager::DownloadIntoBuffer(const String& srcUrl,
                                           void* buffer,
                                           uint32 bufSize,
                                           uint64 downloadOffset,
                                           uint64 downloadSize,
                                           int16 partsCount,
                                           int32 timeout,
                                           int32 retriesCount)
{
    DVASSERT(bufSize > 0 && buffer != nullptr);
    DVASSERT(downloadSize <= bufSize);

    int16 usePartsCount = (-1 == partsCount) ? (preferredDownloadThreadsCount) : partsCount;
    DVASSERT(usePartsCount > 0);
    DownloadTaskDescription* task = new DownloadTaskDescription(srcUrl,
                                                                buffer,
                                                                bufSize,
                                                                FULL,
                                                                timeout,
                                                                retriesCount,
                                                                static_cast<uint8>(usePartsCount),
                                                                downloadOffset,
                                                                downloadSize);

    task->id = prevId.fetch_add(1);

    PlaceToQueue(pendingTaskQueue, task);

    task->status = DL_PENDING;

    return task->id;
}

void DownloadManager::Retry(const uint32& taskId)
{
    DownloadTaskDescription* taskToRetry = ExtractFromQueue(doneTaskQueue, taskId);
    if (taskToRetry)
    {
        taskToRetry->error = DLE_NO_ERROR;
        taskToRetry->type = RESUMED;
        SetTaskStatus(taskToRetry, DL_PENDING);
        PlaceToQueue(pendingTaskQueue, taskToRetry);
    }
}

void DownloadManager::Cancel(const uint32& taskId)
{
    DownloadTaskDescription* curTaskToDelete = currentTask;

    if (curTaskToDelete && taskId == curTaskToDelete->id)
    {
        Interrupt();
        Wait(taskId);
    }
    else
    {
        DownloadTaskDescription* pendingTask = nullptr;
        pendingTask = ExtractFromQueue(pendingTaskQueue, taskId);
        if (pendingTask)
        {
            pendingTask->error = DLE_CANCELLED;
            SetTaskStatus(pendingTask, DL_FINISHED);
            PlaceToQueue(doneTaskQueue, pendingTask);
        }
    }
}

void DownloadManager::CancelCurrent()
{
    DownloadTaskDescription* curTaskToCancel = currentTask;
    if (!curTaskToCancel)
        return;

    Interrupt();
    Wait(curTaskToCancel->id);
}

void DownloadManager::CancelAll()
{
    if (!pendingTaskQueue.empty())
    {
        Deque<DownloadTaskDescription*>::iterator it;
        for (it = pendingTaskQueue.begin(); it != pendingTaskQueue.end();)
        {
            DownloadTaskDescription* task = (*it);
            task->error = DLE_CANCELLED;
            SetTaskStatus(task, DL_FINISHED);
            doneTaskQueue.push_back(task);
            it = pendingTaskQueue.erase(it);
        }
    }

    if (currentTask)
    {
        Interrupt();
        Wait(currentTask->id);
    }
}

void DownloadManager::Clear(const uint32& taskId)
{
    // cancel task if possible
    Cancel(taskId);

    DownloadTaskDescription* task = NULL;
    task = ExtractFromQueue(pendingTaskQueue, taskId);
    if (task)
    {
        SafeDelete(task);
    }
    else
    {
        task = ExtractFromQueue(doneTaskQueue, taskId);
        if (task)
        {
            SafeDelete(task);
        }
    }
}

void DownloadManager::ThreadFunction()
{
    while (isThreadStarted)
    {
        Thread::Sleep(20);

        currentTaskMutex.Lock();
        if (!currentTask || DL_FINISHED == currentTask->status)
        {
            currentTaskMutex.Unlock();
            continue;
        }

        currentTask->error = Download();

        currentTaskMutex.Unlock();

        // if we need to stop thread (finish current task end exit)
        if (!isThreadStarted)
            break;
    }
}

void DownloadManager::ClearAll()
{
    ClearPending();
    ClearDone();

    DownloadTaskDescription* currentTaskToClear = nullptr;

    currentTaskToClear = currentTask;

    Clear(currentTaskToClear->id);
}

void DownloadManager::ClearPending()
{
    ClearQueue(pendingTaskQueue);
}

void DownloadManager::ClearDone()
{
    ClearQueue(doneTaskQueue);
}

void DownloadManager::Wait(const uint32& taskId)
{
    // if you called it from other thread than main - you should be sured that Update() method calls periodically from Main Thread.

    DownloadTaskDescription* waitTask = NULL;
    DownloadTaskDescription* currentTaskToWait = NULL;

    currentTaskToWait = currentTask;

    if (currentTaskToWait && currentTaskToWait->id == taskId)
        waitTask = currentTaskToWait;

    if (!waitTask)
    {
        Deque<DownloadTaskDescription*>::iterator it;
        for (it = pendingTaskQueue.begin(); it != pendingTaskQueue.end(); ++it)
        {
            DownloadTaskDescription* task = (*it);
            if (taskId == task->id)
                waitTask = task;
        }
    }

    // if there is no such task or task is completed - we don;t need to wait
    if (NULL == waitTask || (DL_PENDING != waitTask->status && DL_IN_PROGRESS != waitTask->status))
        return;

    // wait until task is finished
    while (waitTask
           && (waitTask->status == DL_IN_PROGRESS || waitTask->status == DL_PENDING))
    {
        Thread::Sleep(20);
        Update();
    }
}

void DownloadManager::WaitAll()
{
    while (true)
    {
        bool needToWait = currentTask || !pendingTaskQueue.empty();

        if (needToWait)
        {
            Thread::Sleep(20);
            Update();
        }
        else
            break;
    }
}

bool DownloadManager::GetCurrentId(uint32& id)
{
    DownloadTaskDescription* curTaskToGet = currentTask;

    if (curTaskToGet)
    {
        id = curTaskToGet->id;
        return true;
    }

    return false;
}

bool DownloadManager::GetUrl(const uint32& taskId, String& url)
{
    DownloadTaskDescription* task = GetTaskForId(taskId);
    if (!task)
        return false;

    url = task->url;

    return true;
}

bool DownloadManager::GetStorePath(const uint32& taskId, FilePath& path)
{
    DownloadTaskDescription* task = GetTaskForId(taskId);
    if (!task)
        return false;

    path = task->storePath;

    return true;
}

bool DownloadManager::GetType(const uint32& taskId, DownloadType& type)
{
    DownloadTaskDescription* task = GetTaskForId(taskId);
    if (!task)
        return false;

    type = task->type;

    return true;
}

bool DownloadManager::GetStatus(const uint32& taskId, DownloadStatus& status)
{
    DownloadTaskDescription* task = GetTaskForId(taskId);
    if (!task)
        return false;

    status = task->status;

    return true;
}

bool DownloadManager::GetTotal(const uint32& taskId, uint64& total)
{
    DownloadTaskDescription* task = GetTaskForId(taskId);
    if (!task)
        return false;

    total = task->downloadTotal;

    return true;
}

bool DownloadManager::GetProgress(const uint32& taskId, uint64& progress)
{
    DownloadTaskDescription* task = GetTaskForId(taskId);
    if (!task)
        return false;

    progress = task->downloadProgress;

    return true;
}

bool DownloadManager::GetError(const uint32& taskId, DownloadError& error)
{
    DownloadTaskDescription* task = GetTaskForId(taskId);
    if (!task)
        return false;

    error = task->error;

    return true;
}

bool DownloadManager::GetImplError(const uint32& taskId, int32& implError)
{
    DownloadTaskDescription* task = GetTaskForId(taskId);
    if (!task)
        return false;

    implError = task->implError;

    return true;
}

bool DownloadManager::GetFileErrno(const uint32& taskId, int32& fileErrno)
{
    DownloadTaskDescription* task = GetTaskForId(taskId);
    if (!task)
        return false;

    fileErrno = task->fileErrno;

    return true;
}

bool DownloadManager::GetBuffer(uint32 taskId, void*& buffer, uint32& nread)
{
    DownloadTaskDescription* task = GetTaskForId(taskId);
    if (task != nullptr)
    {
        buffer = task->memoryBuffer;
        nread = task->memoryBufferContentSize;
        return true;
    }
    return false;
}

DownloadStatistics DownloadManager::GetStatistics()
{
    return downloader->GetStatistics();
}

void DownloadManager::SetDownloadSpeedLimit(uint64 limit)
{
    downloader->SetDownloadSpeedLimit(limit);
}

void DownloadManager::SetPreferredDownloadThreadsCount(uint8 count)
{
    preferredDownloadThreadsCount = count;
}

void DownloadManager::ResetPreferredDownloadThreadsCount()
{
    preferredDownloadThreadsCount = defaultDownloadThreadsCount;
}

void DownloadManager::ClearQueue(Deque<DownloadTaskDescription*>& queue)
{
    if (!queue.empty())
    {
        for (Deque<DownloadTaskDescription*>::iterator it = queue.begin(); it != queue.end();)
        {
            DownloadTaskDescription* task = (*it);
            delete task;
            it = queue.erase(it);
        }
    }
}

DownloadTaskDescription* DownloadManager::ExtractFromQueue(Deque<DownloadTaskDescription*>& queue, const uint32& taskId)
{
    DownloadTaskDescription* extractedTask = nullptr;

    if (!queue.empty())
    {
        for (Deque<DownloadTaskDescription*>::iterator it = queue.begin(); it != queue.end();)
        {
            DownloadTaskDescription* task = (*it);
            if (task->id == taskId)
            {
                extractedTask = task;

                it = queue.erase(it);
            }
            else
                ++it;
        }
    }
    return extractedTask;
}

void DownloadManager::PlaceToQueue(Deque<DownloadTaskDescription*>& queue, DownloadTaskDescription* task)
{
    queue.push_back(task);
}

DownloadTaskDescription* DownloadManager::GetTaskForId(const uint32& taskId)
{
    DownloadTaskDescription* retPointer = nullptr;

    if (currentTask && taskId == currentTask->id)
    {
        retPointer = currentTask;
        return retPointer;
    }

    Deque<DownloadTaskDescription*>::iterator it;
    for (it = pendingTaskQueue.begin(); it != pendingTaskQueue.end(); ++it)
    {
        DownloadTaskDescription* task = (*it);
        if (task->id == taskId)
            return task;
    }

    for (it = doneTaskQueue.begin(); it != doneTaskQueue.end(); ++it)
    {
        DownloadTaskDescription* task = (*it);
        if (task->id == taskId)
            return task;
    }

    return retPointer;
}

void DownloadManager::SetTaskStatus(DownloadTaskDescription* task, const DownloadStatus& status)
{
    DVASSERT(task);
    DVASSERT(status != task->status);

    task->status = status;

    callbackMutex.Lock();
    callbackMessagesQueue.push_back(CallbackData(task->id, task->status));
    callbackMutex.Unlock();
}

void DownloadManager::Interrupt()
{
    DVASSERT(currentTask);
    DVASSERT(downloader);

    downloader->Interrupt();
}

DownloadError DownloadManager::Download()
{
    DownloadType typeForRetry = FULL;
    DownloadError (DownloadManager::*downloadFunc)() = &DownloadManager::TryDownloadIntoBuffer;
    if (currentTask->memoryBuffer == nullptr)
    {
        FilePath path(currentTask->storePath);
        FileSystem::Instance()->CreateDirectory(path.GetDirectory(), true);

        typeForRetry = RESUMED;
        downloadFunc = &DownloadManager::TryDownload;
    }

    ResetRetriesCount();
    DownloadError error = DLE_NO_ERROR;
    SetTaskStatus(currentTask, DL_IN_PROGRESS);
    downloadedTotal = 0;

    do
    {
        error = (this->*downloadFunc)();

        if (DLE_CONTENT_NOT_FOUND == error
            || DLE_CANCELLED == error
            || DLE_FILE_ERROR == error
            || DLE_NO_RANGE_REQUEST == error
            || DLE_INVALID_RANGE == error)
            break;

        currentTask->type = typeForRetry;

    } while (0 < currentTask->retriesLeft-- && DLE_NO_ERROR != error);

    SetTaskStatus(currentTask, DL_FINISHED);
    return error;
}

DownloadError DownloadManager::TryDownload()
{
    // retrieve remote file size
    currentTask->error = downloader->GetSize(currentTask->url, currentTask->downloadTotal, currentTask->timeout);
    currentTask->fileErrno = downloader->GetFileErrno();
    currentTask->implError = downloader->GetImplError();
    if (DLE_NO_ERROR != currentTask->error)
    {
        return currentTask->error;
    }

    if (GET_SIZE == currentTask->type)
    {
        return currentTask->error;
    }

    DVASSERT(GET_SIZE != currentTask->type);
    currentTask->downloadProgress = 0;

    if (RESUMED == currentTask->type)
    {
        MakeResumedDownload();

        // if file is downloaded - we don't need to try download it again
        ScopedPtr<File> dstFile(File::Create(currentTask->storePath, File::OPEN | File::READ));
        if (dstFile)
        {
            uint64 currentFileSize = dstFile->GetSize();
            uint64 sizeToDownload = currentTask->downloadTotal - currentFileSize;
            if (0 < currentTask->downloadTotal && 0 == sizeToDownload)
            {
                return DLE_NO_ERROR;
            }
        }
    }
    else
    {
        MakeFullDownload();
    }

    if (DLE_NO_ERROR != currentTask->error)
    {
        return currentTask->error;
    }

    currentTask->error = downloader->Download(currentTask->url,
                                              currentTask->downloadOffset,
                                              currentTask->downloadSize,
                                              currentTask->storePath,
                                              currentTask->partsCount,
                                              currentTask->timeout);
    currentTask->fileErrno = downloader->GetFileErrno();
    currentTask->implError = downloader->GetImplError();

    // seems server doesn't supports download resuming. So we need to download whole file.
    if (DLE_COULDNT_RESUME == currentTask->error)
    {
        MakeFullDownload();
        if (DLE_NO_ERROR == currentTask->error)
        {
            currentTask->error = downloader->Download(currentTask->url,
                                                      currentTask->downloadOffset,
                                                      currentTask->downloadSize,
                                                      currentTask->storePath,
                                                      currentTask->partsCount,
                                                      currentTask->timeout);
            currentTask->fileErrno = downloader->GetFileErrno();
            currentTask->implError = downloader->GetImplError();
        }
    }

    return currentTask->error;
}

DownloadError DownloadManager::TryDownloadIntoBuffer()
{
    currentTask->downloadProgress = 0;
    currentTask->error = downloader->DownloadIntoBuffer(currentTask->url,
                                                        currentTask->downloadOffset,
                                                        currentTask->downloadSize,
                                                        currentTask->memoryBuffer,
                                                        currentTask->memoryBufferSize,
                                                        currentTask->partsCount,
                                                        currentTask->timeout,
                                                        &currentTask->memoryBufferContentSize);
    return currentTask->error;
}

void DownloadManager::MakeFullDownload()
{
    currentTask->type = FULL;

    if (FileSystem::Instance()->Exists(currentTask->storePath))
    {
        if (FileSystem::Instance()->DeleteFile(currentTask->storePath))
        {
            currentTask->error = DLE_NO_ERROR;
        }
        else
        {
            currentTask->error = DLE_FILE_ERROR;
        }
    }
    currentTask->downloadProgress = 0;
}

void DownloadManager::MakeResumedDownload()
{
    currentTask->type = RESUMED;
    // if file is particulary downloaded, we will try to download rest part of it
    File* file = File::Create(currentTask->storePath, File::OPEN | File::READ);
    if (NULL == static_cast<File*>(file))
    {
        // download fully if there is no file.
        MakeFullDownload();
    }
    else
    {
        currentTask->downloadProgress = file->GetSize();
        SafeRelease(file);
        // if exsisted file have not the same size as downloading file
        if (currentTask->downloadProgress > currentTask->downloadTotal)
        {
            MakeFullDownload();
        }
    }
}

void DownloadManager::ResetRetriesCount()
{
    currentTask->retriesLeft = currentTask->retriesCount;
}

void DownloadManager::OnCurrentTaskProgressChanged(uint64 progressDelta)
{
    currentTask->downloadProgress += progressDelta;
}
}
