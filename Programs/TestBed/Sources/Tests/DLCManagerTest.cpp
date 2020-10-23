#include "Tests/DLCManagerTest.h"
#include "Infrastructure/TestBed.h"

#include <Engine/Engine.h>
#include <FileSystem/DynamicMemoryFile.h>
#include <FileSystem/Private/CheckIOError.h>
#include <DLCManager/DLCManager.h>
#include <UI/Focus/UIFocusComponent.h>
#include <UI/Render/UIDebugRenderComponent.h>
#include <UI/Update/UIUpdateComponent.h>
#include <EmbeddedWebServer/EmbeddedWebServer.h>
#include <DLCManager/DLCDownloader.h>

#include <fstream>

DLCManagerTest::DLCManagerTest(TestBed& app)
    : BaseScreen(app, "DLCManagerTest")
    , engine(app.GetEngine())
{
    GetOrCreateComponent<DAVA::UIUpdateComponent>();
}

DLCManagerTest::~DLCManagerTest()
{
    DAVA::DLCManager& dm = *engine.GetContext()->dlcManager;

    dm.requestUpdated.DisconnectAll();
    dm.networkReady.DisconnectAll();
    dm.Deinitialize();
}

void DLCManagerTest::TextFieldOnTextChanged(DAVA::UITextField* textField, const DAVA::WideString& newText, const DAVA::WideString& /*oldText*/)
{
    if (one.editUrl == textField)
    {
        urlToServerSuperpack1 = DAVA::UTF8Utils::EncodeToUTF8(newText);
    }
    if (two.editUrl == textField)
    {
        urlToServerSuperpack2 = DAVA::UTF8Utils::EncodeToUTF8(newText);
    }
    UpdateDescription(one);
    UpdateDescription(two);
}

void DLCManagerTest::UpdateDescription(GuiGroup& group)
{
    using namespace DAVA;

    const FilePath publicDocsPath = GetEngineContext()->fileSystem->GetPublicDocumentsPath();
    folderWithDownloadedPacks1 = publicDocsPath + "DLCManagerTest/packs1/";
    folderWithDownloadedPacks2 = publicDocsPath + "DLCManagerTest/packs2/";

    const FilePath& dir = (&group == &one) ? folderWithDownloadedPacks1 : folderWithDownloadedPacks2;
    const String& url = (&group == &one) ? urlToServerSuperpack1 : urlToServerSuperpack2;

    const String packName = group.editPackName->GetUtf8Text();
    const String message = Format("status:\n\"%s\" - name of pack you want to download\n"
                                  "directory to download packs:\n%s\n"
                                  "Url to superpack.dvpk:\n%s\n",
                                  packName.c_str(),
                                  dir.GetAbsolutePathname().c_str(),
                                  url.c_str());

    group.textStatusOutput->SetUtf8Text(message);
}

