#ifndef __LOADING_TEST_H__
#define __LOADING_TEST_H__

#include "BaseTest.h"
#include "Infrastructure/Screen/BaseScreen.h"
#include "Infrastructure/Utils/ControlHelpers.h"
#include "MemoryManager/MemoryProfiler.h"
#include "TeamCityTestsOutput.h"

class LoadingTest : public BaseTest
{
public:
    static const String TEST_NAME;

    LoadingTest(const TestParams& testParams);

    void OnStart() override;
    void OnFinish() override;

    void Update(float32 timeElapsed) override;

    bool IsFinished() const override;

    void OnActive() override;
    void OnInactive() override;

protected:
    void LoadResources() override;
    void UnloadResources() override;

    void CreateUI() override{};
    void UpdateUI() override{};

    void PerformTestLogic(float32 timeElapsed) override{};

private:
    static const uint32 JOB_GROUP_MAX_COUNT = 10;

    uint32 loadingDelayFrames = 0U;

    class LoadJob
    {
    public:
        LoadJob(const DAVA::FilePath& scenePath, const String& jobText, uint32 groupIndex);
        virtual ~LoadJob() = default;

        virtual void Excecute();
        virtual bool IsFinished();
        bool IsExcecuted();

        uint64 GetLoadTime();
        const String& GetJobText();

        uint32 GetGroupIndex();

    protected:
        FilePath scenePath;
        String jobText;
        uint64 loadingTime = 0U; //ms
        uint32 groupIndex = 0;
        bool excecuted = false;
    };

    class LoadThreadJob : public LoadJob
    {
    public:
        LoadThreadJob(const DAVA::FilePath& scenePath, const String& jobText, uint32 groupIndex);
        virtual ~LoadThreadJob();

        void Excecute() override;
        bool IsFinished() override;

    protected:
        Thread* loadingThread = nullptr;
    };

    List<LoadJob*> loadJobs;
    Array<uint64, JOB_GROUP_MAX_COUNT> loadResults = {};
    Array<uint32, JOB_GROUP_MAX_COUNT> loadGroupSize = {};

    UIStaticText* loadingText;
    UIStaticText* testText;
};

#endif
