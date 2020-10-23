#include "DlcTest.h"
#include "Base/GlobalEnum.h"
#include "Database/MongodbClient.h"
#include "Database/MongodbObject.h"
#include "Notification/LocalNotificationProgress.h"
#include "Notification/LocalNotificationText.h"
#include "UI/Focus/UIFocusComponent.h"
#include "Utils/StringUtils.h"
#include "UI/Update/UIUpdateComponent.h"
#include "UI/Render/UIDebugRenderComponent.h"

#if defined(__DAVAENGINE_ANDROID__)
#include "Platform/DeviceInfo.h"
#endif

//#define DLC_TEST
#define DLC_TEST_MAX_RETRY_COUNT 10
#define DLC_MONGO_HOST String("by2-badava-mac-11.corp.wargaming.local")
#define DLC_MONGO_PORT 27017
#define DLC_MONGO_TEST_DB "dlc"
#define DLC_MONGO_TEST_COLLECTION "test.exit"

namespace
{
#if defined(__DAVAENGINE_IPHONE__)
const DAVA::String localServerUrl = "http://by1-builddlc-01/DLC_Blitz";
#else
const DAVA::String localServerUrl = "http://by1-builddlc-01.corp.wargaming.local/DLC_Blitz";
#endif

const DAVA::String cdnServerUrl = "http://dl-wotblitz.wargaming.net/dlc/";

const float32 WIDTH = 500.f;
const float32 LEFT_COLUMN_X = 10.f;
const float32 SPACE = 5.f;
const float32 BUTTON_H = 40.f;
const float32 BUTTON_W = (WIDTH - SPACE) / 2;
const float32 HALF_BUTTON_W = (BUTTON_W - SPACE) / 2;

const float32 RIGHT_COLUMN_X = LEFT_COLUMN_X + (WIDTH - SPACE) / 2 + SPACE;

const float32 VERSION_LINE_Y = 100.f;
const float32 SERVER_Y = VERSION_LINE_Y + BUTTON_H + SPACE;
const float32 SPEED_THREAD_Y = SERVER_Y + BUTTON_H + SPACE;
const float32 INFO_Y = SPEED_THREAD_Y + BUTTON_H + SPACE;
const float32 START_CANCEL_Y = INFO_Y + BUTTON_H + 6 * SPACE;
}

DlcTest::DlcTest(TestBed& app)
    : BaseScreen(app, "DlcTest")
    , options(new KeyedArchive)
    , dlc(nullptr)
{
    GetOrCreateComponent<UIUpdateComponent>();
}

