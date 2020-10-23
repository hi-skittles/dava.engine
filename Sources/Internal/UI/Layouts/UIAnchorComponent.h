#ifndef __DAVAENGINE_UI_ANCHOR_COMPONENT_H__
#define __DAVAENGINE_UI_ANCHOR_COMPONENT_H__

#include "UI/Components/UIComponent.h"
#include <bitset>

namespace DAVA
{
class UIAnchorComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIAnchorComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIAnchorComponent);

public:
    UIAnchorComponent();
    UIAnchorComponent(const UIAnchorComponent& src);

protected:
    virtual ~UIAnchorComponent();

private:
    UIAnchorComponent& operator=(const UIAnchorComponent&) = delete;

public:
    UIAnchorComponent* Clone() const override;

    bool IsEnabled() const;
    void SetEnabled(bool enabled);

    bool IsLeftAnchorEnabled() const;
    void SetLeftAnchorEnabled(bool enabled);

    float32 GetLeftAnchor() const;
    void SetLeftAnchor(float32 anchor);

    bool IsHCenterAnchorEnabled() const;
    void SetHCenterAnchorEnabled(bool enabled);

    float32 GetHCenterAnchor() const;
    void SetHCenterAnchor(float32 anchor);

    bool IsRightAnchorEnabled() const;
    void SetRightAnchorEnabled(bool enabled);

    float32 GetRightAnchor() const;
    void SetRightAnchor(float32 anchor);

    bool IsTopAnchorEnabled() const;
    void SetTopAnchorEnabled(bool enabled);

    float32 GetTopAnchor() const;
    void SetTopAnchor(float32 anchor);

    bool IsVCenterAnchorEnabled() const;
    void SetVCenterAnchorEnabled(bool enabled);

    float32 GetVCenterAnchor() const;
    void SetVCenterAnchor(float32 anchor);

    bool IsBottomAnchorEnabled() const;
    void SetBottomAnchorEnabled(bool enabled);

    float32 GetBottomAnchor() const;
    void SetBottomAnchor(float32 anchor);

    bool IsUseRtl() const;
    void SetUseRtl(bool use);

private:
    enum eFlags
    {
        FLAG_ENABLED,
        FLAG_LEFT_ENABLED,
        FLAG_HCENTER_ENABLED,
        FLAG_RIGHT_ENABLED,
        FLAG_TOP_ENABLED,
        FLAG_VCENTER_ENABLED,
        FLAG_BOTTOM_ENABLED,
        FLAG_USE_RTL,
        FLAGS_COUNT
    };

    void SetLayoutDirty();
    void SetFlag(eFlags flag, bool enabled);

    std::bitset<FLAGS_COUNT> flags;
    float32 leftAnchor = 0.0f;
    float32 hCenterAnchor = 0.0f;
    float32 rightAnchor = 0.0f;
    float32 topAnchor = 0.0f;
    float32 vCenterAnchor = 0.0f;
    float32 bottomAnchor = 0.0f;
};
}


#endif //__DAVAENGINE_UI_ANCHOR_HINT_COMPONENT_H__
