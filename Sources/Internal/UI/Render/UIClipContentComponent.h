#pragma once

#include "Base/BaseTypes.h"
#include "UI/Components/UIComponent.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UIClipContentComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIClipContentComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIClipContentComponent);

public:
    UIClipContentComponent();
    UIClipContentComponent(const UIClipContentComponent& src);

    UIClipContentComponent* Clone() const override;

    void SetEnabled(bool _enabled);
    bool IsEnabled() const;

private:
    ~UIClipContentComponent() override = default;
    UIClipContentComponent& operator=(const UIClipContentComponent&) = delete;

    bool enabled = true;
};
}
