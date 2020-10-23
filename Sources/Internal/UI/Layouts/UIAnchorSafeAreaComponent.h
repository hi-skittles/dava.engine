#pragma once

#include "UI/Components/UIComponent.h"
#include <bitset>

namespace DAVA
{
/**
 We can use this component to mark controls which we would like to move to safe 
 area on iPhone X display.
 
 Insets can has follow values:
  - NONE - ignores insets on this side
  - INSET - moves the control to safe area in any case
  - INSET_ONLY_IF_NOTCH - move control if the iPhone X notch is on this side
  - REVERSE - moves the control in opposit side (useful to move the control in one side but leave the internal background on previous place)
 */
class UIAnchorSafeAreaComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIAnchorCorrectionComponent, UIComponent);

public:
    enum class eInsetType
    {
        NONE,
        INSET,
        INSET_ONLY_IF_NOTCH,
        REVERSE
    };

    DECLARE_UI_COMPONENT(UIAnchorCorrectionComponent);

    UIAnchorSafeAreaComponent();
    UIAnchorSafeAreaComponent(const UIAnchorSafeAreaComponent& src);

    UIAnchorSafeAreaComponent& operator=(const UIAnchorSafeAreaComponent&) = delete;
    UIAnchorSafeAreaComponent* Clone() const override;

    eInsetType GetLeftInset() const;
    void SetLeftInset(eInsetType inset);

    eInsetType GetTopInset() const;
    void SetTopInset(eInsetType inset);

    eInsetType GetRightInset() const;
    void SetRightInset(eInsetType inset);

    eInsetType GetBottomInset() const;
    void SetBottomInset(eInsetType inset);

    float32 GetLeftInsetCorrection() const;
    void SetLeftInsetCorrection(float32 correction);

    float32 GetTopInsetCorrection() const;
    void SetTopInsetCorrection(float32 correction);

    float32 GetRightInsetCorrection() const;
    void SetRightInsetCorrection(float32 correction);

    float32 GetBottomInsetCorrection() const;
    void SetBottomInsetCorrection(float32 correction);

private:
    virtual ~UIAnchorSafeAreaComponent();
    void MarkDirty();

    eInsetType leftInset = eInsetType::NONE;
    eInsetType topInset = eInsetType::NONE;
    eInsetType rightInset = eInsetType::NONE;
    eInsetType bottomInset = eInsetType::NONE;

    float32 leftInsetCorrection = 0.0f;
    float32 topInsetCorrection = 0.0f;
    float32 rightInsetCorrection = 0.0f;
    float32 bottomInsetCorrection = 0.0f;
};
}
