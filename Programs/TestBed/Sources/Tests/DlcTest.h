#pragma once

#include "DAVAEngine.h"
#include "DLC/DLC.h"
#include "Infrastructure/BaseScreen.h"

using namespace DAVA;

class TestBed;

struct DLCCrashTest
{
    int64 cancelTimeout;
    int64 exitTimeout;
    uint32 retryCount;

    DAVA::FilePath testingFileFlag;
    DAVA::String dbObjectId;

    bool forceExit;
    bool inExitMode;

    Thread* exitThread;

    void Init(const DAVA::FilePath& workingDir, const DAVA::FilePath& destinationDir);
    void Update(float32 timeElapsed, DLC* dlc);

    void ExitThread(BaseObject* caller, void* callerData, void* userData);
};

class DlcTest : public BaseScreen, public UITextFieldDelegate
{
public:
    DlcTest(TestBed& app);

protected:
    ~DlcTest() = default;

public:
    void LoadResources() override;
    void UnloadResources() override;
    void OnActive() override;

    void Update(float32 timeElapsed) override;
    void Draw(const UIGeometricData& geometricData) override;

    void TextFieldOnTextChanged(UITextField* textField, const WideString& newText, const WideString& oldText) override;

private:
    void UpdateInfoStr();
    void SetInternalDlServer(BaseObject* obj, void* data, void* callerData);
    void SetExternalDlServer(BaseObject* obj, void* data, void* callerData);
    void SetSpeed(BaseObject* obj, void* data, void* callerData);
    void IncDlThreads(BaseObject* obj, void* data, void* callerData);
    void DecDlThreads(BaseObject* obj, void* data, void* callerData);
    void Start(BaseObject* obj, void* data, void* callerData);
    void Cancel(BaseObject* obj, void* data, void* callerData);
    void Clear(BaseObject* obj, void* data, void* callerData);

protected:
    const FilePath optionsPath = "~doc:/dlc_options.yaml";
    ScopedPtr<KeyedArchive> options;

    const String gameVersion = "DlcGameVersion";
    const String defaultGameVersion = "dlcdevtest";
    const String currentDownloadUrl = "DlcServerUrl";
    const String downloadThreadsCount = "DlcThreadsCount";
    const uint32 defaultdownloadTreadsCount = 4;
    const String downloadSpeed = "DlcDownloadSpeed";

    DAVA::FilePath workingDir;
    DAVA::FilePath sourceDir;
    DAVA::FilePath destinationDir;

    UITextField* gameVersionIn = nullptr;
    UITextField* gpuIn = nullptr;
    UITextField* dlSpeedIn = nullptr;
    UIStaticText* infoText = nullptr;
    WideString infoStr;

    UIStaticText* staticText = nullptr;
    UIControl* animControl = nullptr;
    UIControl* progressControl = nullptr;
    UIStaticText* progressStatistics = nullptr;

    UIButton* startButton = nullptr;
    UIButton* cancelButton = nullptr;
    UIButton* clearButton = nullptr;

    float32 angle = 0;
    float32 lastUpdateTime = 0.f;
    uint32 lastDLCState = 0;

    DLC* dlc = nullptr;
    DLCCrashTest crashTest;
};