void DlcTest::LoadResources()
{
    BaseScreen::LoadResources();

    options->LoadFromYamlFile(optionsPath);

    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    float32 fontSize = 24.0f;
    float32 smallFontSize = 14.0f;

    angle = 0;
    lastUpdateTime = 0;

#ifdef __DAVAENGINE_ANDROID__
    List<DeviceInfo::StorageInfo> storageList = DeviceInfo::GetStoragesList();
    DeviceInfo::StorageInfo external;
    bool externalStorage(false);
    for (const auto& it : storageList)
    {
        if (DeviceInfo::STORAGE_TYPE_PRIMARY_EXTERNAL == it.type)
        {
            external = it;
            DVASSERT(false == externalStorage);
            externalStorage = true;
        }
    }
    workingDir = external.path + "DLC/";
    sourceDir = external.path + "DLC_Resources/";
    destinationDir = external.path + "DLC_Resources/";
#else
    workingDir = "~doc:/DLC/";
    sourceDir = "~doc:/Resources/";
    destinationDir = "~doc:/Resources/";
#endif

    infoText = new UIStaticText(Rect(LEFT_COLUMN_X, LEFT_COLUMN_X, WIDTH, BUTTON_H * 5));
    infoText->SetTextColor(Color::White);
    infoText->SetFont(font);
    infoText->SetFontSize(smallFontSize);
    infoText->SetMultiline(true);
    infoText->SetTextAlign(ALIGN_LEFT | ALIGN_TOP);
    AddControl(infoText);

    UIStaticText* ver = new UIStaticText(Rect(LEFT_COLUMN_X, VERSION_LINE_Y, BUTTON_W, BUTTON_H));
    ver->SetTextColor(Color::White);
    ver->SetFont(font);
    ver->SetFontSize(fontSize);
    ver->SetMultiline(false);
    ver->SetTextAlign(ALIGN_LEFT);
    ver->SetText(L"Game Version, GPU:");
    AddControl(ver);
    SafeRelease(ver);

    gameVersionIn = new UITextField(Rect(LEFT_COLUMN_X + BUTTON_W + SPACE, VERSION_LINE_Y, BUTTON_W, BUTTON_H));
    gameVersionIn->GetOrCreateComponent<UIDebugRenderComponent>();
    String gameVer = options->GetString(gameVersion, defaultGameVersion);
    gameVersionIn->SetText(UTF8Utils::EncodeToWideString(gameVer));
    gameVersionIn->GetOrCreateComponent<UIFocusComponent>();
    gameVersionIn->SetDelegate(this);
    gameVersionIn->SetFont(font);
    gameVersionIn->SetFontSize(fontSize);
    AddControl(gameVersionIn);

    gpuIn = new UITextField(Rect(LEFT_COLUMN_X + BUTTON_W * 2 + SPACE * 2, VERSION_LINE_Y, BUTTON_W, BUTTON_H));
    gpuIn->GetOrCreateComponent<UIDebugRenderComponent>();
    gpuIn->SetText(UTF8Utils::EncodeToWideString(GPUFamilyDescriptor::GetGPUName(DeviceInfo::GetGPUFamily())));
    gpuIn->GetOrCreateComponent<UIFocusComponent>();
    gpuIn->SetDelegate(this);
    gpuIn->SetFont(font);
    gpuIn->SetFontSize(fontSize);
    AddControl(gpuIn);

    //=========================

    UIButton* setDlInternalServerButton = new UIButton(Rect(LEFT_COLUMN_X, SERVER_Y, BUTTON_W, BUTTON_H));
    setDlInternalServerButton->SetStateFont(0xFF, font);
    setDlInternalServerButton->SetStateFontSize(0xFF, fontSize);
    setDlInternalServerButton->SetStateFontColor(0xFF, Color::White);
    setDlInternalServerButton->SetStateText(0xFF, L"Set internal server");
    setDlInternalServerButton->GetOrCreateComponent<UIDebugRenderComponent>();
    setDlInternalServerButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &DlcTest::SetInternalDlServer));
    AddControl(setDlInternalServerButton);
    SafeRelease(setDlInternalServerButton);

    UIButton* setDlexternalServerButton = new UIButton(Rect(RIGHT_COLUMN_X, SERVER_Y, BUTTON_W, BUTTON_H));
    setDlexternalServerButton->SetStateFont(0xFF, font);
    setDlexternalServerButton->SetStateFontSize(0xFF, fontSize);
    setDlexternalServerButton->SetStateFontColor(0xFF, Color::White);
    setDlexternalServerButton->SetStateText(0xFF, L"Set external server");
    setDlexternalServerButton->GetOrCreateComponent<UIDebugRenderComponent>();
    setDlexternalServerButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &DlcTest::SetExternalDlServer));
    AddControl(setDlexternalServerButton);
    SafeRelease(setDlexternalServerButton);

    //=========================

    UIButton* setDlSpeed = new UIButton(Rect(LEFT_COLUMN_X, SPEED_THREAD_Y, HALF_BUTTON_W, BUTTON_H));
    setDlSpeed->SetStateFont(0xFF, font);
    setDlSpeed->SetStateFontSize(0xFF, fontSize);
    setDlSpeed->SetStateFontColor(0xFF, Color::White);
    setDlSpeed->SetStateText(0xFF, L"Set spd");
    setDlSpeed->GetOrCreateComponent<UIDebugRenderComponent>();
    setDlSpeed->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &DlcTest::SetSpeed));
    AddControl(setDlSpeed);
    SafeRelease(setDlSpeed);

    dlSpeedIn = new UITextField(Rect(LEFT_COLUMN_X + HALF_BUTTON_W + SPACE, SPEED_THREAD_Y, HALF_BUTTON_W, BUTTON_H));
    dlSpeedIn->GetOrCreateComponent<UIDebugRenderComponent>();

    uint64 spd = options->GetUInt64(downloadSpeed, 0);
    String spdStr(Format("%lld", spd));
    dlSpeedIn->SetText(UTF8Utils::EncodeToWideString(spdStr));
    dlSpeedIn->GetOrCreateComponent<UIFocusComponent>();
    dlSpeedIn->SetDelegate(this);
    AddControl(dlSpeedIn);

    //=========================

    UIButton* decDlThreadsButton = new UIButton(Rect(RIGHT_COLUMN_X, SPEED_THREAD_Y, HALF_BUTTON_W, BUTTON_H));
    decDlThreadsButton->SetStateFont(0xFF, font);
    decDlThreadsButton->SetStateFontSize(0xFF, fontSize);
    decDlThreadsButton->SetStateFontColor(0xFF, Color::White);
    decDlThreadsButton->SetStateText(0xFF, L"-1 thr");
    decDlThreadsButton->GetOrCreateComponent<UIDebugRenderComponent>();
    decDlThreadsButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &DlcTest::DecDlThreads));
    AddControl(decDlThreadsButton);
    SafeRelease(decDlThreadsButton);

    UIButton* incDlThreadsButton = new UIButton(Rect(RIGHT_COLUMN_X + HALF_BUTTON_W + SPACE, SPEED_THREAD_Y, HALF_BUTTON_W, BUTTON_H));
    incDlThreadsButton->SetStateFont(0xFF, font);
    incDlThreadsButton->SetStateFontSize(0xFF, fontSize);
    incDlThreadsButton->SetStateFontColor(0xFF, Color::White);
    incDlThreadsButton->SetStateText(0xFF, L"+1 thr");
    incDlThreadsButton->GetOrCreateComponent<UIDebugRenderComponent>();
    incDlThreadsButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &DlcTest::IncDlThreads));
    AddControl(incDlThreadsButton);
    SafeRelease(incDlThreadsButton);

    //=========================

    staticText = new UIStaticText(Rect(LEFT_COLUMN_X, INFO_Y, WIDTH - 2 * BUTTON_H, BUTTON_H));
    staticText->SetFont(font);
    staticText->SetFontSize(smallFontSize);
    staticText->SetTextColor(Color::White);
    staticText->SetText(L"Press Start!");
    AddControl(staticText);

    progressControl = new UIControl(Rect(LEFT_COLUMN_X, INFO_Y + BUTTON_H, WIDTH - 2 * BUTTON_H, SPACE));
    progressControl->GetOrCreateComponent<UIDebugRenderComponent>();
    AddControl(progressControl);

    progressStatistics = new UIStaticText(Rect(LEFT_COLUMN_X, INFO_Y + BUTTON_H + SPACE + SPACE, WIDTH - SPACE - BUTTON_H, SPACE));
    progressStatistics->SetText(L"0 / 0");
    progressStatistics->SetFont(font);
    progressStatistics->SetFontSize(smallFontSize);
    progressStatistics->SetTextColor(Color::White);
    AddControl(progressStatistics);

    animControl = new UIControl(Rect(LEFT_COLUMN_X + WIDTH - SPACE - BUTTON_H, INFO_Y + BUTTON_H, BUTTON_H, BUTTON_H));
    animControl->GetOrCreateComponent<UIDebugRenderComponent>();
    animControl->SetPivotPoint(Vector2(BUTTON_H / 2, BUTTON_H / 2));
    AddControl(animControl);

    //=========================

    startButton = new UIButton(Rect(LEFT_COLUMN_X, START_CANCEL_Y, BUTTON_W, BUTTON_H));
    startButton->SetStateFont(0xFF, font);
    startButton->SetStateFontSize(0xFF, fontSize);
    startButton->SetStateFontColor(0xFF, Color::White);
    startButton->SetStateFontColor(UIButton::STATE_DISABLED, Color(0.5f, 0.5f, 0.5f, 0.5f));
    startButton->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    startButton->SetStateText(0xFF, L"Start");
    startButton->GetOrCreateComponent<UIDebugRenderComponent>();
    startButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &DlcTest::Start));
    AddControl(startButton);

    cancelButton = new UIButton(Rect(RIGHT_COLUMN_X, START_CANCEL_Y, BUTTON_W, BUTTON_H));
    cancelButton->SetStateFont(0xFF, font);
    cancelButton->SetStateFontSize(0xFF, fontSize);
    cancelButton->SetStateFontColor(0xFF, Color::White);
    cancelButton->SetStateFontColor(UIButton::STATE_DISABLED, Color(0.5f, 0.5f, 0.5f, 0.5f));
    cancelButton->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    cancelButton->SetStateText(0xFF, L"Cancel");
    cancelButton->GetOrCreateComponent<UIDebugRenderComponent>();
    cancelButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &DlcTest::Cancel));
    AddControl(cancelButton);

    clearButton = new UIButton(Rect(RIGHT_COLUMN_X, START_CANCEL_Y + BUTTON_H + SPACE, BUTTON_W, BUTTON_H));
    clearButton->SetStateFont(0xFF, font);
    clearButton->SetStateFontSize(0xFF, fontSize);
    clearButton->SetStateFontColor(0xFF, Color::White);
    clearButton->SetStateFontColor(UIButton::STATE_DISABLED, Color(0.5f, 0.5f, 0.5f, 0.5f));
    clearButton->SetStateFontColor(UIButton::STATE_PRESSED_INSIDE, Color(0.0f, 1.0f, 0.0f, 1.0f));
    clearButton->SetStateText(0xFF, L"Clear");
    clearButton->GetOrCreateComponent<UIDebugRenderComponent>();
    clearButton->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &DlcTest::Clear));
    AddControl(clearButton);

    DAVA::FilePath::AddResourcesFolder(destinationDir);

