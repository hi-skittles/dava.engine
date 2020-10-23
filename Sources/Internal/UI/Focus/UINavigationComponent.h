#ifndef __DAVAENGINE_UI_NAVIGATION_COMPONENT_H__
#define __DAVAENGINE_UI_NAVIGATION_COMPONENT_H__

#include "Base/BaseTypes.h"

#include "UI/Components/UIComponent.h"
#include "UI/Focus/FocusHelpers.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UINavigationComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UINavigationComponent, UIComponent);
    DECLARE_UI_COMPONENT(UINavigationComponent);

public:
    enum Direction
    {
        LEFT = 0,
        RIGHT,
        UP,
        DOWN,

        DIRECTION_COUNT
    };

    UINavigationComponent();
    UINavigationComponent(const UINavigationComponent& src);

protected:
    virtual ~UINavigationComponent();

private:
    UINavigationComponent& operator=(const UINavigationComponent&) = delete;

public:
    UINavigationComponent* Clone() const override;

    const String& GetNextFocusLeft() const;
    void SetNextFocusLeft(const String& val);

    const String& GetNextFocusRight() const;
    void SetNextFocusRight(const String& val);

    const String& GetNextFocusUp() const;
    void SetNextFocusUp(const String& val);

    const String& GetNextFocusDown() const;
    void SetNextFocusDown(const String& val);

    const String& GetNextControlPathInDirection(Direction dir);

private:
    String nextFocusPath[DIRECTION_COUNT];
};
}


#endif //__DAVAENGINE_UI_NAVIGATION_COMPONENT_H__