void DLCManagerTest::LoadResources()
{
    using namespace DAVA;
    profiler.Start();
    BaseScreen::LoadResources();

    secondDLC = DLCManager::Create();

    const Color greyColor = Color(0.4f, 0.4f, 0.4f, 1.f);

    ScopedPtr<FTFont> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));

    auto InitGuiGroup = [this](const Color& greyColor, const ScopedPtr<FTFont>& font, const float32 dx, const float32 dy, GuiGroup& group)
    {
        auto editPackName = new UITextField(Rect(dx + 5, dy + 10, 500, 40));
        editPackName->SetFont(font);
        editPackName->SetFontSize(20.f);
        editPackName->SetUtf8Text("test_pack");
        editPackName->SetFontSize(14);
        editPackName->GetOrCreateComponent<UIDebugRenderComponent>();
        editPackName->SetTextColor(Color(0.0, 1.0, 0.0, 1.0));
        editPackName->SetInputEnabled(true);
        editPackName->GetOrCreateComponent<UIFocusComponent>();
        editPackName->SetDelegate(this);
        editPackName->SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);
        AddControl(editPackName);
        group.editPackName = editPackName;

        const String& url = &group == &one ? urlToServerSuperpack1 : urlToServerSuperpack2;

        auto editUrl = new UITextField(Rect(dx + 5, dy + 50, 500, 40));
        editUrl->SetFont(font);
        editUrl->SetFontSize(14);
        editUrl->SetText(UTF8Utils::EncodeToWideString(url));
        editUrl->GetOrCreateComponent<UIDebugRenderComponent>();
        editUrl->SetTextColor(Color(0.0, 1.0, 0.0, 1.0));
        editUrl->SetInputEnabled(true);
        editUrl->GetOrCreateComponent<UIFocusComponent>();
        editUrl->SetDelegate(this);
        editUrl->SetTextAlign(ALIGN_LEFT | ALIGN_VCENTER);
        AddControl(editUrl);
        group.editUrl = editUrl;

        auto progressRed = new UIControl(Rect(dx + 5, dy + 100, 500, 5));
        progressRed->GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(Color::Red);
        AddControl(progressRed);
        group.progressRed = progressRed;

        auto progressGreen = new UIControl(Rect(dx + 5, dy + 100, 0, 5));
        progressGreen->GetOrCreateComponent<UIDebugRenderComponent>()->SetDrawColor(Color::Green);
        AddControl(progressGreen);
        group.progressGreen = progressGreen;

        auto textStatusOutput = new UIStaticText(Rect(dx + 5, dy + 120, 500, 150));
        textStatusOutput->SetFont(font);
        textStatusOutput->SetFontSize(14.f);
        textStatusOutput->SetTextColor(Color::White);
        textStatusOutput->SetMultiline(true);
        textStatusOutput->SetUtf8Text("status output: ");
        textStatusOutput->GetOrCreateComponent<UIDebugRenderComponent>();
        textStatusOutput->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
        AddControl(textStatusOutput);
        group.textStatusOutput = textStatusOutput;

        auto buttonInitDLC = new UIButton(Rect(dx + 600, dy + 10, 100, 40));
        buttonInitDLC->GetOrCreateComponent<UIDebugRenderComponent>();
        buttonInitDLC->SetStateFont(0xFF, font);
        buttonInitDLC->SetStateFontSize(0xFF, 20.f);
        buttonInitDLC->SetStateFontColor(0xFF, Color::White);
        buttonInitDLC->SetStateText(0xFF, L"Init");
        buttonInitDLC->SetStateFontColor(STATE_PRESSED_INSIDE, Color::Green);
        buttonInitDLC->AddEvent(EVENT_TOUCH_DOWN, Message(this, &DLCManagerTest::OnInitClicked));
        AddControl(buttonInitDLC);
        group.buttonInitDLC = buttonInitDLC;

        auto buttonLoadPack = new UIButton(Rect(dx + 600, dy + 50, 100, 40));
        buttonLoadPack->GetOrCreateComponent<UIDebugRenderComponent>();
        buttonLoadPack->SetStateFont(0xFF, font);
        buttonLoadPack->SetStateFontSize(0xFF, 20.f);
        buttonLoadPack->SetStateFontColor(0xFF, Color::White);
        buttonLoadPack->SetStateFontColor(STATE_PRESSED_INSIDE, Color::Green);
        buttonLoadPack->SetStateFontColor(STATE_DISABLED, greyColor);
        buttonLoadPack->SetStateText(0xFF, L"Load");
        buttonLoadPack->AddEvent(EVENT_TOUCH_DOWN, Message(this, &DLCManagerTest::OnLoadClicked));
        buttonLoadPack->SetDisabled(true);
        AddControl(buttonLoadPack);
        group.buttonLoadPack = buttonLoadPack;

        auto buttonRemovePack = new UIButton(Rect(dx + 600, dy + 120, 100, 40));
        buttonRemovePack->GetOrCreateComponent<UIDebugRenderComponent>();
        buttonRemovePack->SetStateFont(0xFF, font);
        buttonRemovePack->SetStateFontSize(0xFF, 20.f);
        buttonRemovePack->SetStateFontColor(0xFF, Color::White);
        buttonRemovePack->SetStateFontColor(STATE_PRESSED_INSIDE, Color::Green);
        buttonRemovePack->SetStateText(0xFF, L"Delete");
        buttonRemovePack->SetStateFontColor(STATE_DISABLED, greyColor);
        buttonRemovePack->AddEvent(EVENT_TOUCH_DOWN, Message(this, &DLCManagerTest::OnDeleteClicked));
        buttonRemovePack->SetDisabled(true);
        AddControl(buttonRemovePack);
        group.buttonRemovePack = buttonRemovePack;

        auto buttonPauseResume = new UIButton(Rect(dx + 600, dy + 190, 100, 40));
        buttonPauseResume->GetOrCreateComponent<UIDebugRenderComponent>();
        buttonPauseResume->SetStateFont(0xFF, font);
        buttonPauseResume->SetStateFontSize(0xFF, 20.f);
        buttonPauseResume->SetStateFontColor(0xFF, Color::White);
        buttonPauseResume->SetStateFontColor(STATE_PRESSED_INSIDE, Color::Green);
        buttonPauseResume->SetStateText(0xFF, L"Pause");
        buttonPauseResume->SetStateFontColor(STATE_DISABLED, greyColor);
        buttonPauseResume->AddEvent(EVENT_TOUCH_DOWN, Message(this, &DLCManagerTest::OnOffRequestingClicked));
        buttonPauseResume->SetDisabled(true);
        AddControl(buttonPauseResume);
        group.buttonPauseResume = buttonPauseResume;
    };

    InitGuiGroup(greyColor, font, 0, 0, one);
    InitGuiGroup(greyColor, font, 0, 300, two);

    UpdateDescription(one);
    UpdateDescription(two);
}

