#include "Base/Platform.h"
#include "Base/BaseTypes.h"
#include "Concurrency/Semaphore.h"
#include "Concurrency/Thread.h"
#include "Concurrency/ThreadLocalPtr.h"
#include "Engine/Engine.h"
#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/Win32/WindowImplWin32.h"
#include "Debug/DVAssert.h"
#include "Utils/UTF8Utils.h"

#if defined(__DAVAENGINE_QT__)
#include "Engine/PlatformApiQt.h"
#endif

namespace DAVA
{
namespace Debug
{
namespace MessageBoxDetail
{
class MessageBoxHook
{
public:
    MessageBoxHook();
    ~MessageBoxHook();

    int Show(HWND hwndParent, WideString caption, WideString message, Vector<WideString> buttonNames, int defaultButton);

private:
    struct DialogButtonInfo
    {
        int id;
        HWND hwnd;
    };
    struct IdToIndexMap
    {
        int id;
        int index;
    };

    void PrepareButtons(HWND hwnd);

    static LRESULT CALLBACK HookInstaller(int code, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK HookWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    static BOOL CALLBACK EnumChildProcStart(HWND hwnd, LPARAM lparam);

    HHOOK hhook = nullptr;
    WNDPROC oldWndProc = nullptr;

    Vector<WideString> buttons;
    size_t buttonCount = 0;

    static const int buttonTypes[3];
    static const int defaultButtons[3];
    static const IdToIndexMap map[6];
    static ThreadLocalPtr<MessageBoxHook> thisPointerPerThread;
};

ThreadLocalPtr<MessageBoxHook> MessageBoxHook::thisPointerPerThread([](MessageBoxHook*) {});

const int MessageBoxHook::buttonTypes[3] = {
    MB_OK,
    MB_YESNO,
    MB_ABORTRETRYIGNORE,
};

const int MessageBoxHook::defaultButtons[3] = {
    MB_DEFBUTTON1,
    MB_DEFBUTTON2,
    MB_DEFBUTTON3,
};

const MessageBoxHook::IdToIndexMap MessageBoxHook::map[6] = {
    { IDABORT, 0 },
    { IDRETRY, 1 },
    { IDIGNORE, 2 },
    { IDYES, 0 },
    { IDNO, 1 },
    { IDCANCEL, 0 },
};

MessageBoxHook::MessageBoxHook()
{
    thisPointerPerThread.Reset(this);
}

MessageBoxHook::~MessageBoxHook()
{
    thisPointerPerThread.Reset();
}

int MessageBoxHook::Show(HWND hwndParent, WideString caption, WideString message, Vector<WideString> buttonNames, int defaultButton)
{
    buttons = std::move(buttonNames);
    buttonCount = std::min<size_t>(3, buttons.size());
    defaultButton = std::max(0, std::min(2, defaultButton));

    // Install hook procedure to replace buttons names for standrd MessageBox function
    hhook = ::SetWindowsHookExW(WH_CALLWNDPROC, &MessageBoxHook::HookInstaller, nullptr, ::GetCurrentThreadId());
    const UINT style = MB_SYSTEMMODAL | MB_ICONEXCLAMATION | defaultButtons[defaultButton] | buttonTypes[buttonCount - 1];
    int choice = ::MessageBoxW(hwndParent, message.c_str(), caption.c_str(), style);
    ::UnhookWindowsHookEx(hhook);

    for (const IdToIndexMap& i : map)
    {
        if (choice == i.id)
            return i.index;
    }
    return 0;
}

void MessageBoxHook::PrepareButtons(HWND hwnd)
{
    Vector<DialogButtonInfo> dialogButtons;
    ::EnumChildWindows(hwnd, &MessageBoxHook::EnumChildProcStart, reinterpret_cast<LPARAM>(&dialogButtons));

    size_t n = std::min(dialogButtons.size(), buttonCount);
    for (size_t i = 0; i < n; ++i)
    {
        ::SetWindowTextW(dialogButtons[i].hwnd, buttons[i].c_str());
    }
}

LRESULT CALLBACK MessageBoxHook::HookInstaller(int code, WPARAM wparam, LPARAM lparam)
{
    MessageBoxHook* pthis = thisPointerPerThread.Get();
    if (code == HC_ACTION)
    {
        CWPSTRUCT* cwp = reinterpret_cast<CWPSTRUCT*>(lparam);
        if (cwp->message == WM_INITDIALOG)
        {
            LONG_PTR newWndProc = reinterpret_cast<LONG_PTR>(&MessageBoxHook::HookWndProc);
            pthis->oldWndProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(cwp->hwnd, GWLP_WNDPROC, newWndProc));
        }
    }
    return ::CallNextHookEx(pthis->hhook, code, wparam, lparam);
}

LRESULT CALLBACK MessageBoxHook::HookWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    MessageBoxHook* pthis = thisPointerPerThread.Get();
    LRESULT lresult = ::CallWindowProcW(pthis->oldWndProc, hwnd, message, wparam, lparam);
    if (message == WM_INITDIALOG)
    {
        pthis->PrepareButtons(hwnd);
    }
    return lresult;
}

BOOL CALLBACK MessageBoxHook::EnumChildProcStart(HWND hwnd, LPARAM lparam)
{
    Vector<DialogButtonInfo>& buttons = *reinterpret_cast<Vector<DialogButtonInfo>*>(lparam);
    int id = ::GetDlgCtrlID(hwnd);
    if (id == IDABORT || id == IDRETRY || id == IDIGNORE || id == IDYES || id == IDNO || id == IDCANCEL)
    {
        buttons.push_back({ id, hwnd });
    }
    return TRUE;
}

Semaphore semaphore(1);

} // namespace MessageBoxDetail

int MessageBox(const String& title, const String& message, const Vector<String>& buttons, int defaultButton)
{
    using namespace DAVA::Private;

    struct MessageBoxParams
    {
        WideString title;
        WideString message;
        Vector<WideString> buttons;
        int defaultButton;

        bool (*onEnter)();
        void (*onLeave)();
    };

    DVASSERT(0 < buttons.size() && buttons.size() <= 3);
    DVASSERT(0 <= defaultButton && defaultButton < static_cast<int>(buttons.size()));

    MessageBoxParams params;
    params.title = UTF8Utils::EncodeToWideString(title);
    params.message = UTF8Utils::EncodeToWideString(message);
    params.defaultButton = defaultButton;
    for (const String& s : buttons)
    {
        params.buttons.push_back(UTF8Utils::EncodeToWideString(s));
    }

    int result = -1;
    auto showMessageBox = [&params, &result]() {
        if (params.onEnter())
        {
            HWND hwnd = ::GetActiveWindow();
            MessageBoxDetail::MessageBoxHook msgBox;
            result = msgBox.Show(hwnd,
                                 std::move(params.title),
                                 std::move(params.message),
                                 std::move(params.buttons),
                                 params.defaultButton);

            params.onLeave();
        }
    };

#if defined(__DAVAENGINE_QT__)
    params.onEnter = []() -> bool {
        if (!EngineBackend::showingModalMessageBox)
        {
            EngineBackend::showingModalMessageBox = true;
            Window* primaryWindow = GetPrimaryWindow();
            if (primaryWindow != nullptr && primaryWindow->IsAlive())
                PlatformApi::Qt::AcquireWindowContext(primaryWindow);
            return true;
        }
        return false;
    };
    params.onLeave = []() {
        Window* primaryWindow = GetPrimaryWindow();
        if (primaryWindow != nullptr && primaryWindow->IsAlive())
            PlatformApi::Qt::ReleaseWindowContext(primaryWindow);
        EngineBackend::showingModalMessageBox = false;
    };
    Window* primaryWindow = GetPrimaryWindow();
    if (Thread::IsMainThread())
    {
        showMessageBox();
    }
    else if (primaryWindow != nullptr && primaryWindow->IsAlive())
    {
        primaryWindow->RunOnUIThread(showMessageBox);
    }
#else
    params.onEnter = []() -> bool { return true; };
    params.onLeave = []() {};
    if (Thread::IsMainThread())
    {
        showMessageBox();
    }
    else
    {
        MessageBoxDetail::semaphore.Wait();

        Thread* t = Thread::Create([&showMessageBox]() { showMessageBox(); });
        t->Start();
        t->Join();
        t->Release();

        MessageBoxDetail::semaphore.Post();
    }
#endif
    return result;
}

} // namespace Debug
} // namespace DAVA
