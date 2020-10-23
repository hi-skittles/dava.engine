#pragma once

#include "Infrastructure/BaseScreen.h"

#include <UI/UIControl.h>
#include <UI/UIScrollView.h>
#include <UI/UIFileSystemDialog.h>
#include <FileSystem/FileWatcher.h>

class TestBed;
class FileWatcherTest : public BaseScreen, public DAVA::UIFileSystemDialogDelegate
{
public:
    FileWatcherTest(TestBed& app);

protected:
    void LoadResources() override;
    void UnloadResources() override;

    void OnFileSelected(DAVA::UIFileSystemDialog* forDialog, const DAVA::FilePath& pathToFile) override;
    void OnFileSytemDialogCanceled(DAVA::UIFileSystemDialog* forDialog) override;

    void OpenDlg();

private:
    void AppendLogText(const DAVA::String& s);
#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
    void OnFileEvent(const DAVA::String& path, DAVA::FileWatcher::eWatchEvent e);
#endif

private:
    DAVA::UIControl* pathField = nullptr;
    DAVA::UIControl* openDlgPath = nullptr;
    DAVA::UIControl* log = nullptr;
    DAVA::UIScrollView* scrollView = nullptr;
    DAVA::UIFileSystemDialog* dlg = nullptr;

    DAVA::String currentWatchPath;
#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
    DAVA::FileWatcher* fileWatcher = nullptr;
#endif
};