void DLCManagerTest::UnloadResources()
{
    using namespace DAVA;
    profiler.Stop();

    auto ClearGuiGroup = [this](GuiGroup& group) {
        SafeRelease(group.buttonPauseResume);
        SafeRelease(group.buttonRemovePack);
        SafeRelease(group.editPackName);
        SafeRelease(group.buttonLoadPack);
        SafeRelease(group.textStatusOutput);
        SafeRelease(group.progressRed);
        SafeRelease(group.progressGreen);
        SafeRelease(group.editUrl);
        SafeRelease(group.buttonInitDLC);
    };

    ClearGuiGroup(one);
    ClearGuiGroup(two);

    BaseScreen::UnloadResources();

    DLCManager& dm = *engine.GetContext()->dlcManager;
    dm.Deinitialize();

    secondDLC->Deinitialize();
    DLCManager::Destroy(secondDLC);
    secondDLC = nullptr;
}

void DLCManagerTest::Update(DAVA::float32)
{
    using namespace DAVA;
    DLCManager::Progress progress_;
    DLCManager& dm = *engine.GetContext()->dlcManager;
    auto UpdateProgress = [this](DLCManager& dm, DLCManager::Progress progress_) {
        //DAVA_PROFILER_CPU_SCOPE_CUSTOM(__FUNCTION__, &profiler);
        DLCManager::Progress progress = dm.GetProgress();
        if (progress.isRequestingEnabled && progress.alreadyDownloaded != progress_.alreadyDownloaded)
        {
            //Logger::Info("progress: %d %d", static_cast<uint32>(progress.total), static_cast<uint32>(progress.alreadyDownloaded));
            progress_ = progress;
        }
    };

    UpdateProgress(dm, one.progress);
    UpdateProgress(*secondDLC, two.progress);
}

void DLCManagerTest::UpdateProgress(GuiGroup& group, DAVA::float32 progress)
{
    auto rect = group.progressRed->GetRect();
    rect.dx = rect.dx * progress;
    group.progressGreen->SetRect(rect);
}

void DLCManagerTest::OnRequestUpdated(const DAVA::DLCManager::IRequest& request)
{
    using namespace DAVA;

    DLCManager& dm = request.GetDLCManager();
    GuiGroup& group = (&dm == secondDLC) ? two : one;

    const String& packName = request.GetRequestedPackName();
    // change total download progress
    uint64 requestFileSize = request.GetSize();
    uint64 requestDownloadedSize = request.GetDownloadedSize();
    float32 requestProgress = requestFileSize > 0 ? static_cast<float32>(requestDownloadedSize) / requestFileSize : 1;

    std::stringstream ss;
    int32 dlcIndex = (&group == &one) ? 0 : 1;
    ss << "DLC(" << dlcIndex << ") ";
    ss << packName << ": " << (requestProgress * 100) << '%';

    auto p = dm.GetProgress();
    if (p.total > 0)
    {
        ss << "\ntotal:" << (100.0 * p.alreadyDownloaded) / p.total << '%';
    }

    std::string str = ss.str();

    group.textStatusOutput->SetUtf8Text(str);

    Logger::Info("%s", str.c_str());

    UpdateProgress(group, requestProgress);
}

