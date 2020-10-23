#pragma once

#include "Base/BaseTypes.h"
#include "UI/Components/UIComponent.h"
#include "UIActionMap.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UIControl;

class UIEventBindingComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIEventBindingComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIEventBindingComponent);

public:
    UIEventBindingComponent();
    UIEventBindingComponent(const UIEventBindingComponent& src);

protected:
    virtual ~UIEventBindingComponent();

private:
    UIEventBindingComponent& operator=(const UIEventBindingComponent&) = delete;

public:
    UIEventBindingComponent* Clone() const override;

    UIActionMap& GetActionMap();

    void BindAction(const FastName& eventName, const UIActionMap::SimpleAction& action);
    void UnbindAction(const FastName& eventName);

private:
    UIActionMap actionMap;
};
}
