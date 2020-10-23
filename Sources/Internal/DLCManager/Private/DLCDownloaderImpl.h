#pragma once

#include "DLCManager/DLCDownloader.h"
#include "Concurrency/Thread.h"
#include "Concurrency/Semaphore.h"
#include "Debug/ProfilerCPU.h"

#define CURL_STATICLIB
#include <curl/curl.h>

namespace DAVA
{
struct Buffer
{
    void* ptr = nullptr;
    size_t size = 0;
};

struct IDownloaderSubTask;

struct ICurlEasyStorage
{
    virtual ~ICurlEasyStorage();
    virtual CURLM* GetMultiHandle() = 0;
    virtual CURL* CurlCreateHandle() = 0;
    virtual void CurlDeleteHandle(CURL* easy) = 0;
    virtual int GetFreeHandleCount() = 0;
    virtual void Map(CURL* easy, IDownloaderSubTask& subTask) = 0;
    virtual IDownloaderSubTask& FindInMap(CURL* easy) = 0;
    virtual void UnMap(CURL* easy) = 0;
    virtual int GetChunkSize() = 0;
};

class DLCDownloaderImpl : public DLCDownloader, public ICurlEasyStorage
{
public:
    explicit DLCDownloaderImpl(const Hints& hints_);
    ~DLCDownloaderImpl();

    DLCDownloaderImpl(const DLCDownloaderImpl&) = delete;
    DLCDownloaderImpl(DLCDownloaderImpl&&) = delete;
    DLCDownloaderImpl& operator=(const DLCDownloader&) = delete;

    ITask* StartGetContentSize(const String& srcUrl) override;

    ITask* StartTask(const String& srcUrl, const String& dstPath, Range range = EmptyRange) override;

    ITask* StartTask(const String& srcUrl, std::shared_ptr<IWriter> customWriter, Range range = EmptyRange) override;

    ITask* ResumeTask(const String& srcUrl, const String& dstPath, Range range = EmptyRange) override;

    ITask* ResumeTask(const String& srcUrl, std::shared_ptr<IWriter> customWriter, Range range = EmptyRange) override;

    // Cancel download by ID (works for scheduled and current)
    void RemoveTask(ITask* task) override;

    // wait for task status = finished
    void WaitTask(ITask* task) override;

    const TaskInfo& GetTaskInfo(ITask* task) override;
    const TaskStatus& GetTaskStatus(ITask* task) override;

    void SetHints(const Hints& h) override;

    struct Task;

private:
    void Initialize();
    void Deinitialize();
    bool TakeNewTaskFromInputList();
    void SignalOnFinishedWaitingTasks();
    void AddNewTasks();
    void ConsumeSubTask(CURLMsg* curlMsg, CURL* easyHandle);
    void ProcessMessagesFromMulti();
    void BalancingHandles();

    ITask* StartAnyTask(const String& srcUrl,
                        const String& dsrPath,
                        TaskType taskType,
                        std::shared_ptr<IWriter> dstWriter,
                        Range range = EmptyRange);

    // [start] implement ICurlEasyStorage interface
    CURL* CurlCreateHandle() override;
    void CurlDeleteHandle(CURL* easy) override;
    CURLM* GetMultiHandle() override;
    int GetFreeHandleCount() override;
    void Map(CURL* easy, IDownloaderSubTask& subTask) override;
    IDownloaderSubTask& FindInMap(CURL* easy) override;
    void UnMap(CURL* easy) override;
    int GetChunkSize() override;
    // [end] implement ICurlEasyStorage interface

    void DownloadThreadFunc();
    void DeleteTask(ITask* task);
    void RemoveDeletedTasks();
    Task* AddOneMoreTask();
    int CurlPerform();

    struct WaitingDescTask
    {
        Task* task = nullptr;
        Semaphore* semaphore = nullptr;
    };

    List<Task*> inputList;
    Mutex mutexInputList; // to protect access to taskQueue
    List<WaitingDescTask> waitingTaskList;
    Mutex mutexWaitingList;
    List<ITask*> removedList;
    Mutex mutexRemovedList;

    Thread::Id downloadThreadId = 0;

    // [start] next variables used only from Download thread
    List<Task*> tasks;
    UnorderedMap<CURL*, IDownloaderSubTask*> subtaskMap;
    List<CURL*> reusableHandles;
    CURLM* multiHandle = nullptr;
    Thread* downloadThread = nullptr;
    int numOfRunningSubTasks = 0;
    int multiWaitRepeats = 0;
    // [end] variables

    Semaphore downloadSem; // to resume download thread

    Hints hints; // read only params
    ProfilerCPU unusedProfiler;
};

} // end namespace DAVA
