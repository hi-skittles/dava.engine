#include "Autotesting/AutotestingSystem.h"

#ifdef __DAVAENGINE_AUTOTESTING__

#include "Autotesting/AutotestingDB.h"
#include "Autotesting/AutotestingSystemLua.h"
#include "Engine/Engine.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/KeyedArchive.h"
#include "Input/InputEvent.h"
#include "Input/InputSystem.h"
#include "Job/JobManager.h"
#include "Logger/Logger.h"
#include "Platform/DeviceInfo.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/Image/Image.h"
#include "Render/Texture.h"
#include "Time/DateTime.h"
#include "Time/SystemTimer.h"
#include "UI/Render/UIRenderSystem.h"
#include "UI/UIControlSystem.h"
#include "UI/UIScreen.h"
#include "UI/UIScreenManager.h"
#include "UI/UIStaticText.h"

namespace DAVA
{
const String AutotestingSystem::RecordScriptFileName("RecordedScript.lua");

AutotestingSystem::AutotestingSystem()
    : luaSystem(nullptr)
    , isInit(false)
    , isRunning(false)
    , needExitApp(false)
    , timeBeforeExit(0.0f)
    , projectName("")
    , groupName("default")
    , deviceName("not-initialized")
    , testsDate("not_found")
    , runId("not_found")
    , testIndex(0)
    , stepIndex(0)
    , logIndex(0)
    , testDescription("")
    , testFileName("")
    , testFilePath("")
    , buildDate("not_found")
    , buildId("zero-build")
    , branch("branch")
    , framework("framework")
    , branchRev("0")
    , frameworkRev("0")
    , isDB(true)
    , needClearGroupInDB(false)
    , isMaster(true)
    , requestedHelpers(0)
    , masterId("")
    , masterTask("")
    , masterRunId(0)
    , isRegistered(false)
    , isWaiting(false)
    , isInitMultiplayer(false)
    , multiplayerName("")
    , waitTimeLeft(0.0f)
    , waitCheckTimeLeft(0.0f)
{
    new AutotestingDB();

    //default behavior for autotests is to exit on test end/error
    SetTestFinishedCallback([this] { ExitApp(); });
    SetTestErrorCallback([this](const String& error) { ExitApp(); });
}

AutotestingSystem::~AutotestingSystem()
{
    SafeRelease(luaSystem);
    if (AutotestingDB::Instance())
        AutotestingDB::Instance()->Release();

    SafeRelease(screenshotTexture);
}

void AutotestingSystem::InitLua(AutotestingSystemLuaDelegate* _delegate)
{
    Logger::Info("AutotestingSystem::InitLua");
    DVASSERT(nullptr == luaSystem);
    luaSystem = new AutotestingSystemLua();
    luaSystem->SetDelegate(_delegate);
}

bool AutotestingSystem::ResolvePathToAutomation()
{
    Logger::Info("AutotestingSystem::ResolvePathToAutomation platform=%s", DeviceInfo::GetPlatformString().c_str());
    pathToAutomation = "~doc:/atpath.txt";
    if (FileSystem::Instance()->Exists(pathToAutomation))
    {
        ScopedPtr<File> file(File::Create(pathToAutomation, File::OPEN | File::READ));
        if (file)
        {
            pathToAutomation = file->ReadLine();
            if (FileSystem::Instance()->Exists(pathToAutomation))
            {
                Logger::Info("AutotestingSystem::ResolvePathToAutomation resolved path %s", pathToAutomation.GetAbsolutePathname().c_str());
                return true;
            }
        }
    }

    // Try to find automation data in Documents
    if (DeviceInfo::GetPlatform() == DeviceInfo::PLATFORM_ANDROID)
    {
        pathToAutomation = FileSystem::Instance()->GetPublicDocumentsPath().GetAbsolutePathname() + "/Autotesting/";
    }
    else
    {
        pathToAutomation = "~doc:/Autotesting/";
    }

    if (FileSystem::Instance()->Exists(pathToAutomation))
    {
        Logger::Info("AutotestingSystem::ResolvePathToAutomation resolved path in documents %s", pathToAutomation.GetAbsolutePathname().c_str());
        return true;
    }

    // If there are no automation data in documents, try to find it in Data
    pathToAutomation = "~res:/Autotesting/";
    if (FileSystem::Instance()->Exists(pathToAutomation))
    {
        Logger::Info("AutotestingSystem::ResolvePathToAutomation resolved in resources %s", pathToAutomation.GetAbsolutePathname().c_str());
        return true;
    }
    return false;
}

FilePath AutotestingSystem::GetPathTo(const String& path) const
{
    return pathToAutomation + path;
}

// This method is called on application started and it handle autotest initialisation
void AutotestingSystem::OnAppStarted()
{
    Logger::Info("AutotestingSystem::OnAppStarted");

    if (isInit)
    {
        Logger::Error("AutotestingSystem::OnAppStarted App already initialized.");
        return;
    }
    deviceName = AutotestingSystemLua::Instance()->GetDeviceName();
    FetchParametersFromIdYaml();

    if (isDB)
    {
        SetUpConnectionToDB();
        FetchParametersFromDB();
    }

    const String testFileLocation = Format("/Tests/%s/%s.lua", groupName.c_str(), testFileName.c_str());
    FilePath testFileStrPath = GetPathTo(testFileLocation);
    if (!FileSystem::Instance()->Exists(testFileStrPath))
    {
        Logger::Error("AutotestingSystemLua::OnAppStarted: couldn't open %s", testFileLocation.c_str());
        testFileStrPath = "";
    }

    AutotestingDB::Instance()->WriteLogHeader();
    AutotestingSystemLua::Instance()->InitFromFile(testFileStrPath);

    Token wndSizeChangedToken = GetPrimaryWindow()->sizeChanged.Connect(this, &AutotestingSystem::OnWindowSizeChanged);
    GetPrimaryWindow()->sizeChanged.Track(wndSizeChangedToken, &localTrackedObject);

    Size2i size = GetEngineContext()->uiControlSystem->vcs->GetPhysicalScreenSize();
    ResetScreenshotTexture(size);
}

void AutotestingSystem::OnAppFinished()
{
    Logger::Info("AutotestingSystem::OnAppFinished in");
    ExitApp();
    Logger::Info("AutotestingSystem::OnAppFinished out");
}

void AutotestingSystem::RunTests()
{
    if (!isInit || isRunning)
    {
        return;
    }
    isRunning = true;
    OnTestStarted();
}

void AutotestingSystem::OnInit()
{
    DVASSERT(!isInit);
    isInit = true;
}

// Get test parameters from id.yaml
void AutotestingSystem::FetchParametersFromIdYaml()
{
    Logger::Info("AutotestingSystem::FetchParametersFromIdYaml");
    RefPtr<KeyedArchive> option = GetIdYamlOptions();

    buildId = option->GetString("BuildId");
    buildDate = option->GetString("Date");
    branch = option->GetString("Branch");
    framework = option->GetString("Framework");
    branchRev = option->GetString("BranchRev");
    frameworkRev = option->GetString("FrameworkRev");

    // Check is build fol local debugging.  By default: use DB.
    if ("true" == option->GetString("LocalBuild", "false"))
    {
        groupName = option->GetString("Group", AutotestingDB::DB_ERROR_STR_VALUE);
        testFileName = option->GetString("Filename", AutotestingDB::DB_ERROR_STR_VALUE);
        isDB = false;
    }
}

RefPtr<KeyedArchive> AutotestingSystem::GetIdYamlOptions()
{
    FilePath idYamlStrPath = GetPathTo("/id.yaml");
    RefPtr<KeyedArchive> option(new KeyedArchive());
    if (!FileSystem::Instance()->Exists(idYamlStrPath) || !option->LoadFromYamlFile(idYamlStrPath))
    {
        ForceQuit("Couldn't open file " + idYamlStrPath.GetAbsolutePathname());
    }

    return option;
}

// Get test parameters from autotesting db
void AutotestingSystem::FetchParametersFromDB()
{
    Logger::Info("AutotestingSystem::FetchParametersFromDB");
    AutotestingDB* db = AutotestingDB::Instance();
    groupName = db->GetStringTestParameter(deviceName, "Group");
    if (groupName == AutotestingDB::DB_ERROR_STR_VALUE)
    {
        ForceQuit("Couldn't get 'Group' parameter from DB.");
    }
    testFileName = db->GetStringTestParameter(deviceName, "Filename");
    if (groupName == AutotestingDB::DB_ERROR_STR_VALUE)
    {
        ForceQuit("Couldn't get 'Filename' parameter from DB.");
    }
    runId = db->GetStringTestParameter(deviceName, "RunId");
    if (runId == AutotestingDB::DB_ERROR_STR_VALUE)
    {
        ForceQuit("Couldn't get 'RunId' parameter from DB.");
    }
    testIndex = db->GetIntTestParameter(deviceName, "TestIndex");
    if (testIndex == AutotestingDB::DB_ERROR_INT_VALUE)
    {
        ForceQuit("Couldn't get TestIndex parameter from DB.");
    }
}

// Read DB parameters from config file and set connection to it
void AutotestingSystem::SetUpConnectionToDB()
{
    FilePath dbConfigStrPath = GetPathTo("/dbConfig.yaml");
    KeyedArchive* option = new KeyedArchive();
    if (!FileSystem::Instance()->Exists(dbConfigStrPath) || !option->LoadFromYamlFile(dbConfigStrPath))
    {
        ForceQuit("Couldn't open file " + dbConfigStrPath.GetAbsolutePathname());
    }

    String dbName = option->GetString("name");
    String dbAddress = option->GetString("hostname");
    String collection = option->GetString("collection");
    int32 dbPort = option->GetInt32("port");
    Logger::Info("AutotestingSystem::SetUpConnectionToDB %s -> %s[%s:%d]", collection.c_str(), dbName.c_str(), dbAddress.c_str(), dbPort);

    if (!AutotestingDB::Instance()->ConnectToDB(collection, dbName, dbAddress, dbPort))
    {
        ForceQuit("Couldn't connect to Test DB");
    }

    SafeRelease(option);
}

// Multiplayer API
void AutotestingSystem::InitializeDevice()
{
    Logger::Info("AutotestingSystem::InitializeDevice");
    if (!isDB)
    {
        OnError("Couldn't use multiplayer test in local mode.");
    }
    isInitMultiplayer = true;
}

String AutotestingSystem::GetCurrentTimeString()
{
    DateTime time = DateTime::Now();
    return Format("%02d-%02d-%02d", time.GetHour(), time.GetMinute(), time.GetSecond());
}

void AutotestingSystem::OnTestStart(const String& testDescription)
{
    Logger::Info("AutotestingSystem::OnTestStart %s", testDescription.c_str());
    AutotestingDB::Instance()->Log("DEBUG", Format("OnTestStart %s", testDescription.c_str()));
    if (isDB)
        AutotestingDB::Instance()->SetTestStarted();
}

void AutotestingSystem::OnStepStart(const String& stepName)
{
    Logger::Info("AutotestingSystem::OnStepStart %s", stepName.c_str());

    OnStepFinished();

    AutotestingDB::Instance()->Log("INFO", stepName);
}

void AutotestingSystem::OnStepFinished()
{
    Logger::Info("AutotestingSystem::OnStepFinished");
}

void AutotestingSystem::Update(float32 timeElapsed)
{
    if (!isInit)
    {
        return;
    }

    if (screenshotRequested && screenshotSync.IsValid() && rhi::SyncObjectSignaled(screenshotSync))
    {
        screenshotRequested = false;

        screenshotTexture->Retain();
        Function<void()> fn = Bind(&AutotestingSystem::OnScreenShotInternal, this, screenshotTexture);
        GetEngineContext()->jobManager->CreateWorkerJob(fn);
        isScreenShotSaving = true;
    }

    if (needExitApp)
    {
        timeBeforeExit -= timeElapsed;
        if (timeBeforeExit <= 0.0f)
        {
            needExitApp = false;
            GetEngineContext()->jobManager->WaitWorkerJobs();
            Engine::Instance()->QuitAsync(0);
        }
        return;
    }

    if (isRunning)
    {
        luaSystem->Update(timeElapsed);
    }
}

void AutotestingSystem::Draw()
{
    if (!isInit)
    {
        return;
    }

    DrawTouches();

    if (screenshotRequested && !screenshotSync.IsValid())
    {
        UIControlSystem* controlSystem = GetEngineContext()->uiControlSystem;
        UIScreen* currentScreen = controlSystem->GetScreen();
        if (currentScreen)
        {
            screenshotSync = rhi::GetCurrentFrameSyncObject();

            const Size2i& pScreenSize = controlSystem->vcs->GetPhysicalScreenSize();

            RenderSystem2D::RenderTargetPassDescriptor desc;
            desc.colorAttachment = screenshotTexture->handle;
            desc.depthAttachment = screenshotTexture->handleDepthStencil;
            desc.format = PixelFormat::FORMAT_RGBA8888;
            desc.width = uint32(pScreenSize.dx);
            desc.height = uint32(pScreenSize.dy);
            desc.priority = PRIORITY_SCREENSHOT + PRIORITY_MAIN_2D;
            desc.clearTarget = controlSystem->GetRenderSystem()->GetUI3DViewCount() == 0;
            desc.clearColor = Color::Black;
            desc.transformVirtualToPhysical = true;

            RenderSystem2D::Instance()->BeginRenderTargetPass(desc);
            controlSystem->ForceDrawControl(currentScreen);
            DrawTouches();
            RenderSystem2D::Instance()->FillRect(Rect(0.0f, 0.0f, float32(pScreenSize.dx), float32(pScreenSize.dy)), Color::White, RenderSystem2D::DEFAULT_2D_FILL_ALPHA_MATERIAL);
            RenderSystem2D::Instance()->EndRenderTargetPass();
        }
        else
        {
            Logger::Error("AutotestingSystem::MakeScreenShot no current screen");
        }
    }
}

void AutotestingSystem::DrawTouches()
{
    if (!touches.empty())
    {
        for (Map<int32, UIEvent>::iterator it = touches.begin(); it != touches.end(); ++it)
        {
            Vector2 point = it->second.point;
            RenderSystem2D::Instance()->DrawCircle(point, 25.0f, Color::White);
        }
    }
    RenderSystem2D::Instance()->DrawCircle(GetMousePosition(), 15.0f, Color::White);
}

void AutotestingSystem::OnTestStarted()
{
    Logger::Info("AutotestingSystem::OnTestsStarted");
    startTime = SystemTimer::GetFrameTimestampMs();
    luaSystem->StartTest();
}

void AutotestingSystem::OnError(const String& errorMessage)
{
    Logger::Error("AutotestingSystem::OnError %s", errorMessage.c_str());

    AutotestingDB::Instance()->Log("ERROR", errorMessage);

    MakeScreenShot();

    AutotestingDB::Instance()->Log("ERROR", screenshotName);

    if (isDB && isInitMultiplayer)
    {
        AutotestingDB::Instance()->WriteState(deviceName, "State", "error");
    }

    testErrorCallback(errorMessage);
}

void AutotestingSystem::ForceQuit(const String& errorMessage)
{
    DVASSERT(false, errorMessage.c_str());
    Engine::Instance()->QuitAsync(0);
}

void AutotestingSystem::MakeScreenShot()
{
    Logger::Info("AutotestingSystem::MakeScreenShot");
    String currentDateTime = GetCurrentTimeString();
    screenshotName = Format("%s_%s_%s_%d_%s", groupName.c_str(), testFileName.c_str(), runId.c_str(), testIndex, currentDateTime.c_str());
    String log = Format("AutotestingSystem::ScreenShotName %s", screenshotName.c_str());
    AutotestingDB::Instance()->Log("INFO", log.c_str());

    screenshotRequested = true;
    screenshotSync = rhi::HSyncObject();
}

const String& AutotestingSystem::GetScreenShotName()
{
    Logger::Info("AutotestingSystem::GetScreenShotName %s", screenshotName.c_str());
    return screenshotName;
}

void AutotestingSystem::OnScreenShotInternal(Texture* texture)
{
    DVASSERT(texture);

    Logger::Info("AutotestingSystem::OnScreenShot %s", screenshotName.c_str());
    int64 startTime = SystemTimer::GetMs();

    DAVA::ScopedPtr<DAVA::Image> image(texture->CreateImageFromMemory());
    const Size2i& size = GetEngineContext()->uiControlSystem->vcs->GetPhysicalScreenSize();
    image->ResizeCanvas(uint32(size.dx), uint32(size.dy));
    image->Save(FilePath(AutotestingDB::Instance()->logsFolder + Format("/%s.png", screenshotName.c_str())));

    int64 finishTime = SystemTimer::GetMs();
    Logger::FrameworkDebug("AutotestingSystem::OnScreenShot Upload: %lld", finishTime - startTime);
    isScreenShotSaving = false;

    SafeRelease(texture);
}

void AutotestingSystem::OnWindowSizeChanged(DAVA::Window*, Size2f windowSize, Size2f surfaceSize)
{
    Size2i size;
    size.dx = static_cast<int>(surfaceSize.dx);
    size.dy = static_cast<int>(surfaceSize.dy);
    ResetScreenshotTexture(size);
}

void AutotestingSystem::ResetScreenshotTexture(Size2i size)
{
    SafeRelease(screenshotTexture);

    Texture::FBODescriptor desc;
    desc.width = uint32(size.dx);
    desc.height = uint32(size.dy);
    desc.format = FORMAT_RGBA8888;
    desc.needDepth = true;
    desc.needPixelReadback = true;

    screenshotTexture = Texture::CreateFBO(desc);
}

void AutotestingSystem::ClickSystemBack()
{
    Window* primaryWindow = GetPrimaryWindow();
    primaryWindow->backNavigation.Emit(primaryWindow);

    UIEvent keyEvent;
    keyEvent.device = eInputDevices::KEYBOARD;
    keyEvent.phase = DAVA::UIEvent::Phase::KEY_DOWN;
    keyEvent.key = DAVA::eInputElements::BACK;
    keyEvent.timestamp = SystemTimer::GetMs() / 1000.0;
    GetEngineContext()->uiControlSystem->OnInput(&keyEvent);
}

void AutotestingSystem::EmulatePressKey(DAVA::uint32 key)
{
    InputEvent inputEvent;
    inputEvent.window = GetPrimaryWindow();
    inputEvent.timestamp = SystemTimer::GetMs() / 1000.0;
    inputEvent.deviceType = eInputDeviceTypes::KEYBOARD;
    inputEvent.digitalState = DigitalElementState::JustPressed();
    inputEvent.elementId = (eInputElements)key;
    inputEvent.keyboardEvent.charCode = 0;

    GetEngineContext()->inputSystem->DispatchInputEvent(inputEvent);
}

void AutotestingSystem::OnTestsFinished()
{
    Logger::Info("AutotestingSystem::OnTestsFinished");

    // Mark last step as SUCCESS
    OnStepFinished();

    if (isDB && isInitMultiplayer)
    {
        AutotestingDB::Instance()->WriteState(deviceName, "State", "finished");
    }

    // Mark test as SUCCESS
    AutotestingDB::Instance()->Log("INFO", "Test finished.");

    testFinishedCallback();
}

void AutotestingSystem::OnTestSkipped()
{
    Logger::Info("AutotestingSystem::OnTestSkipped");

    if (isDB && isInitMultiplayer)
    {
        AutotestingDB::Instance()->WriteState(deviceName, "State", "skipped");
    }

    // Mark test as SKIPPED
    AutotestingDB::Instance()->Log("INFO", "Test skipped.");

    ExitApp();
}

void AutotestingSystem::OnInput(const UIEvent& input)
{
    UIScreenManager* uiScreenManager = GetEngineContext()->uiScreenManager;
    if (uiScreenManager)
    {
        String screenName = (uiScreenManager->GetScreen()) ? uiScreenManager->GetScreen()->GetName().c_str() : "noname";
        Logger::Info("AutotestingSystem::OnInput screen is %s (%d)", screenName.c_str(), uiScreenManager->GetScreenId());
    }

    int32 id = input.touchId;
    switch (input.phase)
    {
    case UIEvent::Phase::BEGAN:
    {
        mouseMove = input;
        if (!IsTouchDown(id))
        {
            touches[id] = input;
        }
        else
        {
            Logger::Error("AutotestingSystemYaml::OnInput PHASE_BEGAN duplicate touch id=%d", id);
        }
    }
    break;
#if !defined(__DAVAENGINE_IPHONE__) && !defined(__DAVAENGINE_ANDROID__)
    case UIEvent::Phase::MOVE:
    {
        mouseMove = input;
        if (IsTouchDown(id))
        {
            Logger::Error("AutotestingSystemYaml::OnInput PHASE_MOVE id=%d must be PHASE_DRAG", id);
        }
    }
    break;
#endif
    case UIEvent::Phase::DRAG:
    {
        mouseMove = input;
        Map<int32, UIEvent>::iterator findIt = touches.find(id);
        if (findIt != touches.end())
        {
            findIt->second = input;
        }
        else
        {
            Logger::Error("AutotestingSystemYaml::OnInput PHASE_DRAG id=%d must be PHASE_MOVE", id);
        }
    }
    break;
    case UIEvent::Phase::ENDED:
    {
        mouseMove = input;
        Map<int32, UIEvent>::iterator findIt = touches.find(id);
        if (findIt != touches.end())
        {
            touches.erase(findIt);
        }
        else
        {
            Logger::Error("AutotestingSystemYaml::OnInput PHASE_ENDED id=%d not found", id);
        }
    }
    break;
    default:
        //TODO: keyboard input
        break;
    }
}

bool AutotestingSystem::FindTouch(int32 id, UIEvent& touch)
{
    bool isFound = false;
    Map<int32, UIEvent>::iterator findIt = touches.find(id);
    if (findIt != touches.end())
    {
        isFound = true;
        touch = findIt->second;
    }
    return isFound;
}

bool AutotestingSystem::IsTouchDown(int32 id)
{
    return (touches.find(id) != touches.end());
}

void AutotestingSystem::ExitApp()
{
    if (needExitApp)
    {
        return;
    }
    isRunning = false;
    isWaiting = false;
    needExitApp = true;
    timeBeforeExit = 1.0f;
}

void AutotestingSystem::OnRecordClickControl(UIControl* control)
{
    if (isRecording)
    {
        if (!control->GetParent()->GetName().IsValid()) //this criteria is so unreliable..
        {
            AutotestingSystem::Instance()->OnRecordFastSelectControl(control);
        }
        else
        {
            String hierarchy = GetControlHierarchy(control);
            if (hierarchy.find("DebugPopup") == String::npos)
            {
                String codeLine = Format("ClickControl('%s')", hierarchy.c_str());
                WriteScriptLine(codeLine);
            }
        }
    }
}

void AutotestingSystem::OnRecordDoubleClickControl(UIControl* control)
{
    String hierarchy = GetControlHierarchy(control);
    String codeLine = Format("DoubleClick('%s')", hierarchy.c_str());
    WriteScriptLine(codeLine);
}

void AutotestingSystem::OnRecordSetText(UIControl* control, const String& text)
{
    String hierarchy = GetControlHierarchy(control);
    String codeLine = Format("SetText('%s', '%s')", hierarchy.c_str(), text.c_str());
    WriteScriptLine(codeLine);
}

void AutotestingSystem::OnRecordCheckText(UIControl* control)
{
    String hierarchy = GetControlHierarchy(control);
    String text = DynamicTypeCheck<UIStaticText*>(control)->GetUtf8Text();
    String codeLine = Format("CheckText('%s', '%s')", hierarchy.c_str(), text.c_str());
    WriteScriptLine(codeLine);
}

void AutotestingSystem::OnRecordFastSelectControl(UIControl* control)
{
    String codeLine = Format("FastSelectControl('%s')", control->GetName().c_str());
    WriteScriptLine(codeLine);
}

void AutotestingSystem::OnRecordWaitControlBecomeVisible(UIControl* control)
{
    String hierarchy = GetControlHierarchy(control);
    String codeLine = Format("WaitControlBecomeVisible('%s')", hierarchy.c_str());
    WriteScriptLine(codeLine);
}
void AutotestingSystem::OnRecordWaitControlBecomeEnabled(UIControl* control)
{
    String hierarchy = GetControlHierarchy(control);
    String codeLine = Format("WaitControlBecomeEnabled('%s')", hierarchy.c_str());
    WriteScriptLine(codeLine);
}
void AutotestingSystem::OnRecordWaitControlDissapeared(UIControl* control)
{
    String hierarchy = GetControlHierarchy(control);
    String codeLine = Format("WaitControlDisappeared('%s')", hierarchy.c_str());
    WriteScriptLine(codeLine);
}

void AutotestingSystem::OnRecordIsVisible(UIControl* control)
{
    String hierarchy = GetControlHierarchy(control);
    String codeLine = Format("IsVisible('%s')", hierarchy.c_str());
    WriteScriptLine(codeLine);
}
void AutotestingSystem::OnRecordIsDisabled(UIControl* control)
{
    String hierarchy = GetControlHierarchy(control);
    String codeLine = Format("IsDisabled('%s')", hierarchy.c_str());
    WriteScriptLine(codeLine);
}

String AutotestingSystem::GetControlHierarchy(UIControl* control) const
{
    UIControl* iter = control->GetParent();
    String hierarhy;
    while (iter)
    {
        hierarhy = Format("%s/%s", iter->GetName().c_str(), hierarhy.c_str());
        iter = iter->GetParent();
    }
    FilePath scriptPath = pathToAutomation + RecordScriptFileName;
    hierarhy = Format("%s%s", hierarhy.c_str(), control->GetName().c_str());
    return hierarhy;
}

void AutotestingSystem::WriteScriptLine(const String& textLine)
{
    if (!isRecording)
    {
        return;
    }
    FilePath scriptPath = pathToAutomation + RecordScriptFileName;
    ScopedPtr<File> recordedActs(nullptr);
    if (FileSystem::Instance()->Exists(scriptPath))
    {
        recordedActs.reset(File::Create(scriptPath, File::APPEND | File::WRITE));
    }
    else
    {
        recordedActs.reset(File::Create(scriptPath, File::CREATE | File::WRITE));
    }
    if (recordedActs)
    {
        recordedActs->WriteLine(textLine);
    }
}

String AutotestingSystem::GetLuaString(int32& lineNumber) const
{
    String result;

    FilePath scriptPath = pathToAutomation + RecordScriptFileName;
    ScopedPtr<File> file(File::Create(scriptPath, File::OPEN | File::READ));

    if (file)
    {
        for (int32 i = 0; i <= lineNumber; i++)
        {
            if (!file->IsEof())
            {
                result = file->ReadLine();
                if (i == lineNumber && result.empty())
                {
                    lineNumber++; //skip empty lines
                }
            }
            else
            {
                lineNumber = -1;
                result = "";
            }
        }
    }
    else
    {
        lineNumber = -1;
    }

    return result;
}

void AutotestingSystem::StartRecording()
{
    DVASSERT(!isRecording);
    isRecording = true;
}

void AutotestingSystem::StopRecording()
{
    DVASSERT(isRecording);
    isRecording = false;
}

// Multiplayer API

// Working with DB api
};

#endif //__DAVAENGINE_AUTOTESTING__
