#include "UIScrollSystem.h"

#include "UI/UIControl.h"
#include "UI/UIScrollViewContainer.h"
#include "UI/UIControlHelpers.h"
#include "UI/Components/UIComponent.h"
#include "UI/Scroll/UIScrollComponent.h"

namespace DAVA
{
UIScrollSystem::UIScrollSystem()
{
}

UIScrollSystem::~UIScrollSystem()
{
}

void UIScrollSystem::ScheduleScrollToControl(UIControl* control)
{
    ScheduleScrollToControlImpl(control, false, 0.0f);
}

void UIScrollSystem::ScheduleScrollToControlWithAnimation(UIControl* control, float32 animationTime)
{
    ScheduleScrollToControlImpl(control, true, animationTime);
}

void UIScrollSystem::RegisterControl(UIControl* control)
{
    if (control->GetComponent<UIScrollComponent>())
    {
        scrollViewContainers.push_back(DynamicTypeCheck<UIScrollViewContainer*>(control));
    }
}

void UIScrollSystem::UnregisterControl(UIControl* control)
{
    if (control->GetComponent<UIScrollComponent>())
    {
        auto it = std::find(scrollViewContainers.begin(), scrollViewContainers.end(), control);
        if (it != scrollViewContainers.end())
        {
            scrollViewContainers.erase(it);
        }
        else
        {
            DVASSERT(false);
        }
    }
}

void UIScrollSystem::RegisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIScrollComponent>())
    {
        scrollViewContainers.push_back(DynamicTypeCheck<UIScrollViewContainer*>(control));
    }
}

void UIScrollSystem::UnregisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UIScrollComponent>())
    {
        auto it = std::find(scrollViewContainers.begin(), scrollViewContainers.end(), control);
        if (it != scrollViewContainers.end())
        {
            scrollViewContainers.erase(it);
        }
        else
        {
            DVASSERT(false);
        }
    }
}

void UIScrollSystem::Process(DAVA::float32 elapsedTime)
{
    for (UIScrollViewContainer* container : scrollViewContainers)
    {
        container->Update(elapsedTime);
    }

    for (const ScheduledControl& c : scheduledControls)
    {
        ScrollToScheduledControl(c);
    }
    scheduledControls.clear();
}

void UIScrollSystem::ForceProcessControl(float32 elapsedTime, UIControl* control)
{
    PrepareForScreenshotImpl(control);

    for (ScheduledControl& c : scheduledControls)
    {
        ScrollToScheduledControl(c);
    }
}

void UIScrollSystem::PrepareForScreenshotImpl(UIControl* control)
{
    for (const auto& c : control->GetChildren())
    {
        PrepareForScreenshotImpl(c.Get());
        if (c->GetComponent<UIScrollComponent>() != nullptr)
        {
            c->Update(0);
        }
    }
}

void UIScrollSystem::ScheduleScrollToControlImpl(UIControl* control, bool withAnimation, float32 animationTime)
{
    auto it = std::find_if(scheduledControls.begin(), scheduledControls.end(), [control](const ScheduledControl& c) -> bool {
        return c.control.Get() == control;
    });

    if (it == scheduledControls.end())
    {
        scheduledControls.emplace_back(control, withAnimation, animationTime);
    }
}

void UIScrollSystem::ScrollToScheduledControl(const ScheduledControl& c)
{
    if (c.withAnimation)
    {
        UIControlHelpers::ScrollToControlWithAnimation(c.control.Get(), c.animationTime);
    }
    else
    {
        UIControlHelpers::ScrollToControl(c.control.Get());
    }
}
}
