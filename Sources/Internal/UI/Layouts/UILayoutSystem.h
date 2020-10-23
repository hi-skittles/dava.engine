#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "Base/Token.h"
#include "Functional/Signal.h"
#include "Math/Rect.h"
#include "UI/UISystem.h"

struct UILayoutSystemTest;

namespace DAVA
{
class UIControl;
class UIScreen;
class UIScreenTransition;
class LayoutFormula;

class UILayoutSystem : public UISystem
{
public:
    UILayoutSystem();
    ~UILayoutSystem() override;

    void SetCurrentScreen(const RefPtr<UIScreen>& screen);
    void SetPopupContainer(const RefPtr<UIControl>& popupContainer);

    bool IsRtl() const;
    void SetRtl(bool rtl);

    /**
     Engine Core uses this method to provide the iPhoneX displays safe area to layout system.  
     */
    void SetPhysicalSafeAreaInsets(float32 left, float32 top, float32 right, float32 bottom, bool isLeftNotch, bool isRightNotch);

    float32 GetSafeAreaLeftInset() const;
    float32 GetSafeAreaTopInset() const;
    float32 GetSafeAreaRightInset() const;
    float32 GetSafeAreaBottomInset() const;

    bool IsAutoupdatesEnabled() const;
    void SetAutoupdatesEnabled(bool enabled);

    void SetDirty();
    void CheckDirty();

    void ManualApplyLayout(UIControl* control); //DON'T USE IT!

    Signal<UIControl*> controlLayouted;
    Signal<UIControl*, Vector2::eAxis, const LayoutFormula*> formulaProcessed;
    Signal<UIControl*, Vector2::eAxis, const LayoutFormula*> formulaRemoved;

protected:
    void Process(float32 elapsedTime) override;
    void ForceProcessControl(float32 elapsedTime, UIControl* control) override;

    void UnregisterControl(UIControl* control) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

    void RegisterSystem() override;
    void UnregisterSystem() override;

private:
    UIControl* FindNotDependentOnChildrenControl(UIControl* control) const;
    bool HaveToLayoutAfterReorder(const UIControl* control) const;
    bool HaveToLayoutAfterReposition(const UIControl* control) const;

    void CollectControls(UIControl* control, bool recursive);
    void ProcessControlHierarhy(UIControl* control);
    void ProcessControl(UIControl* control);

    void UpdateVisibilityRect(const Rect& visibilityRect);

    bool autoupdatesEnabled = true;
    bool dirty = false;
    bool needUpdate = false;
    std::unique_ptr<class Layouter> sharedLayouter;
    RefPtr<UIScreen> currentScreen;
    RefPtr<UIControl> popupContainer;

    Token visibleFrameChangedToken;
    Token virtualSizeChangedToken;
    Token inputSizeChangedToken;

    float32 physicalLeftSafeAreaInset = 0.0f;
    float32 physicalTopSafeAreaInset = 0.0f;
    float32 physicalRightSafeAreaInset = 0.0f;
    float32 physicalBottomSafeAreaInset = 0.0f;
    bool isLeftNotch = false;
    bool isRightNotch = false;

    friend UILayoutSystemTest;
};

inline void UILayoutSystem::SetDirty()
{
    dirty = true;
}

inline void UILayoutSystem::CheckDirty()
{
    needUpdate = dirty;
    dirty = false;
}
}