#ifdef DLC_TEST
    crashTest.Init(workingDir, destinationDir);
#endif

    lastDLCState = DLC::DS_DONE;

    SafeRelease(font);
}

void DlcTest::UpdateInfoStr()
{
    infoStr = L"DLCWorkingDir: ";
    infoStr += UTF8Utils::EncodeToWideString(workingDir.GetAbsolutePathname());
    infoStr += L"\nResourcesDir: ";
    infoStr += UTF8Utils::EncodeToWideString(destinationDir.GetAbsolutePathname());
    infoStr += L"\nURL: ";
    DAVA::String url = options->GetString(currentDownloadUrl, localServerUrl);
    infoStr += UTF8Utils::EncodeToWideString(url);
    infoStr += L"\nDownloading threads count: ";
    uint32 currentThreadsCount = options->GetUInt32(downloadThreadsCount, defaultdownloadTreadsCount);
    infoStr += UTF8Utils::EncodeToWideString(Format("%d", currentThreadsCount));
    infoStr += UTF8Utils::EncodeToWideString(Format("\nSpeedLimit %d", 0));

    infoStr += L"\nGPU: ";
    infoStr += UTF8Utils::EncodeToWideString(Format("%s", GPUFamilyDescriptor::GetGPUName(DeviceInfo::GetGPUFamily()).c_str()));

    if (nullptr != infoText)
    {
        infoText->SetText(infoStr);
    }
}

