#include "BaseTest.h"
#include <UI/Update/UIUpdateComponent.h>
#include <UI/Update/UICustomUpdateDeltaComponent.h>

const uint32 BaseTest::FRAME_OFFSET = 5;

BaseTest::BaseTest(const String& _testName, const TestParams& _testParams)
    : testName(_testName)
    , testParams(_testParams)
    , frameNumber(1)
    , startTime(0)
    , overallTestTime(0.0f)
    , minDelta(std::numeric_limits<float>::max())
    , maxDelta(std::numeric_limits<float>::min())
    , currentFrameDelta(0.0f)
    , uiRoot(new DAVA::UIControl())
    , maxAllocatedMemory(0)
{
    sceneName = testName + ": " + GetParams().sceneName;
    GetOrCreateComponent<UIUpdateComponent>();
}

void BaseTest::LoadResources()
{
    const Size2i& size = DAVA::GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize();

    scene = new Scene();

    Rect rect;
    rect.x = 0.0f;
    rect.y = 0.0f;
    rect.dx = static_cast<float32>(size.dx);
    rect.dy = static_cast<float32>(size.dy);

    sceneView = new UI3DView(rect);
    sceneView->SetScene(scene);
    sceneCustomDeltaComponent = sceneView->GetOrCreateComponent<UICustomUpdateDeltaComponent>();

    AddControl(sceneView);

    CreateUI();
}

void BaseTest::UnloadResources()
{
    sceneCustomDeltaComponent = nullptr;
    SafeRelease(scene);

    RemoveControl(sceneView);
    SafeRelease(sceneView);

    uiRoot->RemoveAllControls();
}

void BaseTest::CreateUI()
{
    UIYamlLoader::LoadFonts("~res:/UI/Fonts/fonts.yaml");

    DefaultUIPackageBuilder builder;
    UIPackageLoader().LoadPackage(ControlHelpers::GetPathToUIYaml("ReportItem.yaml"), &builder);
    DAVA::UIControl* reportItem = builder.GetPackage()->GetControl("ReportItem");

    uiRoot->SetPosition(Vector2(0.0f, 0.0f));
    reportItem->SetPosition(Vector2(0.0f, 0.0f));

    testNameText = reportItem->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::TEST_NAME_PATH);
    testNameText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%s", GetSceneName().c_str())));

    UIStaticText* fieldMinFpsText = reportItem->FindByPath<UIStaticText*>("MinDelta/MinDeltaText");
    fieldMinFpsText->SetText(UTF8Utils::EncodeToWideString("Max FPS"));

    maxFPSText = reportItem->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::MIN_DELTA_PATH);
    maxFPSText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", 0.0f)));

    UIStaticText* fieldMaxFpsText = reportItem->FindByPath<UIStaticText*>("MaxDelta/MaxDeltaText");
    fieldMaxFpsText->SetText(UTF8Utils::EncodeToWideString("Min FPS"));

    minFPSText = reportItem->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::MAX_DELTA_PATH);
    minFPSText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", 0.0f)));

    UIStaticText* fieldFpsText = reportItem->FindByPath<UIStaticText*>("AverageDelta/AverageDeltaText");
    fieldFpsText->SetText(UTF8Utils::EncodeToWideString("FPS"));

    fpsText = reportItem->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::AVERAGE_DELTA_PATH);
    fpsText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", 0.0f)));

    testTimeText = reportItem->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::TEST_TIME_PATH);
    testTimeText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", 0.0f)));

    elapsedTimeText = reportItem->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::ELAPSED_TIME_PATH);
    elapsedTimeText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", 0.0f)));

    framesRenderedText = reportItem->FindByPath<UIStaticText*>(ControlHelpers::ReportItem::FRAMES_RENDERED_PATH);
    framesRenderedText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%d", 0)));

    AddControl(uiRoot);
    uiRoot->AddControl(reportItem);
}

void BaseTest::UpdateUI()
{
    float32 fps = currentFrameDelta > 0.001f ? 1.0f / currentFrameDelta : 0.0f;

    maxFPSText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", 1.0f / minDelta)));
    minFPSText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", 1.0f / maxDelta)));
    fpsText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", fps)));
    testTimeText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", overallTestTime)));
    elapsedTimeText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%f", elapsedTime / 1000.0f)));
    framesRenderedText->SetText(UTF8Utils::EncodeToWideString(DAVA::Format("%d", GetTestFrameNumber())));
}

#if defined(__DAVAENGINE_APPLE__)
#include <mach/mach.h>
#endif

uint32 BaseTest::GetAllocatedMemory()
{
    uint32 memory = 0;
    
#if defined(DAVA_MEMORY_PROFILING_ENABLE)
    memory = MemoryManager::Instance()->GetTrackedMemoryUsage();
#elif defined(__DAVAENGINE_APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
    if (KERN_SUCCESS == task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &size))
    {
        memory = static_cast<uint32>(info.resident_size);
    }
