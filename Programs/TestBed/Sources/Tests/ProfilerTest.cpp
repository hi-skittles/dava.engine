#include "Tests/ProfilerTest.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "UI/UIStaticText.h"
#include "UI/UIButton.h"
#include "UI/UIControlSystem.h"
#include "UI/Update/UIUpdateComponent.h"
#include "UI/Render/UIDebugRenderComponent.h"
#include <UI/Layouts/UISizePolicyComponent.h>
#include "Render/2D/Font.h"
#include "Scene3D/Scene.h"
#include "Debug/ProfilerOverlay.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerGPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Debug/ProfilerUtils.h"
#include "Engine/Engine.h"

#include <sstream>

using namespace DAVA;

static const char* PROFILER_TEST_UPDATE_MARKER = "ProfielrTest::Update";
static const char* PROFILER_TEST_FUNC0_MARKER = "TestFunction0";
static const char* PROFILER_TEST_FUNC1_MARKER = "TestFunction1";
static const char* PROFILER_TEST_FUNC2_MARKER = "TestFunction2";

static const char* PROFILER_TEST_DUMP_JSON_PATH = "~doc:/TestBed/profiler_test_dump.json";

ProfilerTest::ProfilerTest(TestBed& app)
    : BaseScreen(app, "ProfilerTest")
{
    GetOrCreateComponent<UIUpdateComponent>();
}