void DlcTest::UnloadResources()
{
    BaseScreen::UnloadResources();

    options->SaveToYamlFile(optionsPath);
    DownloadManager::Instance()->SetDownloadSpeedLimit(0);

    SafeRelease(dlSpeedIn);
    SafeRelease(gameVersionIn);
    SafeRelease(gpuIn);
    SafeRelease(infoText);
    SafeRelease(staticText);
    SafeRelease(progressStatistics);
    SafeRelease(animControl);

    if (nullptr != dlc)
    {
        dlc->Cancel();
        SafeDelete(dlc);
    }

    SafeRelease(startButton);
    SafeRelease(cancelButton);
    SafeRelease(clearButton);

    DeviceInfo::SetOverridenGPU(GPU_INVALID);
}

void DlcTest::OnActive()
{
}

void DlcTest::Update(float32 timeElapsed)
{
    BaseScreen::Update(timeElapsed);
    lastUpdateTime += timeElapsed;
    if (lastUpdateTime > 0.05f)
    {
        UpdateInfoStr();

        uint64 cur = 0;
        uint64 total = 0;

        if (nullptr != dlc)
        {
            dlc->GetProgress(cur, total);
        }

        DownloadStatistics stat = DownloadManager::Instance()->GetStatistics();
        String statText = Format("%lld kbytes / %lld kbytes    %lld kbytes/s", cur / 1024, total / 1024, stat.downloadSpeedBytesPerSec / 1024);
        progressStatistics->SetText(UTF8Utils::EncodeToWideString(statText));

        if (nullptr != dlc)
        {
            dlc->GetProgress(cur, total);

            if (dlc->GetState() == DLC::DS_DOWNLOADING)
            {
                DownloadStatistics stat = DownloadManager::Instance()->GetStatistics();
                String statText = Format("%lld(%lld) Kb, %lld Kb/s", cur / 1024, total / 1024, stat.downloadSpeedBytesPerSec / 1024);
                progressStatistics->SetText(UTF8Utils::EncodeToWideString(statText));
            }
            else
            {
                String statText = Format("%lld(%lld)", cur, total);
                progressStatistics->SetText(UTF8Utils::EncodeToWideString(statText));
            }
        }

        // update animation
        angle += 0.10f;
        lastUpdateTime = 0;

        animControl->SetAngle(angle);

        // update progress control
        Rect r = staticText->GetRect();
        if (r.dx > 0)
        {
            float32 w = 0;
            switch (lastDLCState)
            {
            case DLC::DS_READY:
            case DLC::DS_DOWNLOADING:
            case DLC::DS_PATCHING:
            {
                if (total > 0)
                {
                    w = cur * r.dx / total;
                    if (0 == w && r.dx > 0)
                    {
                        w = 1;
                    }
                }
                else
                {
                    w = r.dx;
                }
            }
            break;
            default:
                w = r.dx;
                break;
            }

            Rect pr = progressControl->GetRect();
            progressControl->SetRect(Rect(pr.x, pr.y, w, pr.dy));
        }
    }

    uint32 dlcState = lastDLCState;
    uint32 dlcError = DLC::DE_NO_ERROR;

    if (nullptr != dlc)
    {
        dlcState = dlc->GetState();
        dlcError = dlc->GetError();
    }

    if (lastDLCState != dlcState)
    {
        lastDLCState = dlcState;

        switch (lastDLCState)
        {
        case DLC::DS_CHECKING_INFO:
            staticText->SetText(L"Checking version...");
            break;
        case DLC::DS_CHECKING_PATCH:
            staticText->SetText(L"Checking patch...");
            break;
        case DLC::DS_CHECKING_META:
            staticText->SetText(L"Checking meta information...");
            break;
        case DLC::DS_READY:
            staticText->SetText(L"Ready for download.");
            break;
        case DLC::DS_DOWNLOADING:
            staticText->SetText(L"Downloading...");
            break;
        case DLC::DS_PATCHING:
            staticText->SetText(L"Patching...");
            break;
        case DLC::DS_CANCELLING:
            staticText->SetText(L"Canceling...");
            break;
        case DLC::DS_DONE:
            if (dlcError == DLC::DE_NO_ERROR)
            {
                staticText->SetText(L"Done, OK!");
            }
            else
            {
                DAVA::String errorText = DAVA::Format("Done, error %s!", GlobalEnumMap<DAVA::DLC::DLCError>::Instance()->ToString(dlc->GetError()));
                DAVA::WideString wErrorText;
                DAVA::UTF8Utils::EncodeToWideString(reinterpret_cast<const DAVA::uint8*>(errorText.c_str()), errorText.size(), wErrorText);
                staticText->SetText(wErrorText);
            }
            break;
        default:
            break;
        }
    }

    if (nullptr != startButton && nullptr != cancelButton)
    {
        bool isDone = dlcState == DLC::DS_DONE;

        startButton->SetDisabled(!isDone);
        startButton->SetInputEnabled(isDone);

        clearButton->SetDisabled(!isDone);
        clearButton->SetInputEnabled(isDone);

        cancelButton->SetDisabled(isDone);
        cancelButton->SetInputEnabled(!isDone);
    }

#ifdef DLC_TEST
    crashTest.Update(timeElapsed, dlc);
#endif
}

