#pragma once

#include "Infrastructure/BaseScreen.h"
#include <FileSystem/FilePath.h>
#include <DLCManager/DLCManager.h>
#include <Debug/ProfilerCPU.h>

class TestBed;
class DLCManagerTest : public BaseScreen, DAVA::UITextFieldDelegate
{
public:
    DLCManagerTest(TestBed& app);
    ~DLCManagerTest();

private:
    void TextFieldOnTextChanged(DAVA::UITextField* textField, const DAVA::WideString& newText, const DAVA::WideString& /*oldText*/) override;
    struct GuiGroup;
    void UpdateDescription(GuiGroup&);

    void LoadResources() override;
    void UnloadResources() override;

    void Update(DAVA::float32 timeElapsed) override;
    void UpdateProgress(GuiGroup&, DAVA::float32 progress);

    void OnInitClicked(BaseObject* sender, void* data, void* callerData);
    void OnIOErrorClicked(BaseObject* sender, void* data, void* callerData);
    void OnDeleteClicked(BaseObject* sender, void* data, void* callerData);
    void OnListPacksClicked(BaseObject* sender, void* data, void* callerData);
    void OnOffRequestingClicked(BaseObject* sender, void* data, void* callerData);
    void OnLoadClicked(BaseObject* sender, void* data, void* callerData);
    void OnExitButton(BaseObject* obj, void* data, void* callerData) override;

    void OnRequestUpdated(const DAVA::DLCManager::IRequest& request);
    void OnNetworkReady1(bool isReady);
    void OnNetworkReady2(bool isReady);
    void OnInitializeFinished1(size_t numDownloaded, size_t numTotalFiles);
    void OnInitializeFinished2(size_t numDownloaded, size_t numTotalFiles);
    void OnErrorSignal1(DAVA::DLCManager::ErrorOrigin errType, DAVA::int32 errnoVal, const DAVA::String& msg);
    void OnErrorSignal2(DAVA::DLCManager::ErrorOrigin errType, DAVA::int32 errnoVal, const DAVA::String& msg);

    DAVA::Engine& engine;
    DAVA::ProfilerCPU profiler;

    DAVA::FilePath folderWithDownloadedPacks1;
    DAVA::FilePath folderWithDownloadedPacks2;
    // quick and dirty way to test download on all platforms
    DAVA::String urlToServerSuperpack1 = "http://by1-smartdlc-01/mali.dvpk";
    DAVA::String urlToServerSuperpack2 = "http://by1-smartdlc-01/mali.dvpk";

    struct GuiGroup
    {
        DAVA::UITextField* editPackName = nullptr;
        DAVA::UITextField* editUrl = nullptr;
        DAVA::UIControl* progressRed = nullptr;
        DAVA::UIControl* progressGreen = nullptr;
        DAVA::UIStaticText* textStatusOutput = nullptr;

        DAVA::UIButton* buttonInitDLC = nullptr;
        DAVA::UIButton* buttonLoadPack = nullptr;
        DAVA::UIButton* buttonRemovePack = nullptr;
        DAVA::UIButton* buttonPauseResume = nullptr;

        DAVA::DLCManager::Progress progress;
    } one, two;

    DAVA::DLCManager* secondDLC = nullptr;
};
