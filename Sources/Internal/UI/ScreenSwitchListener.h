#pragma once

namespace DAVA
{
class UIScreen;

class ScreenSwitchListener
{
public:
    virtual ~ScreenSwitchListener() = default;

    virtual void OnScreenWillSwitch(UIScreen* newScreen)
    {
    }
    virtual void OnScreenDidSwitch(UIScreen* newScreen)
    {
    }
};
}
