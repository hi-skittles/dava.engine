#pragma once

#include "Base/BaseTypes.h"
#include "Base/Singleton.h"
#include "Functional/Signal.h"
#include "Concurrency/Mutex.h"

#include "DownloaderCommon.h"

namespace DAVA
{
class Thread;
class Downloader;
class Engine;

class DownloadManager final : public Singleton<DownloadManager>
{
    friend class Downloader;

public:
    DownloadManager(Engine* e);
    virtual ~DownloadManager();

    // Downloader for further operations
    void SetDownloader(Downloader* _downloader);
    Downloader* GetDownloader();

    // Checks tasks status and determine current task and handles tasks queues
    void Update(float32 frameDelta = 0.0f);

    // Schedule download content or get content size (indicated by downloadMode)
    uint32 Download(const String& srcUrl,
                    const FilePath& storeToFilePath,
                    DownloadType downloadMode = RESUMED,
                    int16 partsCount = -1,
                    int32 timeout = 30,
                    int32 retriesCount = 3,
                    uint64 downloadOffset = 0,
                    uint64 downloadSize = 0);
    // Handy method to download file part
    uint32 DownloadRange(const String& srcUrl,
                         const FilePath& storeToFilePath,
                         uint64 downloadOffset,
                         uint64 downloadSize,
                         DownloadType downloadMode = RESUMED,
                         int16 partsCount = -1,
                         int32 timeout = 30,
                         int32 retriesCount = 3);
    // Schedule download content into memory buffer
    uint32 DownloadIntoBuffer(const String& url,
                              void* buffer,
                              uint32 bufSize,
                              uint64 downloadOffset = 0,
                              uint64 downloadSize = 0,
                              int16 partsCount = -1,
                              int32 timeout = 30,
                              int32 retriesCount = 3);

    // Retry finished download
    void Retry(const uint32& taskId);

    // Cancel active download
    void CancelCurrent();
    // Cancel download by ID (works for scheduled and current)
    void Cancel(const uint32& taskId);
    // Cancell all scheduled and current downloads
    void CancelAll();

    // wait for task statis = finished
    void Wait(const uint32& taskId);
    // wait to end of all tasks
    void WaitAll();

    bool GetCurrentId(uint32& id);
    bool GetUrl(const uint32& taskId, String& url);
    bool GetStorePath(const uint32& taskId, FilePath& path);
    bool GetType(const uint32& taskId, DownloadType& type);
    bool GetStatus(const uint32& taskId, DownloadStatus& status);
    /**
		return full size of requested file
	*/
    bool GetTotal(const uint32& taskId, uint64& total);
    /**
		return current progress of downloading in bytes
	*/
    bool GetProgress(const uint32& taskId, uint64& progress);
    bool GetError(const uint32& taskId, DownloadError& error);
    bool GetImplError(const uint32& taskId, int32& implError);
    bool GetFileErrno(const uint32& taskId, int32& fileErrno);
    bool GetBuffer(uint32 taskId, void*& buffer, uint32& nread);
    DownloadStatistics GetStatistics();
    void SetDownloadSpeedLimit(uint64 limit);
    void SetPreferredDownloadThreadsCount(uint8 count);
    void ResetPreferredDownloadThreadsCount();

    // Signal about download task state changing
    Signal<uint32, DownloadStatus> downloadTaskStateChanged;

private:
    struct CallbackData
    {
        CallbackData(uint32 _id, DownloadStatus _status);

        uint32 id;
        DownloadStatus status;
    };

    void SetTaskStatus(DownloadTaskDescription* task, const DownloadStatus& status);

    void StartProcessingThread();
    void StopProcessingThread();
    void ThreadFunction();

    void ClearQueue(Deque<DownloadTaskDescription*>& queue);
    DownloadTaskDescription* ExtractFromQueue(Deque<DownloadTaskDescription*>& queue, const uint32& taskId);
    void PlaceToQueue(Deque<DownloadTaskDescription*>& queue, DownloadTaskDescription* task);

    DownloadTaskDescription* GetTaskForId(const uint32& taskId);

    void Clear(const uint32& taskId);
    void ClearAll();
    void ClearPending();
    void ClearDone();

    DownloadError Download();
    DownloadError TryDownload();
    DownloadError TryDownloadIntoBuffer();
    void Interrupt();
    void MakeFullDownload();
    void MakeResumedDownload();
    void ResetRetriesCount();
    void OnCurrentTaskProgressChanged(uint64 progressDelta);

    Engine* engine = nullptr;
    Thread* thisThread = nullptr;
    bool isThreadStarted = false;

    Deque<DownloadTaskDescription*> pendingTaskQueue;
    Deque<DownloadTaskDescription*> doneTaskQueue;

    Deque<CallbackData> callbackMessagesQueue;
    Mutex callbackMutex;

    DownloadTaskDescription* currentTask = nullptr;
    static Mutex currentTaskMutex;

    Downloader* downloader = nullptr;
    const uint8 defaultDownloadThreadsCount = 4;
    uint8 preferredDownloadThreadsCount = defaultDownloadThreadsCount;

    uint64 downloadedTotal = 0;
};
}