void DLCManagerTest::OnNetworkReady1(bool isReady)
{
    using namespace DAVA;
    // To visualize on MacOS DownloadManager::Instance()->SetDownloadSpeedLimit(100000);
    // on MacOS slowly connect and then fast downloading
    std::stringstream ss;
    ss << "network ready = " << std::boolalpha << isReady;
    Logger::Info("%s", ss.str().c_str());

    one.textStatusOutput->SetUtf8Text("loading: " + ss.str());
}

void DLCManagerTest::OnNetworkReady2(bool isReady)
{
    using namespace DAVA;
    // To visualize on MacOS DownloadManager::Instance()->SetDownloadSpeedLimit(100000);
    // on MacOS slowly connect and then fast downloading
    std::stringstream ss;
    ss << "network ready = " << std::boolalpha << isReady;
    Logger::Info("%s", ss.str().c_str());

    two.textStatusOutput->SetUtf8Text("loading: " + ss.str());
}

static std::ostream& operator<<(std::ostream& stream, DAVA::DLCManager::InitStatus status)
{
    switch (status)
    {
    case DAVA::DLCManager::InitStatus::InProgress:
        stream << "not_finished";
        break;
    case DAVA::DLCManager::InitStatus::FinishedWithLocalMeta:
        stream << "using_local_meta";
        break;
    case DAVA::DLCManager::InitStatus::FinishedWithRemoteMeta:
        stream << "using_remote_meta";
        break;
    }
    return stream;
}

static std::ostream& operator<<(std::ostream& stream, DAVA::DLCManager::ErrorOrigin errOrigin)
{
    switch (errOrigin)
    {
    case DAVA::DLCManager::ErrorOrigin::FileIO:
        stream << "error_file_io";
        break;
    case DAVA::DLCManager::ErrorOrigin::InitTimeout:
        stream << "error_init_timeout";
        break;
    }
    return stream;
}

void DLCManagerTest::OnInitializeFinished1(size_t numDownloaded, size_t numTotalFiles)
{
    one.buttonRemovePack->SetDisabled(false);
    one.buttonLoadPack->SetDisabled(false);
    one.buttonPauseResume->SetDisabled(false);

    DAVA::DLCManager& dm = *engine.GetContext()->dlcManager;
    std::stringstream ss;
    ss << "initialize finished: num_downloaded = " << numDownloaded << " num_total = " << numTotalFiles
       << "\ninit_status: " << dm.GetInitStatus() << std::endl;
    one.textStatusOutput->SetUtf8Text(ss.str());
}

void DLCManagerTest::OnInitializeFinished2(size_t numDownloaded, size_t numTotalFiles)
{
    two.buttonRemovePack->SetDisabled(false);
    two.buttonLoadPack->SetDisabled(false);
    two.buttonPauseResume->SetDisabled(false);

    DAVA::DLCManager& dm = *secondDLC;
    std::stringstream ss;
    ss << "initialize finished: num_downloaded = " << numDownloaded << " num_total = " << numTotalFiles
       << "\ninit_status: " << dm.GetInitStatus() << std::endl;
    two.textStatusOutput->SetUtf8Text(ss.str());
}

void DLCManagerTest::OnErrorSignal1(DAVA::DLCManager::ErrorOrigin errType, DAVA::int32 errnoVal, const DAVA::String& msg)
{
    std::stringstream ss;
    ss << "sdl_1 on_error_signal: msg: " << errType << " " << msg << " signal: " << errnoVal << " (" << strerror(errnoVal) << ")\n";
    //DAVA::String prev = logPring->GetUtf8Text();
    //logPring->SetUtf8Text(prev + ss.str());
}

void DLCManagerTest::OnErrorSignal2(DAVA::DLCManager::ErrorOrigin errType, DAVA::int32 errnoVal, const DAVA::String& msg)
{
    std::stringstream ss;
    ss << "sdl_2 on_error_signal: msg: " << errType << " " << msg << " signal: " << errnoVal << " (" << strerror(errnoVal) << ")\n";
    //DAVA::String prev = logPring->GetUtf8Text();
    //logPring->SetUtf8Text(prev + ss.str());
}