void ProfilerTest::LoadResources()
{
    customCPUProfiler = new ProfilerCPU();

    scene = new Scene();
    scene->LoadScene("~res:/TestBed/3d/Maps/test_scene/test_scene.sc2");

    ScopedPtr<Camera> camera(new Camera());

    Size2i screenSize = GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize();

    float32 aspect = float32(screenSize.dy) / float32(screenSize.dx);
    camera->SetupPerspective(70.f, aspect, 0.5f, 2500.f);
    camera->SetLeft(Vector3(0.608f, -0.793f, 0.f));
    camera->SetUp(Vector3(0.f, 0.f, 1.f));
    camera->SetTarget(Vector3(-33.61f, -15.987f, 16.671f));
    camera->SetPosition(Vector3(-41.546f, -22.069f, 16.88f));

    scene->AddCamera(camera);
    scene->SetCurrentCamera(camera);

    Rect screenRect = Rect(0.f, 0.f, float32(screenSize.dx), float32(screenSize.dy));
    SetRect(screenRect);
    ScopedPtr<UI3DView> sceneView(new UI3DView(screenRect));
    sceneView->SetScene(scene);
    AddControl(sceneView);

    textFont = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    dumpFont = FTFont::Create("~res:/TestBed/Fonts/DroidSansMono.ttf");

    AddControl(ScopedPtr<UIButton>(CreateButton(Rect(5.f, 5.f, 400.f, 35.f), L"Start/Stop Global CPU-Profiler", Message(this, &ProfilerTest::OnGlobalCPUProfiler))));
    AddControl(ScopedPtr<UIButton>(CreateButton(Rect(5.f, 45.f, 400.f, 35.f), L"Start/Stop Custom CPU-Profiler", Message(this, &ProfilerTest::OnCustomCPUProfiler))));
    AddControl(ScopedPtr<UIButton>(CreateButton(Rect(5.f, 85.f, 400.f, 35.f), L"Start/Stop GPU-Profiler", Message(this, &ProfilerTest::OnGPUProfiler))));
    AddControl(ScopedPtr<UIButton>(CreateButton(Rect(5.f, 145.f, 400.f, 35.f), L"Overlay with Global CPU-Profiler", Message(this, &ProfilerTest::OnGlobalCPUProfilerOverlay))));
    AddControl(ScopedPtr<UIButton>(CreateButton(Rect(5.f, 185.f, 400.f, 35.f), L"Overlay with Custom CPU-Profiler", Message(this, &ProfilerTest::OnCustomCPUProfilerOverlay))));

    profilersText[0] = CreateStaticText(Rect(420.f, 5.f, 400.f, 35.f), L"Global CPU-Profilers Started", textFont, 20.f, Color(0.f, 1.f, 0.f, 1.f));
    profilersText[1] = CreateStaticText(Rect(420.f, 5.f, 400.f, 35.f), L"Global CPU-Profilers Stopped", textFont, 20.f, Color(1.f, 0.f, 0.f, 1.f));
    profilersText[2] = CreateStaticText(Rect(420.f, 45.f, 400.f, 35.f), L"Custom CPU-Profilers Started", textFont, 20.f, Color(0.f, 1.f, 0.f, 1.f));
    profilersText[3] = CreateStaticText(Rect(420.f, 45.f, 400.f, 35.f), L"Custom CPU-Profilers Stopped", textFont, 20.f, Color(1.f, 0.f, 0.f, 1.f));
    profilersText[4] = CreateStaticText(Rect(420.f, 85.f, 400.f, 35.f), L"GPU-Profilers Started", textFont, 20.f, Color(0.f, 1.f, 0.f, 1.f));
    profilersText[5] = CreateStaticText(Rect(420.f, 85.f, 400.f, 35.f), L"GPU-Profilers Stopped", textFont, 20.f, Color(1.f, 0.f, 0.f, 1.f));

    for (int32 i = 0; i < 6; ++i)
    {
        AddControl(profilersText[i]);
        profilersText[i]->Release();
    }

    profilersText[0]->SetVisibilityFlag(false);
    profilersText[2]->SetVisibilityFlag(false);
    profilersText[4]->SetVisibilityFlag(false);

    AddControl(ScopedPtr<UIButton>(CreateButton(Rect(5.f, 245.f, 400.f, 35.f), L"Dump (Global Profiler)", Message(this, &ProfilerTest::OnDumpAverage, ProfilerCPU::globalProfiler))));
    AddControl(ScopedPtr<UIButton>(CreateButton(Rect(5.f, 285.f, 400.f, 35.f), L"Dump (Custom Profiler)", Message(this, &ProfilerTest::OnDumpAverage, customCPUProfiler))));

    AddControl(ScopedPtr<UIButton>(CreateButton(Rect(5.f, 345.f, 400.f, 35.f), L"Make snapshot (Custom Profiler)", Message(this, &ProfilerTest::OnMakeSnapshot))));
    AddControl(ScopedPtr<UIButton>(CreateButton(Rect(5.f, 385.f, 400.f, 35.f), L"Dump snapshot (Custom Profiler)", Message(this, &ProfilerTest::OnDumpAverageSnapshot))));

    AddControl(ScopedPtr<UIButton>(CreateButton(Rect(5.f, 445.f, 400.f, 35.f), L"Dump JSON (Custom Profiler)", Message(this, &ProfilerTest::OnDumpJSON))));
    AddControl(ScopedPtr<UIButton>(CreateButton(Rect(5.f, 485.f, 400.f, 35.f), L"Dump JSON (Global CPU-GPU)", Message(this, &ProfilerTest::OnDumpGlobalCPUGPU))));
    AddControl(ScopedPtr<UIStaticText>(CreateStaticText(Rect(5.f, 525.f, 400.f, 35.f), L"JSON path: \"" + UTF8Utils::EncodeToWideString(PROFILER_TEST_DUMP_JSON_PATH) + L"\"", textFont, 20.f, Color::White)));

    dumpScrollView = new UIScrollView();
    dumpScrollView->GetOrCreateComponent<UIDebugRenderComponent>();
    UIControlBackground* dumpScrollViewBg = dumpScrollView->GetOrCreateComponent<UIControlBackground>();
    dumpScrollViewBg->SetColor(Color(0.f, 0.f, 0.f, 0.5f));
    dumpScrollViewBg->SetDrawType(UIControlBackground::DRAW_FILL);
    dumpScrollView->SetRect(Rect(425.f, 245.f, 500.f, 400.f));
    AddControl(dumpScrollView);
    dumpScrollView->Release();

    dumpText = CreateStaticText(Rect(0.f, 0.f, 500.f, 400.f), L"Dump Text", dumpFont, 10.f, Color(0.f, 1.f, 0.f, 1.f));
    dumpText->SetMultilineType(UIStaticText::MULTILINE_ENABLED);
    dumpText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    dumpText->GetOrCreateComponent<UISizePolicyComponent>()->SetVerticalPolicy(UISizePolicyComponent::eSizePolicy::PERCENT_OF_CONTENT);
    dumpScrollView->AddControlToContainer(dumpText);
    dumpScrollView->RecalculateContentSize();

    BaseScreen::LoadResources();

    ProfilerOverlay::globalProfilerOverlay->SetInputEnabled(true);
    defaultCPUMarkers = ProfilerOverlay::globalProfilerOverlay->GetInterestMarkers();

    ProfilerCPU::globalProfiler->Stop();
    ProfilerGPU::globalProfiler->Stop();
}