#endif

    return memory;
}
void BaseTest::OnStart()
{
    Logger::Info(TeamcityPerformanceTestsOutput::FormatTestStarted(GetSceneName()).c_str());
}

void BaseTest::OnFinish()
{
    PrintStatistic(GetFramesInfo());

    Logger::Info(TeamcityPerformanceTestsOutput::FormatTestFinished(GetSceneName()).c_str());
}

void BaseTest::PrintStatistic(const Vector<FrameInfo>& frames)
{
    size_t framesCount = GetFramesInfo().size();

    for (const auto& frameInfo : frames)
    {
        Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                     TeamcityPerformanceTestsOutput::FRAME_DELTA,
                     DAVA::Format("%f", frameInfo.delta))
                     .c_str());
    }

    Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                 TeamcityPerformanceTestsOutput::MIN_DELTA,
                 DAVA::Format("%f", minDelta))
                 .c_str());

    Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                 TeamcityPerformanceTestsOutput::MAX_DELTA,
                 DAVA::Format("%f", maxDelta))
                 .c_str());

    Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                 TeamcityPerformanceTestsOutput::AVERAGE_DELTA,
                 DAVA::Format("%f", overallTestTime / framesCount))
                 .c_str());

    Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                 TeamcityPerformanceTestsOutput::MAX_FPS,
                 DAVA::Format("%f", 1.0f / minDelta))
                 .c_str());

    Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                 TeamcityPerformanceTestsOutput::MIN_FPS,
                 DAVA::Format("%f", 1.0f / maxDelta))
                 .c_str());

    Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                 TeamcityPerformanceTestsOutput::AVERAGE_FPS,
                 DAVA::Format("%f", 1.0f / (overallTestTime / framesCount)))
                 .c_str());

    Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                 TeamcityPerformanceTestsOutput::TEST_TIME,
                 DAVA::Format("%f", overallTestTime))
                 .c_str());

    Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                 TeamcityPerformanceTestsOutput::TIME_ELAPSED,
                 DAVA::Format("%f", elapsedTime / 1000.0f))
                 .c_str());

    Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(
                 TeamcityPerformanceTestsOutput::MAX_MEM_USAGE,
                 DAVA::Format("%d", maxAllocatedMemory))
                 .c_str());
}

void BaseTest::Update(float32 timeElapsed)
{
    BaseScreen::Update(timeElapsed);
    uint32 allocatedMem = GetAllocatedMemory();
    if (allocatedMem > maxAllocatedMemory)
    {
        maxAllocatedMemory = allocatedMem;
    }

    bool isFrameForDebug = testParams.frameForDebug > 0 && GetTestFrameNumber() >= testParams.frameForDebug;
    bool isGreaterMaxDelta = testParams.maxDelta > 0.001f && timeElapsed >= testParams.maxDelta;

    float32 delta = 0.0f;
    float32 currentTimeMs = overallTestTime * 1000;

    if (frameNumber > FRAME_OFFSET)
    {
        if (currentTimeMs >= testParams.startTime && currentTimeMs <= testParams.endTime)
        {
            if (isGreaterMaxDelta)
            {
                Logger::Info(DAVA::Format("Time delta: %f \nMaxDelta: %f \nFrame : %d", timeElapsed, testParams.maxDelta, frameNumber).c_str());
            }
            if (isFrameForDebug)
            {
                Logger::Info(DAVA::Format("Frame for debug: %d", frameNumber - BaseTest::FRAME_OFFSET).c_str());
            }

            frames.push_back(FrameInfo(timeElapsed));
        }

        if (!isFrameForDebug && !isGreaterMaxDelta)
        {
            delta = testParams.targetFrameDelta > 0.001f ? testParams.targetFrameDelta : timeElapsed;
        }

        overallTestTime += delta;
        currentFrameDelta = delta;

        if (delta < minDelta)
        {
            minDelta = delta;
        }
        if (delta > maxDelta)
        {
            maxDelta = delta;
        }

        if (IsUIVisible())
        {
            UpdateUI();
        }

        PerformTestLogic(currentFrameDelta);
    }

    if (sceneCustomDeltaComponent)
    {
        sceneCustomDeltaComponent->SetDelta(currentFrameDelta);
    }
}

void BaseTest::BeginFrame()
{
    if (0 == startTime && frameNumber >= FRAME_OFFSET)
    {
        startTime = SystemTimer::GetFrameTimestampMs();
    }
}

void BaseTest::EndFrame()
{
    frameNumber++;

    elapsedTime = SystemTimer::GetFrameTimestampMs() - startTime;
}

bool BaseTest::IsFinished() const
{
    if (testParams.targetFramesCount > 0)
    {
        if (GetTestFrameNumber() > testParams.targetFramesCount)
        {
            return true;
        }
    }
    else if (testParams.targetTime > 0 && (overallTestTime * 1000) >= testParams.targetTime)
    {
        return true;
    }

    return false;
}

const String& BaseTest::GetSceneName() const
{
    return sceneName;
}