void DlcTest::Draw(const UIGeometricData& geometricData)
{
}

void DlcTest::TextFieldOnTextChanged(UITextField* textField, const WideString& newText, const WideString& oldText)
{
    if (gameVersionIn == textField)
    {
        options->SetString(gameVersion, UTF8Utils::EncodeToUTF8(newText));
    }

    if (dlSpeedIn == textField)
    {
        uint64 speedLimit = std::atoi(UTF8Utils::EncodeToUTF8(newText).c_str());
        options->SetUInt64(downloadSpeed, speedLimit);
    }
}

void DlcTest::SetExternalDlServer(BaseObject* obj, void* data, void* callerData)
{
    options->SetString(currentDownloadUrl, cdnServerUrl);
}

void DlcTest::SetInternalDlServer(BaseObject* obj, void* data, void* callerData)
{
    options->SetString(currentDownloadUrl, localServerUrl);
}

void DlcTest::SetSpeed(BaseObject* obj, void* data, void* callerData)
{
    uint64 currentDlSpeed = options->GetUInt64(downloadSpeed, 0);
    DownloadManager::Instance()->SetDownloadSpeedLimit(currentDlSpeed * 10);
}

void DlcTest::IncDlThreads(BaseObject* obj, void* data, void* callerData)
{
    uint32 currentThreadsCount = options->GetUInt32(downloadThreadsCount, defaultdownloadTreadsCount);
    DownloadManager::Instance()->SetPreferredDownloadThreadsCount(++currentThreadsCount);
    options->SetUInt32(downloadThreadsCount, currentThreadsCount);
}

