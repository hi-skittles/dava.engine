#include "Base/Platform.h"
#include "Base/BaseTypes.h"
#include "Concurrency/AutoResetEvent.h"
#include "Concurrency/Semaphore.h"
#include "Concurrency/Thread.h"
#include "Engine/PlatformApiWin10.h"
#include "Engine/Private/Win10/PlatformCoreWin10.h"
#include "Utils/UTF8Utils.h"

#include <ppltasks.h>

namespace DAVA
{
namespace Debug
{
namespace MessageBoxDetail
{
ref class MessageBoxImpl sealed
{
    // clang-format off
internal:
    static int ShowModal(const String& title, const String& message, const Vector<String>& buttons, int defaultButton);
    // clang-format on

    void OnSuspending();

private:
    // clang-format off
    // ref class can only inherit from a ref class or interface class so use proxy object
    struct SuspendProxy : public PlatformApi::Win10::XamlApplicationListener
    {
        SuspendProxy(MessageBoxImpl ^ target) : target(target) { }
        void OnSuspending() override { target->OnSuspending(); }
        MessageBoxImpl^ target = nullptr;
    };
    // clang-format on

    MessageBoxImpl(const String& title, const String& message, const Vector<String>& buttons, int defaultButton);

    int ShowModalBlockingDesktop();
    int ShowModalBlockingPhone();
    int ShowModalNonBlocking();

    ::Windows::UI::Popups::MessageDialog ^ CreateDialog(bool addClickHandlers);
    void OnButtonClick(::Windows::UI::Popups::IUICommand ^ cmd);

    ::Platform::String ^ title = nullptr;
    ::Platform::String ^ content = nullptr;
    Vector<::Platform::String ^> buttonNames;
    int defaultButton = 0;

    int result = -1;
    AutoResetEvent autoEvent;
    SuspendProxy proxy;
    static Semaphore semaphore;
};

Semaphore MessageBoxImpl::semaphore(1);

int MessageBoxImpl::ShowModal(const String& title, const String& message, const Vector<String>& buttons, int defaultButton)
{
    using ::Windows::UI::Core::CoreDispatcher;

    CoreDispatcher ^ disp = Private::PlatformCore::GetCoreDispatcher();
    if (!disp->HasThreadAccess)
    {
        // Wait until other blocking modal dialog is dismissed
        semaphore.Wait();

        MessageBoxImpl ^ msgbox = ref new MessageBoxImpl(title, message, buttons, defaultButton);
        int result = Private::PlatformCore::IsPhoneContractPresent() ? msgbox->ShowModalBlockingPhone() : msgbox->ShowModalBlockingDesktop();
        semaphore.Post(1);
        return result;
    }
    else
    {
        MessageBoxImpl ^ msgbox = ref new MessageBoxImpl(title, message, buttons, defaultButton);
        return msgbox->ShowModalNonBlocking();
    }
}

MessageBoxImpl::MessageBoxImpl(const String& title, const String& message, const Vector<String>& buttons, int defaultButton)
    : title(ref new ::Platform::String(UTF8Utils::EncodeToWideString(title).c_str()))
    , content(ref new ::Platform::String(UTF8Utils::EncodeToWideString(message).c_str()))
    , defaultButton(defaultButton)
    , proxy(this)
{
    // Ups, Win10 mobile apps support only 2 buttons otherwise exception is thrown
    const size_t MAX_BUTTONS = Private::PlatformCore::IsPhoneContractPresent() ? 2 : 3;
    size_t n = std::min<size_t>(MAX_BUTTONS, buttons.size());
    buttonNames.reserve(n);
    for (size_t i = 0; i < n; ++i)
    {
        buttonNames.push_back(ref new ::Platform::String(UTF8Utils::EncodeToWideString(buttons[i]).c_str()));
    }
}

void MessageBoxImpl::OnSuspending()
{
    // Unblock thread to avoid deadlock when MessageBox is shown from main thread and application goes suspended
    result = -1;
    autoEvent.Signal();
}

int MessageBoxImpl::ShowModalBlockingDesktop()
{
    using ::Windows::UI::Popups::MessageDialog;
    using namespace ::Windows::UI::Core;

    if (Thread::IsMainThread())
    {
        PlatformApi::Win10::RegisterXamlApplicationListener(&proxy);
    }

    CoreDispatcher ^ disp = Private::PlatformCore::GetCoreDispatcher();
    disp->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]() {
                       MessageDialog ^ msgbox = CreateDialog(true);
                       msgbox->ShowAsync();
                   }));

    autoEvent.Wait();
    if (Thread::IsMainThread())
    {
        PlatformApi::Win10::UnregisterXamlApplicationListener(&proxy);
    }
    return result;
}

int MessageBoxImpl::ShowModalBlockingPhone()
{
    using namespace ::Windows::UI::Core;
    using namespace ::Windows::UI::Popups;

    CoreDispatcher ^ disp = Private::PlatformCore::GetCoreDispatcher();
    disp->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]() {
                       MessageDialog ^ msgbox = CreateDialog(false);
                       concurrency::create_task(msgbox->ShowAsync()).then([this](IUICommand ^ cmd) {
                           OnButtonClick(cmd);
                       });
                   }));

    autoEvent.Wait();
    return result;
}

int MessageBoxImpl::ShowModalNonBlocking()
{
    using ::Windows::UI::Popups::MessageDialog;

    MessageDialog ^ msgbox = CreateDialog(false);
    msgbox->ShowAsync();
    return -1;
}

::Windows::UI::Popups::MessageDialog ^ MessageBoxImpl::CreateDialog(bool addClickHandlers)
{
    using namespace ::Windows::UI::Popups;
    using namespace ::Windows::UI::Xaml;

    MessageDialog ^ msgbox = ref new MessageDialog(nullptr);
    msgbox->Title = title;
    msgbox->Content = content;

    int index = 0;
    for (::Platform::String ^ s : buttonNames)
    {
        UICommand ^ cmd = ref new UICommand(s);
        cmd->Id = index;
        if (addClickHandlers)
        {
            cmd->Invoked = ref new UICommandInvokedHandler(this, &MessageBoxImpl::OnButtonClick);
        }
        msgbox->Commands->Append(cmd);
        index += 1;
    }
    msgbox->DefaultCommandIndex = defaultButton;
    return msgbox;
}

void MessageBoxImpl::OnButtonClick(::Windows::UI::Popups::IUICommand ^ cmd)
{
    result = cmd != nullptr ? safe_cast<int>(cmd->Id) : -1;
    autoEvent.Signal();
}

} // namespace MessageBoxDetail

int MessageBox(const String& title, const String& message, const Vector<String>& buttons, int defaultButton)
{
    DVASSERT(0 < buttons.size() && buttons.size() <= 3);
    DVASSERT(0 <= defaultButton && defaultButton < static_cast<int>(buttons.size()));

    return MessageBoxDetail::MessageBoxImpl::ShowModal(title, message, buttons, defaultButton);
}

} // namespace Debug
} // namespace DAVA
