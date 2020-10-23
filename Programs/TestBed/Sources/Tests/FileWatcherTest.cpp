#include "Tests/FileWatcherTest.h"

#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <UI/UIControlSystem.h>
#include <UI/Layouts/UISizePolicyComponent.h>
#include <UI/Text/UITextComponent.h>
#include <UI/Render/UIDebugRenderComponent.h>
#include <Render/2D/FTFont.h>
#include <Render/2D/Font.h>

#include <Math/Color.h>

FileWatcherTest::FileWatcherTest(TestBed& app)
    : BaseScreen(app, "FileWatcherTest")
{
}

void FileWatcherTest::LoadResources()
{
#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
    using namespace DAVA;

    Size2i screenSize = GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize();
    RefPtr<Font> font(FTFont::Create("~res:/TestBed/Fonts/korinna.ttf"));

    auto SetText = [font](UIControl* c, const String& s) {
        UITextComponent* t = c->GetOrCreateComponent<UITextComponent>();
        t->SetFont(font);
        t->SetText(s);
        t->SetFontSize(20.0f);
        t->SetColorInheritType(UIControlBackground::eColorInheritType::COLOR_IGNORE_PARENT);
        t->SetColor(Color::Red);
        t->SetAlign(eAlign::ALIGN_LEFT);
    };

    pathField = new UIControl({ 4.f, 10.f, screenSize.dx - 44.f, 32.f });
    SetText(pathField, "Watch path: ");
    AddControl(pathField);

    openDlgPath = new UIControl({ screenSize.dx - 40.f, 10.f, 32.f, 32.f });
    openDlgPath->SetName(FastName("openPath"));
    openDlgPath->AddEvent(EVENT_TOUCH_UP_INSIDE, Message([](BaseObject*, void* x, void*) { reinterpret_cast<FileWatcherTest*>(x)->OpenDlg(); }, this));
    openDlgPath->GetOrCreateComponent<UIDebugRenderComponent>();
    SetText(openDlgPath, "...");
    openDlgPath->GetComponent<UITextComponent>()->SetAlign(eAlign::ALIGN_HCENTER);
    UIControlBackground* bg = openDlgPath->GetOrCreateComponent<UIControlBackground>();
    bg->SetDrawType(UIControlBackground::DRAW_FILL);
    AddControl(openDlgPath);

    dlg = new UIFileSystemDialog(FilePath("~res:/TestBed/Fonts/korinna.ttf"));
    dlg->SetOperationType(UIFileSystemDialog::OPERATION_CHOOSE_DIR);
    dlg->SetDelegate(this);
    dlg->SetTitle(L"Choose folder to watch");

    {
        scrollView = new UIScrollView();
        scrollView->SetRect({ 4.f, 44.f, screenSize.dx - 8.f, screenSize.dy - 100.f });
        scrollView->GetOrCreateComponent<UIDebugRenderComponent>();
        AddControl(scrollView);

        log = new UIControl();
        log->SetInputEnabled(false);
        UITextComponent* t = log->GetOrCreateComponent<UITextComponent>();
        t->SetMultiline(UITextComponent::MULTILINE_ENABLED);
        t->SetAlign(eAlign::ALIGN_LEFT | eAlign::ALIGN_TOP);
        SetText(log, "");
        UISizePolicyComponent* sizePolicy = log->GetOrCreateComponent<UISizePolicyComponent>();
        sizePolicy->SetVerticalPolicy(UISizePolicyComponent::eSizePolicy::PERCENT_OF_CONTENT);
        sizePolicy->SetVerticalValue(100.0f);
        sizePolicy->SetHorizontalPolicy(UISizePolicyComponent::eSizePolicy::PERCENT_OF_CONTENT);
        sizePolicy->SetHorizontalValue(100.0f);
        log->UpdateLayout();
        scrollView->AddControlToContainer(log);
        scrollView->RecalculateContentSize();
    }

    fileWatcher = new FileWatcher();
    fileWatcher->onWatchersChanged.Connect(this, &FileWatcherTest::OnFileEvent);
#endif

    BaseScreen::LoadResources();
}

void FileWatcherTest::UnloadResources()
{
#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
    delete fileWatcher;
    fileWatcher = nullptr;

    DAVA::SafeRelease(pathField);
    DAVA::SafeRelease(pathField);
    DAVA::SafeRelease(openDlgPath);
    DAVA::SafeRelease(log);
    DAVA::SafeRelease(scrollView);
#endif
    BaseScreen::UnloadResources();
}

void FileWatcherTest::OnFileSelected(DAVA::UIFileSystemDialog* forDialog, const DAVA::FilePath& pathToFile)
{
#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
    if (currentWatchPath.empty() == false)
    {
        fileWatcher->Remove(currentWatchPath);
    }
    currentWatchPath = pathToFile.GetAbsolutePathname();
    DAVA::UITextComponent* t = pathField->GetOrCreateComponent<DAVA::UITextComponent>();
    t->SetText("Watch path: " + currentWatchPath);

    if (currentWatchPath.empty() == false)
    {
        fileWatcher->Add(currentWatchPath, true);
    }
#endif
}

void FileWatcherTest::OnFileSytemDialogCanceled(DAVA::UIFileSystemDialog* forDialog)
{
}

void FileWatcherTest::OpenDlg()
{
#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
    dlg->Show(this);
#endif
}

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
void FileWatcherTest::OnFileEvent(const DAVA::String& path, DAVA::FileWatcher::eWatchEvent e)
{
    using namespace DAVA;
    UITextComponent* t = log->GetOrCreateComponent<UITextComponent>();
    String currentText = t->GetText();

    String eventName;
    switch (e)
    {
    case FileWatcher::FILE_CREATED:
        eventName = " : Created";
        break;
    case FileWatcher::FILE_REMOVED:
        eventName = " : Removed";
        break;
    case FileWatcher::FILE_MODIFIED:
        eventName = " : Modifyed";
        break;
    }
    currentText += path;
    currentText += eventName;
    currentText += "\n";
    t->SetText(currentText);
    log->UpdateLayout();
    scrollView->RecalculateContentSize();
}
#endif
