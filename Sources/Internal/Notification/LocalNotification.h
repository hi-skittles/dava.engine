#pragma once

#include "Base/BaseTypes.h"
#include "Base/Singleton.h"
#include "Base/Message.h"

namespace DAVA
{
class LocalNotificationImpl;

class LocalNotification : public BaseObject
{
protected:
    LocalNotification();
    virtual ~LocalNotification();

public:
    void SetAction(const Message& msg);
    inline void RunAction();

    void SetTitle(const String& _title);
    void SetText(const String& _text);
    void SetUseSound(const bool value);

    void Show();
    void Hide();
    void Update();

    inline bool IsChanged() const;
    inline bool IsVisible() const;
    const DAVA::String& GetId() const;

private:
    virtual void ImplShow() = 0;

protected:
    LocalNotificationImpl* impl = nullptr;

    bool isChanged = false;
    bool isVisible = true;

    Message action;

    String title;
    String text;
    bool useSound = false;
};

inline void LocalNotification::RunAction()
{
    action(this);
}

inline bool LocalNotification::IsChanged() const
{
    return isChanged;
}

inline bool LocalNotification::IsVisible() const
{
    return isVisible;
}
} // namespace DAVA