void DlcTest::DecDlThreads(BaseObject* obj, void* data, void* callerData)
{
    uint32 currentThreadsCount = options->GetUInt32(downloadThreadsCount, defaultdownloadTreadsCount);

    --currentThreadsCount;
    if (0 == currentThreadsCount)
        currentThreadsCount = 1;

    DownloadManager::Instance()->SetPreferredDownloadThreadsCount(currentThreadsCount);

    options->SetUInt32(downloadThreadsCount, currentThreadsCount);
}

void DlcTest::Start(BaseObject* obj, void* data, void* callerData)
{
    options->SaveToYamlFile(optionsPath);

    if (nullptr != dlc)
    {
        dlc->Cancel();
        SafeDelete(dlc);
    }

    FileSystem::Instance()->CreateDirectory(workingDir);
    FileSystem::Instance()->CreateDirectory(destinationDir);

    String url = options->GetString(currentDownloadUrl, localServerUrl);
    String gameVersionToDl = options->GetString(gameVersion, defaultGameVersion);
    dlc = new DLC(url, sourceDir, destinationDir, workingDir, gameVersionToDl, destinationDir + "/version/resources.txt");

    lastDLCState = dlc->GetState();

    DeviceInfo::SetOverridenGPU(GPUFamilyDescriptor::GetGPUByName(UTF8Utils::EncodeToUTF8(gpuIn->GetText())));

    staticText->SetText(L"Starting DLC...");
    dlc->Start();
}

void DlcTest::Cancel(BaseObject* obj, void* data, void* callerData)
{
    staticText->SetText(L"Canceling DLC...");
    dlc->Cancel();
}

void DlcTest::Clear(BaseObject* obj, void* data, void* callerData)
{
    FileSystem::Instance()->DeleteDirectory(workingDir);
    FileSystem::Instance()->DeleteDirectory(sourceDir);
    FileSystem::Instance()->DeleteDirectory(destinationDir);
}