void ProfilerTest::UnloadResources()
{
    SafeDelete(customCPUProfiler);

    SafeRelease(scene);
    SafeRelease(textFont);
    SafeRelease(dumpFont);

    ProfilerCPU::globalProfiler->Stop();
    ProfilerGPU::globalProfiler->Stop();

    ProfilerOverlay::globalProfilerOverlay->SetEnabled(false);
    ProfilerOverlay::globalProfilerOverlay->SetInputEnabled(false);

    BaseScreen::UnloadResources();
}

UIButton* ProfilerTest::CreateButton(const Rect& rect, const WideString& text, const Message& msg)
{
    DVASSERT(textFont);

    UIButton* button = new UIButton(rect);
    button->SetStateText(UIControl::STATE_NORMAL, text);
    button->SetStateTextAlign(UIControl::STATE_NORMAL, ALIGN_HCENTER | ALIGN_VCENTER);
    button->SetStateFont(UIControl::STATE_NORMAL, textFont);
    button->SetStateFontSize(UIControl::STATE_NORMAL, 20.f);
    button->SetStateFontColor(UIControl::STATE_NORMAL, Color::White);
    button->SetStateFontColor(UIControl::STATE_PRESSED_INSIDE, Color(0.7f, 0.7f, 0.7f, 1.f));
    button->AddEvent(UIButton::EVENT_TOUCH_UP_INSIDE, msg);
    button->GetOrCreateComponent<UIDebugRenderComponent>();

    return button;
}

UIStaticText* ProfilerTest::CreateStaticText(const Rect& rect, const WideString& text, DAVA::Font* font, float32 fontSize, const DAVA::Color& color)
{
    DVASSERT(textFont);

    UIStaticText* staticText = new UIStaticText(rect);
    staticText->SetText(text);
    staticText->SetFont(font);
    staticText->SetFontSize(fontSize);
    staticText->SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);
    staticText->SetTextColor(color);
    staticText->SetFittingOption(TextBlock::FITTING_REDUCE);

    return staticText;
}

void ProfilerTest::Update(float32 timeElapsed)
{
    DAVA_PROFILER_CPU_SCOPE(PROFILER_TEST_UPDATE_MARKER);
    DAVA_PROFILER_CPU_SCOPE_CUSTOM_WITH_FRAME_INDEX(PROFILER_TEST_UPDATE_MARKER, customCPUProfiler, Engine::Instance()->GetGlobalFrameIndex());

    profilersText[0]->SetVisibilityFlag(ProfilerCPU::globalProfiler->IsStarted());
    profilersText[1]->SetVisibilityFlag(!ProfilerCPU::globalProfiler->IsStarted());
    profilersText[2]->SetVisibilityFlag(customCPUProfiler->IsStarted());
    profilersText[3]->SetVisibilityFlag(!customCPUProfiler->IsStarted());
    profilersText[4]->SetVisibilityFlag(ProfilerGPU::globalProfiler->IsStarted());
    profilersText[5]->SetVisibilityFlag(!ProfilerGPU::globalProfiler->IsStarted());

    TestFunction0();
    TestFunction1();
}

