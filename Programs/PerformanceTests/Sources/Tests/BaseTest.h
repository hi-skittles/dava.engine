#ifndef __BASE_TEST_H__
#define __BASE_TEST_H__

#include "Infrastructure/Screen/BaseScreen.h"
#include "Infrastructure/Utils/ControlHelpers.h"
#include "MemoryManager/MemoryProfiler.h"
#include "TeamCityTestsOutput.h"

namespace DAVA
{
class UICustomUpdateDeltaComponent;
}
class BaseTest : public BaseScreen
{
public:
    struct FrameInfo
    {
        FrameInfo()
            : delta(0.0f)
        {
        }
        FrameInfo(float32 _delta)
            : delta(_delta)
        {
        }

        float32 delta;
    };

    struct TestParams
    {
        TestParams()
            : targetTime(0)
            , startTime(0)
            , endTime(0)
            , targetFramesCount(0)
            , targetFrameDelta(0.0f)
            , frameForDebug(0)
            , maxDelta(0.0f)
        {
        }

        int32 targetTime;

        int32 startTime;
        int32 endTime;

        int32 targetFramesCount;
        float32 targetFrameDelta;

        int32 frameForDebug;

        // stop test logic when frameDelta greater than maxDelta
        float32 maxDelta;

        String sceneName;
        String scenePath;
    };

    struct StatisticData
    {
        float32 minDelta;
        float32 maxDelta;
        float32 averageDelta;

        float32 testTime;
        float32 elapsedTime;

        uint64 maxAllocatedMemory;
    };

    BaseTest(const String& testName, const TestParams& testParams);

    void OnStart() override;
    void OnFinish() override;

    void BeginFrame() override;
    void EndFrame() override;

    void ShowUI(bool visible);
    bool IsUIVisible() const;

    const String& GetTestName() const;
    virtual const String& GetSceneName() const;

    bool IsFinished() const override;

    void SetParams(const TestParams& testParams);
    void MergeParams(const TestParams& otherParams);
    const TestParams& GetParams() const;

    float32 GetOverallTestTime() const;
    uint64 GetElapsedTime() const;

    int32 GetAbsoluteFrameNumber() const;
    int32 GetTestFrameNumber() const;

    Scene* GetScene() const;
    const Vector<FrameInfo>& GetFramesInfo() const;
    float32 GetCurrentFrameDelta() const;

    static const uint32 FRAME_OFFSET;

protected:
    virtual ~BaseTest(){};

    void LoadResources() override;
    void UnloadResources() override;
    void Update(float32 timeElapsed) override;

    virtual void PrintStatistic(const Vector<FrameInfo>& frames);

    virtual void CreateUI();
    virtual void UpdateUI();

    uint32 GetAllocatedMemory();

    virtual void PerformTestLogic(float32 timeElapsed) = 0;

private:
    Vector<FrameInfo> frames;

    String testName;
    String sceneName;

    TestParams testParams;

    uint32 frameNumber;

    uint64 startTime;
    uint64 elapsedTime;

    float32 overallTestTime;

    float32 minDelta;
    float32 maxDelta;
    float32 currentFrameDelta;

    Scene* scene;
    UI3DView* sceneView;

    ScopedPtr<DAVA::UIControl> uiRoot;

    UIStaticText* testNameText;
    UIStaticText* maxFPSText;
    UIStaticText* minFPSText;
    UIStaticText* fpsText;
    UIStaticText* testTimeText;
    UIStaticText* elapsedTimeText;
    UIStaticText* framesRenderedText;

    uint32 maxAllocatedMemory;
    UICustomUpdateDeltaComponent* sceneCustomDeltaComponent;
};

inline const Vector<BaseTest::FrameInfo>& BaseTest::GetFramesInfo() const
{
    return frames;
}

inline Scene* BaseTest::GetScene() const
{
    return scene;
}

inline const String& BaseTest::GetTestName() const
{
    return testName;
}

inline uint64 BaseTest::GetElapsedTime() const
{
    return elapsedTime;
}

inline float32 BaseTest::GetOverallTestTime() const
{
    return overallTestTime;
}

inline int32 BaseTest::GetAbsoluteFrameNumber() const
{
    return frameNumber;
}

inline int32 BaseTest::GetTestFrameNumber() const
{
    int32 frameDiff = frameNumber - FRAME_OFFSET;
    return frameDiff < 0 ? 0 : frameDiff;
}

inline void BaseTest::SetParams(const TestParams& _testParams)
{
    DVASSERT(frameNumber == 1, "Can't set params after test started");

    this->testParams = _testParams;
}

inline const BaseTest::TestParams& BaseTest::GetParams() const
{
    return testParams;
}

inline void BaseTest::ShowUI(bool visible)
{
    uiRoot->SetVisibilityFlag(visible);
}

inline bool BaseTest::IsUIVisible() const
{
    return uiRoot->GetVisibilityFlag();
}

inline void BaseTest::MergeParams(const TestParams& otherParams)
{
    testParams.targetTime = otherParams.targetTime;
    testParams.startTime = otherParams.startTime;
    testParams.endTime = otherParams.endTime;
    testParams.targetFramesCount = otherParams.targetFramesCount;
    testParams.targetFrameDelta = otherParams.targetFrameDelta;
    testParams.frameForDebug = otherParams.frameForDebug;
    testParams.maxDelta = otherParams.maxDelta;
}
inline float32 BaseTest::GetCurrentFrameDelta() const
{
    return currentFrameDelta;
}

#endif
