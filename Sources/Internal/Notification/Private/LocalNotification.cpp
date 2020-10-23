#include "Notification/LocalNotification.h"
#include "Utils/Utils.h"
#include "Concurrency/LockGuard.h"
#include "Notification/Private/LocalNotificationImpl.h"

namespace DAVA
{
LocalNotification::LocalNotification()
{
    impl = LocalNotificationImpl::Create(GenerateGUID());
}

LocalNotification::~LocalNotification()
{
    delete impl;
}

void LocalNotification::SetAction(const Message& msg)
{
    action = msg;
    impl->SetAction("");
}

void LocalNotification::SetTitle(const String& _title)
{
    if (_title != title)
    {
        isChanged = true;
        title = _title;
    }
}

void LocalNotification::SetText(const String& _text)
{
    if (_text != text)
    {
        isChanged = true;
        text = _text;
    }
}

void LocalNotification::SetUseSound(const bool value)
{
    if (useSound != value)
    {
        isChanged = true;
        useSound = value;
    }
}

void LocalNotification::Show()
{
    if (!isVisible)
    {
        isVisible = true;
        isChanged = true;
    }
}

void LocalNotification::Hide()
{
    if (isVisible)
    {
        isVisible = false;
        isChanged = true;
    }
}

void LocalNotification::Update()
{
    if (false == isChanged)
        return;

    if (true == isVisible)
        ImplShow();
    else
        impl->Hide();

    isChanged = false;
}

const DAVA::String& LocalNotification::GetId() const
{
    return impl->GetId();
}
} // namespace DAVA
