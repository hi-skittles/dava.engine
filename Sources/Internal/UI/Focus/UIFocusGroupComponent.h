#ifndef __DAVAENGINE_UI_FOCUS_GROUP_COMPONENT_H__
#define __DAVAENGINE_UI_FOCUS_GROUP_COMPONENT_H__

#include "Base/BaseTypes.h"

#include "UI/Components/UIComponent.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UIFocusGroupComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIFocusGroupComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIFocusGroupComponent);

public:
    UIFocusGroupComponent();
    UIFocusGroupComponent(const UIFocusGroupComponent& src);

protected:
    virtual ~UIFocusGroupComponent();

private:
    UIFocusGroupComponent& operator=(const UIFocusGroupComponent&) = delete;

public:
    UIFocusGroupComponent* Clone() const override;
};
}


#endif //__DAVAENGINE_UI_FOCUS_GROUP_COMPONENT_H__
