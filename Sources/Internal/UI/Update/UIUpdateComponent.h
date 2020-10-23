#pragma once

#include "Base/BaseTypes.h"
#include "UI/Components/UIComponent.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
/**
 Component-marker for UIUpdateSystem.
 Temporary component for backward compatibility with existing code.
 **WILL BE REMOVED** after refactoring all `UIControl::Update` logic.
 */
class UIUpdateComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIUpdateComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIUpdateComponent);

public:
    UIUpdateComponent();
    UIUpdateComponent(const UIUpdateComponent& src);
    UIComponent* Clone() const override;

    // Notificate what control with this component should
    // receive update signal if it invisible
    // Need for all native controls
    // TODO: On CoreV2 use for this purpose update signal from core
    void SetUpdateInvisible(bool value);
    bool GetUpdateInvisible() const;

protected:
    ~UIUpdateComponent() override;

private:
    bool updateInvisible = false;
};

inline void UIUpdateComponent::SetUpdateInvisible(bool value)
{
    updateInvisible = value;
}

inline bool DAVA::UIUpdateComponent::GetUpdateInvisible() const
{
    return updateInvisible;
}
}