#pragma once

#include "DAVAConfig.h"

#ifdef __DAVAENGINE_AUTOTESTING__

#include "Autotesting/AutotestingSystemLua.h"
#include "Base/Singleton.h"
#include "Functional/TrackedObject.h"
#include "Render/RHI/rhi_Public.h"
#include "Time/DateTime.h"
#include "UI/UIEvent.h"

namespace DAVA
{
class Image;
class AutotestingSystemLuaDelegate;
class AutotestingSystemLua;
class Texture;
class Window;
class AutotestingSystem : public Singleton<AutotestingSystem>
{
public:
    AutotestingSystem();
    ~AutotestingSystem();

    void OnAppStarted();
    void OnAppFinished();
    void OnTestSkipped();

    void Update(float32 timeElapsed);
    void Draw();

    void OnInit();
    inline bool IsInit()
    {
        return isInit;
    };

    void InitLua(AutotestingSystemLuaDelegate* _delegate);

    void RunTests();

    // Parameters from DB
    void FetchParametersFromDB();
    void FetchParametersFromIdYaml();
    void SetUpConnectionToDB();
    RefPtr<KeyedArchive> GetIdYamlOptions();

    void InitializeDevice();

    // Test organization
    void OnTestStart(const String& testName);
    void OnStepStart(const String& stepName);
    void OnStepFinished();
    void OnTestStarted();
    void OnError(const String& errorMessage = "");
    void ForceQuit(const String& logMessage = "");
    void OnTestsFinished();

    // helpers
    void OnInput(const UIEvent& input);

    inline Vector2 GetMousePosition()
    {
        return mouseMove.point;
    };
    bool FindTouch(int32 id, UIEvent& touch);
    bool IsTouchDown(int32 id);

    const String& GetScreenShotName();
    void MakeScreenShot();
    bool GetIsScreenShotSaving() const;
    void ClickSystemBack();
    void EmulatePressKey(DAVA::uint32 key);

    // DB Master-Helper relations

    String GetTestId()
    {
        return Format("Test%03d", testIndex);
    };
    String GetStepId()
    {
        return Format("Step%03d", stepIndex);
    };
    String GetLogId()
    {
        return Format("Message%03d", logIndex);
    };

    String GetCurrentTimeString();
    String GetCurrentTimeMsString();

    inline AutotestingSystemLua* GetLuaSystem()
    {
        return luaSystem;
    };

    bool ResolvePathToAutomation();
    FilePath GetPathTo(const String& path) const;

    // Returns String at 'lineNumber'.
    // If 'lineNumber' points to empy line next non-empty line is read and 'lineNumber' is adjusted.
    // If 'lineNumber' points beyond file scope empty line is returned and 'lineNumber' is set to '-1'
    String GetLuaString(int32& lineNumber) const;

    void OnRecordClickControl(UIControl*);
    void OnRecordDoubleClickControl(UIControl*);
    void OnRecordFastSelectControl(UIControl*);

    void OnRecordWaitControlBecomeVisible(UIControl*);
    void OnRecordWaitControlBecomeEnabled(UIControl*);
    void OnRecordWaitControlDissapeared(UIControl*);

    void OnRecordSetText(UIControl*, const String&);
    void OnRecordCheckText(UIControl*);

    void OnRecordIsVisible(UIControl*);
    void OnRecordIsDisabled(UIControl*);

    void StartRecording();
    void StopRecording();
    bool IsRecording() const
    {
        return isRecording;
    }

    void SetTestFinishedCallback(const Function<void()> callback)
    {
        testFinishedCallback = callback;
    }
    void SetTestErrorCallback(const Function<void(const String&)> callback)
    {
        testErrorCallback = callback;
    }

protected:
    void DrawTouches();
    void OnScreenShotInternal(Texture* texture);
    void OnWindowSizeChanged(Window*, Size2f windowSize, Size2f surfaceSize);

    void ResetScreenshotTexture(Size2i size);

    AutotestingSystemLua* luaSystem;
    //DB
    void ExitApp();

    //Recording
    String GetControlHierarchy(UIControl*) const;
    void WriteScriptLine(const String&);

private:
    bool isScreenShotSaving = false;
    FilePath pathToAutomation;

    Function<void()> testFinishedCallback;
    Function<void(const String&)> testErrorCallback;

public:
    static const String RecordScriptFileName;
    int64 startTime = 0;

    bool isInit;
    bool isRunning;
    bool needExitApp;
    float32 timeBeforeExit;

    String projectName;
    String groupName;
    String deviceName;
    String testsDate;
    String runId;
    int32 testIndex;
    int32 stepIndex;
    int32 logIndex;

    String testDescription;
    String testFileName;
    String testFilePath;

    String buildDate;
    String buildId;
    String branch;
    String framework;
    String branchRev;
    String frameworkRev;

    bool isDB;
    bool needClearGroupInDB;

    bool isMaster;
    int32 requestedHelpers;
    String masterId; // for communication
    String masterTask;
    int32 masterRunId;
    bool isRegistered;
    bool isWaiting;
    bool isInitMultiplayer;
    String multiplayerName;
    float32 waitTimeLeft;
    float32 waitCheckTimeLeft;

    Map<int32, UIEvent> touches;
    UIEvent mouseMove;

    String screenshotName;
    Texture* screenshotTexture = nullptr;
    rhi::HSyncObject screenshotSync;
    bool screenshotRequested = false;

    TrackedObject localTrackedObject;

    bool isRecording = false;
};

inline bool AutotestingSystem::GetIsScreenShotSaving() const
{
    return isScreenShotSaving;
}
};

#endif //__DAVAENGINE_AUTOTESTING__
