#ifndef __DAVAENGINE_UI_LINEAR_LAYOUT_COMPONENT_H__
#define __DAVAENGINE_UI_LINEAR_LAYOUT_COMPONENT_H__

#include "Math/Vector.h"
#include "UI/Components/UIComponent.h"
#include "Reflection/Reflection.h"
#include <bitset>

namespace DAVA
{
class UILinearLayoutComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UILinearLayoutComponent, UIComponent);
    DECLARE_UI_COMPONENT(UILinearLayoutComponent);

public:
    enum eOrientation
    {
        LEFT_TO_RIGHT = 0,
        RIGHT_TO_LEFT = 1,
        TOP_DOWN = 2,
        BOTTOM_UP = 3,
    };

public:
    UILinearLayoutComponent();
    UILinearLayoutComponent(const UILinearLayoutComponent& src);

protected:
    virtual ~UILinearLayoutComponent();

private:
    UILinearLayoutComponent& operator=(const UILinearLayoutComponent&) = delete;

public:
    virtual UILinearLayoutComponent* Clone() const override;

    bool IsEnabled() const;
    void SetEnabled(bool enabled);

    eOrientation GetOrientation() const;
    void SetOrientation(eOrientation orientation);

    Vector2::eAxis GetAxis() const;
    bool IsInverse() const;

    float32 GetPadding() const;
    void SetPadding(float32 padding);

    float32 GetSpacing() const;
    void SetSpacing(float32 spacing);

    bool IsDynamicPadding() const;
    void SetDynamicPadding(bool dynamic);

    bool IsSafeAreaPaddingInset() const;
    void SetSafeAreaPaddingInset(bool inset);

    bool IsDynamicSpacing() const;
    void SetDynamicSpacing(bool dynamic);
    bool IsUseRtl() const;
    void SetUseRtl(bool use);

    bool IsSkipInvisibleControls() const;
    void SetSkipInvisibleControls(bool skip);

private:
    enum eFlags
    {
        FLAG_ENABLED,
        FLAG_ORIENTATION_VERTICAL,
        FLAG_ORIENTATION_INVERSE,
        FLAG_DYNAMIC_PADDING,
        FLAG_SAFE_AREA_PADDING_INSET,
        FLAG_DYNAMIC_SPACING,
        FLAG_RTL,
        FLAG_SKIP_INVISIBLE_CONTROLS,
        FLAG_COUNT
    };
    int32 GetOrientationAsInt() const;
    void SetOrientationFromInt(int32 orientation);

    void SetLayoutDirty();
    void SetFlag(eFlags flag, bool enabled);

    std::bitset<eFlags::FLAG_COUNT> flags;
    float32 padding = 0.0f;
    float32 spacing = 0.0f;
};
}


#endif //__DAVAENGINE_UI_LINEAR_LAYOUT_COMPONENT_H__
