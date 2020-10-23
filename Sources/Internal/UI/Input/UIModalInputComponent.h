#ifndef __DAVAENGINE_UI_MODAL_INPUT_COMPONENT_H__
#define __DAVAENGINE_UI_MODAL_INPUT_COMPONENT_H__

#include "Base/BaseTypes.h"

#include "UI/Components/UIComponent.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UIControl;

class UIModalInputComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIModalInputComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIModalInputComponent);

public:
    UIModalInputComponent();
    UIModalInputComponent(const UIModalInputComponent& src);

protected:
    virtual ~UIModalInputComponent();

private:
    UIModalInputComponent& operator=(const UIModalInputComponent&) = delete;

public:
    UIModalInputComponent* Clone() const override;

    bool IsEnabled() const;
    void SetEnabled(bool enabled_);

private:
    bool enabled = true;
};
}



#endif // __DAVAENGINE_UI_MODAL_INPUT_COMPONENT_H__
