#pragma once

#include "Base/BaseTypes.h"
#include "Engine/Engine.h"
#include "Reflection/Reflection.h"
#include "UI/UIControl.h"

namespace DAVA
{
class UIScreenTransition;
class UIControlSystem;
class UIScreen : public UIControl
{
    DAVA_VIRTUAL_REFLECTION(UIScreen, UIControl);

public:
    enum eFillBorderOrder
    {
        FILL_BORDER_BEFORE_DRAW = 0,
        FILL_BORDER_AFTER_DRAW,
        FILL_BORDER_NONE
    };

protected:
    virtual ~UIScreen();

public:
    UIScreen();
    UIScreen(const Rect& rect);

    /* 
		This is block of functions used by transition
	 */
    virtual void LoadGroup();
    virtual void UnloadGroup();
    virtual bool IsLoaded();

    virtual void AddToGroup(int32 groupId);
    virtual void RemoveFromGroup();

    virtual int32 GetGroupId();

    void SystemScreenSizeChanged(const Rect& newFullScreenSize) override;

protected:
    virtual void LoadResources()
    {
    }
    virtual void UnloadResources()
    {
    }

    int32 groupId;

private:
    bool isLoaded;
    static List<UIScreen*> appScreens;
    static int32 groupIdCounter;
};
};
