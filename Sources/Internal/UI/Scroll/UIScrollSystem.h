#pragma once

#include "Base/BaseTypes.h"
#include "UI/UISystem.h"
#include "Base/RefPtr.h"

namespace DAVA
{
class UIScrollViewContainer;

class UIScrollSystem : public UISystem
{
public:
    UIScrollSystem();
    ~UIScrollSystem() override;

    void ScheduleScrollToControl(UIControl* control);
    void ScheduleScrollToControlWithAnimation(UIControl* control, float32 animationTime);

protected:
    void RegisterControl(UIControl* control) override;
    void UnregisterControl(UIControl* control) override;
    void RegisterComponent(UIControl* control, UIComponent* component) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

    void Process(DAVA::float32 elapsedTime) override;
    void ForceProcessControl(float32 elapsedTime, UIControl* control) override;

private:
    struct ScheduledControl
    {
        ScheduledControl(UIControl* control_, bool withAnimation_ = false, float32 animationTime_ = 0.3f)
            : withAnimation(withAnimation_)
            , animationTime(animationTime_)
        {
            control = control_;
        }

        RefPtr<UIControl> control;
        bool withAnimation = false;
        float32 animationTime = 0;
    };

    void PrepareForScreenshotImpl(UIControl* control);
    void ScheduleScrollToControlImpl(UIControl* control, bool withAnimation, float32 animationTime);
    void ScrollToScheduledControl(const ScheduledControl& c);
    Vector<UIScrollViewContainer*> scrollViewContainers;

    Vector<ScheduledControl> scheduledControls;
};
}