void ProfilerTest::TestFunction0()
{
    DAVA_PROFILER_CPU_SCOPE(PROFILER_TEST_FUNC0_MARKER);
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(PROFILER_TEST_FUNC0_MARKER, customCPUProfiler);

    TestFunction1();
    TestFunction2();
}

void ProfilerTest::TestFunction1()
{
    DAVA_PROFILER_CPU_SCOPE(PROFILER_TEST_FUNC1_MARKER);
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(PROFILER_TEST_FUNC1_MARKER, customCPUProfiler);

    volatile Matrix4 mat1 = Matrix4::IDENTITY;
    volatile Matrix4 mat2 = Matrix4::MakeTranslation(Vector3(1.f, 1.f, 1.f));

    for (int32 i = 0; i < 10000; ++i)
        const_cast<Matrix4&>(mat1) = const_cast<Matrix4&>(mat1) * const_cast<Matrix4&>(mat2);
}

void ProfilerTest::TestFunction2()
{
    DAVA_PROFILER_CPU_SCOPE(PROFILER_TEST_FUNC2_MARKER);
    DAVA_PROFILER_CPU_SCOPE_CUSTOM(PROFILER_TEST_FUNC2_MARKER, customCPUProfiler);

    volatile Matrix4 mat1 = Matrix4::IDENTITY;
    volatile Matrix4 mat2 = Matrix4::MakeRotation(Vector3(1.f, 1.f, 1.f), -1.f);

    for (int32 i = 0; i < 10000; ++i)
        const_cast<Matrix4&>(mat1) = const_cast<Matrix4&>(mat1) * const_cast<Matrix4&>(mat2);
}

void ProfilerTest::DumpAverageToUI(DAVA::ProfilerCPU* profiler, DAVA::int32 snapshotID)
{
    DVASSERT(profiler);

    bool started = profiler->IsStarted();
    if (snapshotID == ProfilerCPU::NO_SNAPSHOT_ID && started)
        profiler->Stop();

    const char* marker = (profiler == customCPUProfiler) ? PROFILER_TEST_UPDATE_MARKER : ProfilerCPUMarkerName::ENGINE_ON_FRAME;
    std::stringstream stream;
    profiler->DumpAverage(marker, 10, stream, snapshotID);

    dumpText->SetUtf8Text(stream.str());
    dumpText->UpdateLayout();

    dumpScrollView->RecalculateContentSize();
    dumpScrollView->SetVerticalScrollPosition(0.f);

    if (snapshotID == ProfilerCPU::NO_SNAPSHOT_ID && started)
        profiler->Start();
}

void ProfilerTest::OnGlobalCPUProfilerOverlay(BaseObject* sender, void* data, void* callerData)
{
    ProfilerOverlay::globalProfilerOverlay->SetCPUProfiler(ProfilerCPU::globalProfiler, ProfilerCPUMarkerName::ENGINE_ON_FRAME);
    ProfilerOverlay::globalProfilerOverlay->SetEnabled(true);

    ProfilerOverlay::globalProfilerOverlay->ClearInterestMarkers();
    ProfilerOverlay::globalProfilerOverlay->AddInterestMarkers(defaultCPUMarkers);
}

void ProfilerTest::OnCustomCPUProfilerOverlay(BaseObject* sender, void* data, void* callerData)
{
    ProfilerOverlay::globalProfilerOverlay->SetCPUProfiler(customCPUProfiler, PROFILER_TEST_UPDATE_MARKER);
    ProfilerOverlay::globalProfilerOverlay->SetEnabled(true);

    ProfilerOverlay::globalProfilerOverlay->ClearInterestMarkers();
    ProfilerOverlay::globalProfilerOverlay->AddInterestMarker(FastName(PROFILER_TEST_UPDATE_MARKER));
    ProfilerOverlay::globalProfilerOverlay->AddInterestMarker(FastName(PROFILER_TEST_FUNC0_MARKER));
    ProfilerOverlay::globalProfilerOverlay->AddInterestMarker(FastName(PROFILER_TEST_FUNC1_MARKER));
    ProfilerOverlay::globalProfilerOverlay->AddInterestMarker(FastName(PROFILER_TEST_FUNC2_MARKER));
}

