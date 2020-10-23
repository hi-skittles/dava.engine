#pragma once
#include "Base/BaseTypes.h"
#include "Math/Color.h"
#include "Reflection/Reflection.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
class UIDebugRenderComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIDebugRenderComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIDebugRenderComponent);

public:
    enum ePivotPointDrawMode
    {
        DRAW_NEVER = 0, //!<Never draw the Pivot Point.
        DRAW_ONLY_IF_NONZERO, //!<Draw the Pivot Point only if it is defined (nonzero).
        DRAW_ALWAYS //!<Always draw the Pivot Point mark.
    };

    UIDebugRenderComponent();
    UIDebugRenderComponent(const UIDebugRenderComponent& src);

    UIDebugRenderComponent* Clone() const override;

    void SetEnabled(bool _debugDrawEnabled);
    bool IsEnabled() const;

    void SetDrawColor(const Color& color);
    const Color& GetDrawColor() const;

    void SetPivotPointDrawMode(ePivotPointDrawMode mode);
    ePivotPointDrawMode GetPivotPointDrawMode() const;

private:
    ~UIDebugRenderComponent() override = default;
    UIDebugRenderComponent& operator=(const UIDebugRenderComponent&) = delete;

    bool enabled = true;
    Color drawColor = Color::Red;
    ePivotPointDrawMode pivotPointDrawMode = DRAW_NEVER;
};
}