void DLCManagerTest::OnInitClicked(DAVA::BaseObject* sender, void* data, void* callerData)
{
    using namespace DAVA;
    GuiGroup& group = sender == one.buttonInitDLC ? one : two;
    DLCManager& dm = (sender == one.buttonInitDLC) ? *engine.GetContext()->dlcManager : *secondDLC;

    dm.Deinitialize();

    group.textStatusOutput->SetUtf8Text("done: start init");

    dm.networkReady.DisconnectAll();
    dm.networkReady.Connect(this, (&dm == secondDLC) ? &DLCManagerTest::OnNetworkReady2 : &DLCManagerTest::OnNetworkReady1);
    dm.initializeFinished.Connect(this, (&dm == secondDLC) ? &DLCManagerTest::OnInitializeFinished2 : &DLCManagerTest::OnInitializeFinished1);
    dm.error.Connect(this, (&dm == secondDLC) ? &DLCManagerTest::OnErrorSignal2 : &DLCManagerTest::OnErrorSignal1);

    DLCManager::Hints hints;
    FilePath publicDocsPath = GetEngineContext()->fileSystem->GetPublicDocumentsPath();
    hints.logFilePath = publicDocsPath.GetStringValue() + "dlc_manager_testbed.log";

    dm.Initialize((&dm == secondDLC) ? folderWithDownloadedPacks2 : folderWithDownloadedPacks1,
                  (&dm == secondDLC) ? urlToServerSuperpack2 : urlToServerSuperpack1,
                  hints);

    dm.SetRequestingEnabled(true);

    group.textStatusOutput->SetUtf8Text("done: start initialize PackManager");
}

void DLCManagerTest::OnIOErrorClicked(BaseObject*, void*, void*)
{
    using namespace DAVA;
    DebugFS::IOErrorTypes ioErr;

    ioErr.moveFailed = true;
    ioErr.ioErrorCode = EACCES;

    GenerateIOErrorOnNextOperation(ioErr);
}

void DLCManagerTest::OnDeleteClicked(BaseObject* sender, void* data, void* callerData)
{
    using namespace DAVA;

    GuiGroup& group = sender == one.buttonRemovePack ? one : two;
    DLCManager& dm = (sender == one.buttonRemovePack) ? *engine.GetContext()->dlcManager : *secondDLC;

    UpdateProgress(group, 0.f);

    String packName = group.editPackName->GetUtf8Text();
    dm.RemovePack(packName);
    group.textStatusOutput->SetUtf8Text("done: remove dvpk:" + packName);
}

void DLCManagerTest::OnListPacksClicked(BaseObject* sender, void* data, void* callerData)
{
    using namespace DAVA;
}

void DLCManagerTest::OnOffRequestingClicked(DAVA::BaseObject* sender, void* data, void* callerData)
{
    using namespace DAVA;

    GuiGroup& group = (sender == one.buttonPauseResume) ? one : two;
    DLCManager& dm = (sender == one.buttonPauseResume) ? *engine.GetContext()->dlcManager : *secondDLC;

    if (dm.IsRequestingEnabled())
    {
        dm.SetRequestingEnabled(false);
        group.buttonPauseResume->SetStateText(0xFF, L"Resume");
    }
    else
    {
        dm.SetRequestingEnabled(true);
        group.buttonPauseResume->SetStateText(0xFF, L"Pause");
    }
}

void DLCManagerTest::OnLoadClicked(DAVA::BaseObject* sender, void* data, void* callerData)
{
    using namespace DAVA;

    GuiGroup& group = (sender == one.buttonLoadPack) ? one : two;
    DLCManager& dm = (sender == one.buttonLoadPack) ? *engine.GetContext()->dlcManager : *secondDLC;

    dm.requestUpdated.DisconnectAll();
    dm.requestUpdated.Connect(this, &DLCManagerTest::OnRequestUpdated);

    const String packName = group.editPackName->GetUtf8Text();

    try
    {
        if (dm.IsInitialized())
        {
            const DLCManager::IRequest* p = dm.RequestPack(packName);
            if (p != nullptr && p->IsDownloaded())
            {
                group.textStatusOutput->SetUtf8Text("already downloaded: " + packName);
                return;
            }
        }

        group.textStatusOutput->SetUtf8Text("loading: " + packName);
        dm.RequestPack(packName);
    }
    catch (Exception& ex)
    {
        group.textStatusOutput->SetUtf8Text(ex.what());
    }
}

void DLCManagerTest::OnExitButton(BaseObject* obj, void* data, void* callerData)
{
    using namespace DAVA;
    DLCManager& pm = *engine.GetContext()->dlcManager;
    pm.Deinitialize();

    secondDLC->Deinitialize();

    BaseScreen::OnExitButton(obj, data, callerData);
}
