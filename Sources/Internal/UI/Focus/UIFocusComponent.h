#ifndef __DAVAENGINE_UI_FOCUS_COMPONENT_H__
#define __DAVAENGINE_UI_FOCUS_COMPONENT_H__

#include "Base/BaseTypes.h"

#include "UI/Components/UIComponent.h"
#include "UI/Focus/FocusHelpers.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UIFocusComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIFocusComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIFocusComponent);

public:
    UIFocusComponent();
    UIFocusComponent(const UIFocusComponent& src);

protected:
    virtual ~UIFocusComponent();

private:
    UIFocusComponent& operator=(const UIFocusComponent&) = delete;

public:
    UIFocusComponent* Clone() const override;

    bool IsEnabled() const;
    void SetEnabled(bool value);

    bool IsRequestFocus() const;
    void SetRequestFocus(bool value);

private:
    bool enabled = true;
    bool requestFocus = false;
};
}


#endif //__DAVAENGINE_UI_FOCUS_COMPONENT_H__
