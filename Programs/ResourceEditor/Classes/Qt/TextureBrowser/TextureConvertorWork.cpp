#include "Classes/Qt/TextureBrowser/TextureConvertorWork.h"

#include <Concurrency/LockGuard.h>

JobStack::JobStack()
    : head(NULL)
    , itemsCount(0)
{
}

JobStack::~JobStack()
{
    JobItemWrapper* item;

    while (NULL != head)
    {
        item = head;
        head = head->next;

        delete item;
    }
}

bool JobStack::push(const JobItem& item)
{
    bool ret = true;
    JobItemWrapper* i = head;

    // remember force value
    eTextureConvertMode convertMode = item.convertMode;

    // search for the same works in list and remove it
    while (NULL != i)
    {
        if (i->type == item.type && NULL != i->descriptor && i->descriptor == item.descriptor)
        {
            if (NULL != i->prev)
            {
                i->prev->next = i->next;
            }

            if (NULL != i->next)
            {
                i->next->prev = i->prev;
            }

            if (i == head)
            {
                head = i->next;
            }

            // if this job has more strict convert mode, we should move it to the new job
            if (i->convertMode < convertMode)
            {
                convertMode = i->convertMode;
            }

            delete i;
            itemsCount--;

            ret = false;
            break;
        }

        i = i->next;
    }

    // add new work
    i = new JobItemWrapper(item);

    // restore convert mode value
    i->convertMode = convertMode;

    if (NULL != head)
    {
        head->prev = i;
        i->next = head;
    }

    head = i;
    itemsCount++;

    return ret;
}

JobItem* JobStack::pop()
{
    JobItemWrapper* item = head;

    if (NULL != head)
    {
        head = head->next;
        if (NULL != head)
        {
            head->prev = NULL;
        }

        itemsCount--;
    }

    return item;
}

int JobStack::size()
{
    return itemsCount;
}

JobStack::JobItemWrapper::JobItemWrapper(const JobItem& item)
    : JobItem(item)
    , next(NULL)
    , prev(NULL)
{
}

class JobWatcher::Impl
{
public:
    Impl(JobWatcher* watcher_)
        : watcher(watcher_)
    {
    }

    void Deinit()
    {
        DAVA::LockGuard<DAVA::Mutex> guard(mutex);
        watcher = nullptr;
    }

    // will be called on worker thread
    void OnConvert()
    {
        DAVA::LockGuard<DAVA::Mutex> guard(mutex);
        if (watcher != nullptr)
        {
            watcher->OnConvert();
        }
    }

    // will be called on main thread
    void OnReady()
    {
        if (watcher != nullptr)
        {
            watcher->OnReady();
        }
    }

private:
    DAVA::Mutex mutex;
    JobWatcher* watcher = nullptr;
};

JobWatcher::JobWatcher(DAVA::JobManager* manager_)
    : impl(new Impl(this))
    , manager(manager_)
{
    DVASSERT(manager != nullptr);
}

JobWatcher::~JobWatcher()
{
    DVASSERT(impl != nullptr);
    impl->Deinit();
}

void JobWatcher::Init(const TJobFunction& convertFunction_, const TReadyCallback& readyCallback_)
{
    convertFunction = convertFunction_;
    readyCallback = readyCallback_;
}

bool JobWatcher::IsFinished() const
{
    DAVA::LockGuard<DAVA::Mutex> guard(mutex);
    return isFinished;
}

void JobWatcher::RunJob(std::unique_ptr<JobItem>&& item)
{
    DVASSERT(convertFunction != nullptr);
    DVASSERT(readyCallback != nullptr);
    DVASSERT(IsFinished() == true);
    DVASSERT(GetCurrentJobItem() == nullptr);
    {
        DAVA::LockGuard<DAVA::Mutex> guard(mutex);
        isFinished = false;
        jobItem = std::move(item);
    }

    manager->CreateWorkerJob(DAVA::MakeFunction(impl, &JobWatcher::Impl::OnConvert));
}

const JobItem* JobWatcher::GetCurrentJobItem() const
{
    DAVA::LockGuard<DAVA::Mutex> guard(mutex);
    return jobItem.get();
}

JobItem* JobWatcher::GetCurrentJobItemImpl()
{
    DAVA::LockGuard<DAVA::Mutex> guard(mutex);
    return jobItem.get();
}

void JobWatcher::OnConvert()
{
    DVASSERT(IsFinished() == false);
    TextureInfo r = convertFunction(GetCurrentJobItemImpl());
    {
        DAVA::LockGuard<DAVA::Mutex> guard(mutex);
        result = r;
    }

    manager->CreateMainJob(DAVA::MakeFunction(impl, &JobWatcher::Impl::OnReady), DAVA::JobManager::JOB_MAINLAZY);
}

void JobWatcher::OnReady()
{
    TextureInfo r;
    std::unique_ptr<JobItem> readyJobItem;
    {
        DAVA::LockGuard<DAVA::Mutex> guard(mutex);
        isFinished = true;
        readyJobItem = std::move(jobItem);
        r = result;
        result = TextureInfo();
    }
    readyCallback(r, readyJobItem.get());
}
