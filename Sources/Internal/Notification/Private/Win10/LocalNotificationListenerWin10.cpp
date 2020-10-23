#include "Notification/Private/Win10/LocalNotificationListenerWin10.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include "Notification/LocalNotificationController.h"

#include "Engine/Engine.h"
#include "Logger/Logger.h"
#include "Utils/UTF8Utils.h"
namespace DAVA
{
namespace Private
{
LocalNotificationListener::LocalNotificationListener(LocalNotificationController& controller)
    : localNotificationController(controller)
{
    PlatformApi::Win10::RegisterXamlApplicationListener(this);
}

LocalNotificationListener::~LocalNotificationListener()
{
    PlatformApi::Win10::UnregisterXamlApplicationListener(this);
}

void LocalNotificationListener::OnLaunched(::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs ^ launchArgs)
{
    using namespace DAVA;
    String arguments = UTF8Utils::EncodeToUTF8(launchArgs->Arguments->Data());
    if (launchArgs->Kind == Windows::ApplicationModel::Activation::ActivationKind::Launch)
    {
        Platform::String ^ launchString = launchArgs->Arguments;
        if (!arguments.empty())
        {
            auto function = [this, arguments]()
            {
                localNotificationController.OnNotificationPressed(arguments);
            };
            RunOnMainThreadAsync(function);
        }
    }
}
} // namespace Private
} // namespace DAVA
#endif // __DAVAENGINE_WIN_UAP__
