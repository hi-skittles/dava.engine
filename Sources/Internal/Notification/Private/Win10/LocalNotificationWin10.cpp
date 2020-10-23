#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN_UAP__)

#include "Notification/Private/Win10/LocalNotificationWin10.h"
#include "Utils/StringFormat.h"
#include "Utils/UTF8Utils.h"
#include "Logger/Logger.h"

namespace DAVA
{
::Windows::Data::Xml::Dom::XmlDocument ^ GenerateToastDeclaration(const String& title, const String& text, bool useSound, Platform::String ^ notificationId)
{
    using namespace ::Windows::Data::Xml::Dom;
    using namespace ::Windows::UI::Notifications;

    Platform::String ^ toastTitle = ref new Platform::String(UTF8Utils::EncodeToWideString(title).c_str());
    Platform::String ^ toastText = ref new Platform::String(UTF8Utils::EncodeToWideString(text).c_str());
    XmlDocument ^ toastXml = ToastNotificationManager::GetTemplateContent(ToastTemplateType::ToastText02);

    Platform::String ^ sttr = toastXml->GetXml();

    //Set the title and the text
    XmlNodeList ^ toastTextElements = toastXml->GetElementsByTagName("text");
    toastTextElements->GetAt(0)->AppendChild(toastXml->CreateTextNode(toastTitle));
    toastTextElements->GetAt(1)->AppendChild(toastXml->CreateTextNode(toastText));

    IXmlNode ^ toastNode = toastXml->SelectSingleNode("/toast");
    XmlElement ^ toastElement = static_cast<XmlElement ^>(toastNode);
    toastElement->SetAttribute("launch", notificationId);
    //Set silence
    if (!useSound)
    {
        XmlElement ^ audioNode = toastXml->CreateElement("audio");
        audioNode->SetAttribute("silent", "true");
        toastNode->AppendChild(audioNode);
    }
    return toastXml;
}

LocalNotificationUAP::LocalNotificationUAP(const String& _id)
{
    using ::Windows::UI::Notifications::ToastNotificationManager;

    notificationId = _id;
    nativeNotificationId = ref new Platform::String(UTF8Utils::EncodeToWideString(_id).c_str());

    // Microsoft says that:
    // "The presence of the notification system on the customer's machine is outside the control of the app.
    // If notifications aren't available, inadvertently using the notification interfaces generates an exception that should be handled by the app"
    // (https://blogs.windows.com/buildingapps/2014/05/23/understanding-and-resolving-app-crashes-and-failures-in-windows-store-apps-part-2-json-and-tiles/#aRFLBMqSo2IswEUh.97)
    //
    // CreateToastNotifier and other methods might throw an exception, e.g. with WPN_E_PLATFORM_UNAVAILABLE in case notification system is not initialized
    // We want to handle that by catching exception and claiming notification object to be invalid (via toastNotifier == nullptr check), so that subsequent calls to it do not do anything
    try
    {
        toastNotifier = ToastNotificationManager::CreateToastNotifier();
    }
    catch (Platform::Exception ^ e)
    {
        DVASSERT(false);
        Logger::Error("Exception occured when tried to create toast notifier: %s (hresult=0x%08X)", UTF8Utils::EncodeToUTF8(e->Message->Data()).c_str(), e->HResult);
    }
}

void LocalNotificationUAP::SetAction(const String& action)
{
}

void LocalNotificationUAP::Hide()
{
    if (toastNotifier == nullptr)
    {
        return;
    }

    if (!notification)
    {
        return;
    }

    toastNotifier->Hide(notification);
    notification = nullptr;
}

void LocalNotificationUAP::ShowText(const String& title, const String& text, bool useSound)
{
    using ::Windows::Data::Xml::Dom::XmlDocument;

    if (toastNotifier == nullptr)
    {
        return;
    }

    XmlDocument ^ toastDoc = GenerateToastDeclaration(title, text, useSound, nativeNotificationId);
    CreateOrUpdateNotification(toastDoc);
}

void LocalNotificationUAP::ShowProgress(const String& title,
                                        const String& text,
                                        uint32 total,
                                        uint32 progress,
                                        bool useSound)
{
    using ::Windows::Data::Xml::Dom::XmlDocument;

    if (toastNotifier == nullptr)
    {
        return;
    }

    double percentage = (static_cast<double>(progress) / total) * 100.0;
    String titleText = title + Format(" %.02f%%", percentage);
    XmlDocument ^ toastDoc = GenerateToastDeclaration(titleText, text, useSound, nativeNotificationId);

    CreateOrUpdateNotification(toastDoc, 0, true);
}

void LocalNotificationUAP::PostDelayedNotification(const String& title,
                                                   const String& text,
                                                   int delaySeconds,
                                                   bool useSound)
{
    using ::Windows::Data::Xml::Dom::XmlDocument;

    if (toastNotifier == nullptr)
    {
        return;
    }

    XmlDocument ^ toastDoc = GenerateToastDeclaration(title, text, useSound, nativeNotificationId);

    CreateOrUpdateNotification(toastDoc, delaySeconds);
}

void LocalNotificationUAP::RemoveAllDelayedNotifications()
{
    if (toastNotifier == nullptr)
    {
        return;
    }

    try
    {
        auto scheduledNotifications = toastNotifier->GetScheduledToastNotifications();

        for (unsigned i = 0; i < scheduledNotifications->Size; ++i)
        {
            toastNotifier->RemoveFromSchedule(scheduledNotifications->GetAt(i));
        }
    }
    catch (Platform::Exception ^ e)
    {
        DVASSERT(false);
        Logger::Error("Exception occured when tried to removed notification from schedule: %s (hresult=0x%08X)", UTF8Utils::EncodeToUTF8(e->Message->Data()).c_str(), e->HResult);
    }
}

void LocalNotificationUAP::CreateOrUpdateNotification(::Windows::Data::Xml::Dom::XmlDocument ^ notificationDeclaration,
                                                      int32 delayInSeconds,
                                                      bool ghostNotification)
{
    using namespace ::Windows::UI::Notifications;

    try
    {
        if (delayInSeconds < 0)
        {
            DVASSERT(false, Format("Attempt to create a local notification in the past. Requested delay in seconds = %d. Ignored", delayInSeconds).c_str());
        }
        else if (delayInSeconds == 0)
        {
            ToastNotification ^ notif = ref new ToastNotification(notificationDeclaration);
            notif->SuppressPopup = ghostNotification;
            toastNotifier->Show(notif);

            if (notification)
            {
                toastNotifier->Hide(notification);
            }
            notification = notif;
        }
        else
        {
            auto scheduledNotifications = toastNotifier->GetScheduledToastNotifications();
            if (scheduledNotifications->Size >= 4096)
            {
                DVASSERT(false, "UWP forbids scheduling more than 4096 notifications. Ignored");
                return;
            }

            Windows::Globalization::Calendar ^ calendar = ref new Windows::Globalization::Calendar;
            calendar->AddSeconds(delayInSeconds);
            Windows::Foundation::DateTime deliveryTime = calendar->GetDateTime();

            ScheduledToastNotification ^ notif = ref new ScheduledToastNotification(notificationDeclaration, deliveryTime);
            notif->SuppressPopup = ghostNotification;

            toastNotifier->AddToSchedule(notif);
        }
    }
    catch (Platform::Exception ^ e)
    {
        Logger::Error("Exception occured when tried to create or update notification: %s (hresult=0x%08X)", UTF8Utils::EncodeToUTF8(e->Message->Data()).c_str(), e->HResult);
    }
}

LocalNotificationImpl* LocalNotificationImpl::Create(const String& _id)
{
    return new LocalNotificationUAP(_id);
}

void LocalNotificationImpl::RequestPermissions()
{
}

} // namespace DAVA

#endif