void DLCCrashTest::Init(const DAVA::FilePath& workingDir, const DAVA::FilePath& destinationDir)
{
    forceExit = false;
    inExitMode = true;

    DAVA::Random::Instance()->Seed();
    cancelTimeout = DAVA::Random::Instance()->Rand(25);
    exitTimeout = DAVA::Random::Instance()->Rand(25);
    retryCount = DLC_TEST_MAX_RETRY_COUNT;

    testingFileFlag = workingDir + "run.test";
    if (!testingFileFlag.Exists())
    {
        DAVA::File* f = DAVA::File::Create(testingFileFlag, DAVA::File::CREATE | DAVA::File::WRITE);
        f->Write(&retryCount);

        DAVA::FileSystem::Instance()->DeleteDirectoryFiles(destinationDir, true);
        exitThread = Thread::Create(Message(this, &DLCCrashTest::ExitThread));
        exitThread->Start();

        DAVA::MongodbClient* dbClient = DAVA::MongodbClient::Create(DLC_MONGO_HOST, DLC_MONGO_PORT);
        if (dbClient->IsConnected())
        {
            dbClient->SetDatabaseName(DLC_MONGO_TEST_DB);
            dbClient->SetCollectionName(DLC_MONGO_TEST_COLLECTION);

            DAVA::MongodbObject* dbObject = new DAVA::MongodbObject();
            dbObject->SetUniqueObjectName();
            dbObject->AddString("state", "started");
            dbObject->AddInt32("retries", retryCount - DLC_TEST_MAX_RETRY_COUNT);
            dbObject->AddInt32("error", 0);
            dbObject->Finish();

            dbObjectId = dbObject->GetObjectName();
            f->WriteString(dbObjectId);

            dbClient->SaveDBObject(dbObject);
            SafeRelease(dbObject);
        }

        f->Release();
    }
    else
    {
        DAVA::File* f = DAVA::File::Create(testingFileFlag, DAVA::File::OPEN | DAVA::File::READ);
        if (nullptr != f && f->Read(&retryCount) > 0)
        {
            f->ReadString(dbObjectId);
        }
        else
        {
            retryCount = 0;
        }

        f->Release();
        inExitMode = false;
    }
}

void DLCCrashTest::Update(float32 timeElapsed, DLC* dlc)
{
    static float32 runtimeElapsed = 0;
    runtimeElapsed += timeElapsed;

    if (inExitMode)
    {
        if (runtimeElapsed > cancelTimeout)
        {
            dlc->Cancel();
        }

        if (forceExit)
        {
            exit(1);
        }
    }

    if (dlc->GetState() == DLC::DS_DONE)
    {
        int ret = 0;
        if (dlc->GetError() != DLC::DE_NO_ERROR)
        {
            ret = 1;
        }

        if (!inExitMode)
        {
            // finished without errors or there is no retry counts
            if (0 == ret || 0 == retryCount)
            {
                DAVA::FileSystem::Instance()->DeleteFile(testingFileFlag);
            }

            // dlc finished ok, write info into db
            if (0 == ret)
            {
                DAVA::MongodbClient* dbClient = DAVA::MongodbClient::Create(DLC_MONGO_HOST, DLC_MONGO_PORT);
                if (dbClient->IsConnected())
                {
                    dbClient->SetDatabaseName(DLC_MONGO_TEST_DB);
                    dbClient->SetCollectionName(DLC_MONGO_TEST_COLLECTION);

                    DAVA::MongodbObject* dbObject = dbClient->FindObjectByKey(dbObjectId);
                    if (nullptr != dbObject)
                    {
                        DAVA::MongodbObject* newDbObject = new DAVA::MongodbObject();
                        newDbObject->SetObjectName(dbObject->GetObjectName());
                        newDbObject->AddString("state", "finished");
                        newDbObject->AddInt32("retries", retryCount - DLC_TEST_MAX_RETRY_COUNT);
                        newDbObject->AddInt32("error", dlc->GetError());
                        newDbObject->Finish();

                        dbClient->SaveDBObject(newDbObject, dbObject);
                        SafeRelease(dbObject);
                        SafeRelease(newDbObject);
                    }
                }
            }
        }

        exit(ret);
    }
}

void DLCCrashTest::ExitThread(BaseObject* caller, void* callerData, void* userData)
{
    DAVA::int64 start = DAVA::SystemTimer::GetMs();
    DAVA::int64 elapsed = 0;

    while (elapsed < exitTimeout)
    {
        elapsed = (DAVA::SystemTimer::GetMs() - start) / 1000;
        Thread::Sleep(100);
    }

    forceExit = true;
}
