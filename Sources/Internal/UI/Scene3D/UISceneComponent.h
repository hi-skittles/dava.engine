#pragma once

#include "UI/Components/UIComponent.h"

namespace DAVA
{
/** Component for mark UIControl what it has Scene (like UI3DView). */
class UISceneComponent final : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UISceneComponent, UIComponent);
    DECLARE_UI_COMPONENT(UISceneComponent);

public:
    UISceneComponent();
    UISceneComponent(const UISceneComponent& src);

    UISceneComponent* Clone() const override;

    UISceneComponent& operator=(const UISceneComponent&) = delete;

protected:
    ~UISceneComponent() override;
};
}
