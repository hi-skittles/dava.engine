#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
/**
 Component for UIUpdateSystem that defines which frameDelta will be send to UIControl.
 Temporary component for backward compatibility with existing code.
 **WILL BE CHANGED** after design replays/custom speed logic.
 */
class UICustomUpdateDeltaComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UICustomUpdateDeltaComponent, UIComponent);
    DECLARE_UI_COMPONENT(UICustomUpdateDeltaComponent);

public:
    UICustomUpdateDeltaComponent();
    UICustomUpdateDeltaComponent(const UICustomUpdateDeltaComponent& src);
    UIComponent* Clone() const override;

    void SetDelta(float32 delta);
    float32 GetDelta() const;

protected:
    ~UICustomUpdateDeltaComponent() override;

private:
    float32 customDelta = 0.f;
};

inline float32 UICustomUpdateDeltaComponent::GetDelta() const
{
    return customDelta;
}
}