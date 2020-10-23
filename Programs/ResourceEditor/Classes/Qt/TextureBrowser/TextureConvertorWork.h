#pragma once

#include "DAVAEngine.h"
#include "TextureConvertMode.h"
#include "TextureInfo.h"

#include "Render/TextureDescriptor.h"

#include "Concurrency/Mutex.h"

struct JobItem
{
    int id;
    int type;
    eTextureConvertMode convertMode;
    const DAVA::TextureDescriptor* descriptor;

    JobItem()
        : id(0)
        , type(0)
        , convertMode(CONVERT_NOT_EXISTENT)
        , descriptor(NULL)
    {
    }
};

class JobStack
{
public:
    JobStack();
    ~JobStack();

    bool push(const JobItem& item);
    JobItem* pop();
    int size();

private:
    struct JobItemWrapper : public JobItem
    {
        JobItemWrapper(const JobItem& item);

        JobItemWrapper* next;
        JobItemWrapper* prev;
    };

    JobItemWrapper* head;

    int itemsCount;
};

class JobWatcher
{
public:
    using TJobFunction = DAVA::Function<TextureInfo(const JobItem*)>;
    using TReadyCallback = DAVA::Function<void(const TextureInfo&, const JobItem*)>;

    JobWatcher(DAVA::JobManager* manager);
    ~JobWatcher();

    void Init(const TJobFunction& convertFunction, const TReadyCallback& readyCallback);

    bool IsFinished() const;
    void RunJob(std::unique_ptr<JobItem>&& item);
    const JobItem* GetCurrentJobItem() const;

private:
    void OnConvert();
    void OnReady();

    JobItem* GetCurrentJobItemImpl();

private:
    class Impl;
    std::shared_ptr<Impl> impl;
    TJobFunction convertFunction;
    TReadyCallback readyCallback;
    DAVA::JobManager* manager = nullptr;
    std::unique_ptr<JobItem> jobItem = nullptr;
    TextureInfo result;
    bool isFinished = true;
    mutable DAVA::Mutex mutex;
};
