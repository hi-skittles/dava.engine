#include "LoadingTest.h"

namespace LoadingTestDetails
{
static const uint32 LOADING_DELAY_FRAMES = 20;
static const uint32 LOADING_THREAD_STACK_SIZE = 1024 * 1024; // 1 mb

static const int32 FPS_LOADING = 10;
static const int32 FPS_REGULAR = 60;
}

const String LoadingTest::TEST_NAME = "LoadingTest";

////////////////////////////////////////////////////////////////////////////////////////////

LoadingTest::LoadJob::LoadJob(const DAVA::FilePath& _scenePath, const String& _jobText, uint32 _groupIndex)
    : scenePath(_scenePath)
    , jobText(_jobText)
    , groupIndex(_groupIndex)
{
}

void LoadingTest::LoadJob::Excecute()
{
    excecuted = true;

    uint64 time = SystemTimer::GetMs();

    ScopedPtr<Scene> scene(new Scene());
    scene->LoadScene(scenePath);

    loadingTime = SystemTimer::GetMs() - time;
}

bool LoadingTest::LoadJob::IsFinished()
{
    return excecuted;
}

bool LoadingTest::LoadJob::IsExcecuted()
{
    return excecuted;
}

uint64 LoadingTest::LoadJob::GetLoadTime()
{
    return loadingTime;
}

const String& LoadingTest::LoadJob::GetJobText()
{
    return jobText;
};

uint32 LoadingTest::LoadJob::GetGroupIndex()
{
    return groupIndex;
};

////////////////////////////////////////////////////////////////////////////////////////////

LoadingTest::LoadThreadJob::LoadThreadJob(const DAVA::FilePath& scenePath, const String& jobText, uint32 groupID)
    : LoadJob(scenePath, jobText, groupID)
{
}

LoadingTest::LoadThreadJob::~LoadThreadJob()
{
    SafeRelease(loadingThread);
}

void LoadingTest::LoadThreadJob::Excecute()
{
    excecuted = true;

    loadingThread = Thread::Create([this]()
                                   {
                                       LoadJob::Excecute();
                                   });
    loadingThread->SetStackSize(LoadingTestDetails::LOADING_THREAD_STACK_SIZE);
    loadingThread->Start();
}

bool LoadingTest::LoadThreadJob::IsFinished()
{
    return (loadingThread) && (loadingThread->GetState() == Thread::STATE_ENDED);
}

////////////////////////////////////////////////////////////////////////////////////////////

LoadingTest::LoadingTest(const TestParams& _testParams)
    : BaseTest(TEST_NAME, _testParams)
{
}

void LoadingTest::LoadResources()
{
    ScopedPtr<Font> font(FTFont::Create("~res:/Fonts/korinna.ttf"));

    loadingText = new UIStaticText();
    loadingText->SetFont(font);
    loadingText->SetFontSize(18.f);
    loadingText->SetTextColor(Color(0.f, 1.f, 0.f, 1.f));
    loadingText->SetTextAlign(ALIGN_HCENTER | ALIGN_VCENTER);
    loadingText->SetRect(DAVA::GetEngineContext()->uiControlSystem->vcs->GetFullScreenVirtualRect());
    AddControl(loadingText);

    testText = new UIStaticText();
    testText->SetFont(font);
    testText->SetFontSize(12.f);
    testText->SetTextColor(Color(0.f, 1.f, 0.f, 1.f));
    testText->SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);
    testText->SetRect(Rect(10.f, 10.f, 150.f, 10.f));
    testText->SetText(UTF8Utils::EncodeToWideString(TEST_NAME));
    AddControl(testText);

    FilePath scenePath("~res:/3d/Maps/" + GetParams().scenePath);

    loadJobs.push_back(new LoadJob(scenePath, Format("Loading map '%s' on main thread (0)...", GetParams().sceneName.c_str()), 0));
    for (int32 i = 0; i < 5; ++i)
        loadJobs.push_back(new LoadJob(scenePath, Format("Loading map '%s' on main thread (%d)...", GetParams().sceneName.c_str(), i + 1), 1));

    loadJobs.push_back(new LoadThreadJob(scenePath, Format("Loading map '%s' on loading thread (0)...", GetParams().sceneName.c_str()), 2));
    for (int32 i = 0; i < 5; ++i)
        loadJobs.push_back(new LoadThreadJob(scenePath, Format("Loading map '%s' on loading thread (%d)...", GetParams().sceneName.c_str(), i + 1), 3));

    loadingText->SetText(UTF8Utils::EncodeToWideString(loadJobs.front()->GetJobText()));

    loadingDelayFrames = LoadingTestDetails::LOADING_DELAY_FRAMES;
}

void LoadingTest::UnloadResources()
{
    SafeRelease(loadingText);
    SafeRelease(testText);

    DVASSERT(loadJobs.size() == 0);
}

void LoadingTest::OnActive()
{
    Renderer::SetDesiredFPS(LoadingTestDetails::FPS_LOADING);
}

void LoadingTest::OnInactive()
{
    Renderer::SetDesiredFPS(LoadingTestDetails::FPS_REGULAR);
}

void LoadingTest::Update(float32 timeElapsed)
{
    BaseScreen::Update(timeElapsed);
    if (loadJobs.size())
    {
        if (loadingDelayFrames == 0)
        {
            loadJobs.front()->Excecute();
            loadingDelayFrames = LoadingTestDetails::LOADING_DELAY_FRAMES;
        }

        if (!loadJobs.front()->IsExcecuted())
        {
            --loadingDelayFrames;
            DVASSERT(loadingDelayFrames < LoadingTestDetails::LOADING_DELAY_FRAMES);
        }

        if (loadJobs.front()->IsFinished())
        {
            DVASSERT(loadJobs.front()->GetGroupIndex() < JOB_GROUP_MAX_COUNT && loadJobs.front()->GetGroupIndex() < JOB_GROUP_MAX_COUNT);

            loadResults[loadJobs.front()->GetGroupIndex()] += loadJobs.front()->GetLoadTime();
            ++loadGroupSize[loadJobs.front()->GetGroupIndex()];

            SafeDelete(loadJobs.front());
            loadJobs.pop_front();

            loadingText->SetText(loadJobs.size() ? UTF8Utils::EncodeToWideString(loadJobs.front()->GetJobText()) : L"");
        }
    }
}

void LoadingTest::OnStart()
{
    Logger::Info(TeamcityPerformanceTestsOutput::FormatTestStarted(GetSceneName()).c_str());
}

void LoadingTest::OnFinish()
{
    for (int32 i = 0; i < JOB_GROUP_MAX_COUNT; ++i)
    {
        if (loadGroupSize[i])
        {
            Logger::Info(TeamcityPerformanceTestsOutput::FormatBuildStatistic(DAVA::Format("Loading%d", i), DAVA::Format("%lld", loadResults[i] / loadGroupSize[i])).c_str());
        }
    }

    Logger::Info(TeamcityPerformanceTestsOutput::FormatTestFinished(GetSceneName()).c_str());
}

bool LoadingTest::IsFinished() const
{
    return (loadJobs.size() == 0);
}