void ProfilerTest::OnCustomCPUProfiler(BaseObject* sender, void* data, void* callerData)
{
    if (customCPUProfiler->IsStarted())
        customCPUProfiler->Stop();
    else
        customCPUProfiler->Start();
}

void ProfilerTest::OnGlobalCPUProfiler(BaseObject* sender, void* data, void* callerData)
{
    if (ProfilerCPU::globalProfiler->IsStarted())
        ProfilerCPU::globalProfiler->Stop();
    else
        ProfilerCPU::globalProfiler->Start();
}

void ProfilerTest::OnGPUProfiler(BaseObject* sender, void* data, void* callerData)
{
    if (ProfilerGPU::globalProfiler->IsStarted())
        ProfilerGPU::globalProfiler->Stop();
    else
        ProfilerGPU::globalProfiler->Start();
}

void ProfilerTest::OnMakeSnapshot(BaseObject* sender, void* data, void* callerData)
{
    bool started = customCPUProfiler->IsStarted();
    if (started)
        customCPUProfiler->Stop();

    customCPUProfiler->DeleteSnapshots();
    snapshotID = customCPUProfiler->MakeSnapshot();

    if (started)
        customCPUProfiler->Start();
}

void ProfilerTest::OnDumpAverage(BaseObject* sender, void* data, void* callerData)
{
    DumpAverageToUI(reinterpret_cast<ProfilerCPU*>(data), ProfilerCPU::NO_SNAPSHOT_ID);
}

void ProfilerTest::OnDumpAverageSnapshot(BaseObject* sender, void* data, void* callerData)
{
    DumpAverageToUI(customCPUProfiler, snapshotID);
}

void ProfilerTest::OnDumpJSON(BaseObject* sender, void* data, void* callerData)
{
    FileSystem::Instance()->CreateDirectory(FilePath(PROFILER_TEST_DUMP_JSON_PATH).GetDirectory(), true);
    DAVA::File* json = DAVA::File::Create(PROFILER_TEST_DUMP_JSON_PATH, DAVA::File::CREATE | DAVA::File::WRITE);
    if (json)
    {
        bool cpuStarted = customCPUProfiler->IsStarted();
        if (cpuStarted)
            customCPUProfiler->Stop();

        Vector<TraceEvent> trace = customCPUProfiler->GetTrace();
        std::stringstream stream;
        TraceEvent::DumpJSON(trace, stream);
        json->WriteNonTerminatedString(stream.str());

        if (cpuStarted)
            customCPUProfiler->Start();
    }
    SafeRelease(json);
}

void ProfilerTest::OnDumpGlobalCPUGPU(BaseObject* sender, void* data, void* callerData)
{
    bool cpuStarted = ProfilerCPU::globalProfiler->IsStarted();
    if (cpuStarted)
        ProfilerCPU::globalProfiler->Stop();

    bool gpuStarted = ProfilerGPU::globalProfiler->IsStarted();
    if (gpuStarted)
        ProfilerGPU::globalProfiler->Stop();

    FileSystem::Instance()->CreateDirectory(FilePath(PROFILER_TEST_DUMP_JSON_PATH).GetDirectory(), true);
    ProfilerUtils::DumpCPUGPUTraceToFile(ProfilerCPU::globalProfiler, ProfilerGPU::globalProfiler, PROFILER_TEST_DUMP_JSON_PATH);

    if (cpuStarted)
        ProfilerCPU::globalProfiler->Start();

    if (gpuStarted)
        ProfilerGPU::globalProfiler->Start();
}
