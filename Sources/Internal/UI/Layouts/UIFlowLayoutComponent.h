#ifndef __DAVAENGINE_UI_FLOW_LAYOUT_COMPONENT_H__
#define __DAVAENGINE_UI_FLOW_LAYOUT_COMPONENT_H__

#include "Math/Vector.h"
#include "UI/Components/UIComponent.h"
#include "Reflection/Reflection.h"
#include <bitset>

namespace DAVA
{
class UIFlowLayoutComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIFlowLayoutComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIFlowLayoutComponent);

public:
    enum eOrientation
    {
        ORIENTATION_LEFT_TO_RIGHT,
        ORIENTATION_RIGHT_TO_LEFT
    };

public:
    UIFlowLayoutComponent();
    UIFlowLayoutComponent(const UIFlowLayoutComponent& src);

protected:
    virtual ~UIFlowLayoutComponent();

private:
    UIFlowLayoutComponent& operator=(const UIFlowLayoutComponent&) = delete;

public:
    virtual UIFlowLayoutComponent* Clone() const override;

    bool IsEnabled() const;
    void SetEnabled(bool enabled);

    eOrientation GetOrientation() const;
    void SetOrientation(eOrientation orientation);

    float32 GetHorizontalPadding() const;
    void SetHorizontalPadding(float32 padding);

    float32 GetHorizontalSpacing() const;
    void SetHorizontalSpacing(float32 spacing);

    bool IsDynamicHorizontalPadding() const;
    void SetDynamicHorizontalPadding(bool dynamic);

    bool IsDynamicHorizontalInLinePadding() const;
    void SetDynamicHorizontalInLinePadding(bool dynamic);

    bool IsHorizontalSafeAreaPaddingInset() const;
    void SetHorizontalSafeAreaPaddingInset(bool inset);

    bool IsDynamicHorizontalSpacing() const;
    void SetDynamicHorizontalSpacing(bool dynamic);

    float32 GetVerticalPadding() const;
    void SetVerticalPadding(float32 padding);

    float32 GetVerticalSpacing() const;
    void SetVerticalSpacing(float32 spacing);

    bool IsDynamicVerticalPadding() const;
    void SetDynamicVerticalPadding(bool dynamic);

    bool IsVerticalSafeAreaPaddingInset() const;
    void SetVerticalSafeAreaPaddingInset(bool inset);

    bool IsDynamicVerticalSpacing() const;
    void SetDynamicVerticalSpacing(bool dynamic);

    float32 GetPaddingByAxis(int32 axis) const;
    float32 GetSpacingByAxis(int32 axis) const;

    bool IsUseRtl() const;
    void SetUseRtl(bool use);

    bool IsSkipInvisibleControls() const;
    void SetSkipInvisibleControls(bool skip);

private:
    enum eFlags
    {
        FLAG_ENABLED,
        FLAG_DYNAMIC_HORIZONTAL_PADDING,
        FLAG_DYNAMIC_HORIZONTAL_IN_LINE_PADDING,
        FLAG_HORIZONTAL_SAFE_AREA_PADDING,
        FLAG_DYNAMIC_HORIZONTAL_SPACING,
        FLAG_DYNAMIC_VERTICAL_PADDING,
        FLAG_VERTICAL_SAFE_AREA_PADDING,
        FLAG_DYNAMIC_VERTICAL_SPACING,
        FLAG_USE_RTL,
        FLAG_SKIP_INVISIBLE_CONTROLS,
        FLAG_IS_RIGHT_TO_LEFT,
        FLAG_COUNT
    };

    int32 GetOrientationAsInt() const;
    void SetOrientationFromInt(int32 orientation);

    void SetLayoutDirty();
    void SetFlag(eFlags flag, bool enabled);

    Array<float32, Vector2::AXIS_COUNT> padding;
    Array<float32, Vector2::AXIS_COUNT> spacing;

    std::bitset<eFlags::FLAG_COUNT> flags;
};
}


#endif //__DAVAENGINE_UI_FLOW_LAYOUT_COMPONENT_H__
