#include "UIScrollView.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/ScrollHelper.h"
#include "UI/Render/UIClipContentComponent.h"
#include "UI/UIControlHelpers.h"
#include "UI/UIScrollBar.h"
#include "UI/UIScrollViewContainer.h"

namespace DAVA
{
static const FastName UISCROLL_VIEW_CONTAINER_NAME("scrollContainerControl");

DAVA_VIRTUAL_REFLECTION_IMPL(UIScrollView)
{
    ReflectionRegistrator<UIScrollView>::Begin()[M::DisplayName("Scroll View")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIScrollView* o) { o->Release(); })
    .Field("autoUpdate", &UIScrollView::IsAutoUpdate, &UIScrollView::SetAutoUpdate)[M::DisplayName("Auto Update")]
    .Field("centerContent", &UIScrollView::IsCenterContent, &UIScrollView::SetCenterContent)[M::DisplayName("Center Content")]
    .Field("returnSpeed", &UIScrollView::GetReturnSpeed, &UIScrollView::SetReturnSpeed)[M::DisplayName("Return Speed")]
    .Field("scrollSpeed", &UIScrollView::GetScrollSpeed, &UIScrollView::SetScrollSpeed)[M::DisplayName("Scroll Speed")]
    .End();
}

UIScrollView::UIScrollView(const Rect& rect)
    : UIControl(rect)
    , scrollContainer(new UIScrollViewContainer())
    , scrollHorizontal(new ScrollHelper())
    , scrollVertical(new ScrollHelper())
    , autoUpdate(false)
    , centerContent(false)
{
    SetInputEnabled(false, false);
    multiInput = true;

    scrollContainer->SetName(UISCROLL_VIEW_CONTAINER_NAME);
    // Scroll container is a child of ScrollView
    AddControl(scrollContainer);

    GetOrCreateComponent<UIClipContentComponent>();
}

UIScrollView::~UIScrollView()
{
    SafeRelease(scrollContainer);
    SafeRelease(scrollHorizontal);
    SafeRelease(scrollVertical);
}

void UIScrollView::SetRect(const Rect& rect)
{
    UIControl::SetRect(rect);

    scrollHorizontal->SetViewSize(rect.dx);
    scrollVertical->SetViewSize(rect.dy);
}

void UIScrollView::SetSize(const DAVA::Vector2& newSize)
{
    UIControl::SetSize(newSize);

    scrollHorizontal->SetViewSize(newSize.x);
    scrollVertical->SetViewSize(newSize.y);

    if (scrollContainer)
    {
        scrollContainer->ApplySizeChanges();
    }
}

void UIScrollView::AddControl(UIControl* control)
{
    UIControl::AddControl(control);

    if (control->GetName() == UISCROLL_VIEW_CONTAINER_NAME && scrollContainer != control)
    {
        SafeRelease(scrollContainer);
        scrollContainer = SafeRetain(DynamicTypeCheck<UIScrollViewContainer*>(control));
    }
}

void UIScrollView::RemoveControl(UIControl* control)
{
    if (control == scrollContainer)
    {
        SafeRelease(scrollContainer);
    }

    UIControl::RemoveControl(control);
}

void UIScrollView::PushContentToBounds(UIControl* parentControl)
{
    // We have to shift each child of ScrollContent to fit its bounds
    const auto& childslist = parentControl->GetChildren();
    for (auto it = childslist.begin(); it != childslist.end(); ++it)
    {
        UIControl* childControl = it->Get();
        if (!(childControl && childControl->GetVisibilityFlag()))
            continue;

        Rect childRect = childControl->GetRect();

        Vector2 position = GetControlOffset(childControl, Vector2(childRect.x, childRect.y));

        if (position.x < 0)
        {
            childRect.x += Abs(position.x);
        }

        if (position.y < 0)
        {
            childRect.y += Abs(position.y);
        }

        // Move each first child
        if (childRect != childControl->GetRect())
        {
            childControl->SetRect(childRect);
        }
    }
}

Vector2 UIScrollView::GetControlOffset(UIControl* parentControl, Vector2 currentContentOffset)
{
    Vector2 currentOffset = currentContentOffset;
    // Get control's farest position inside scrollContainer
    const auto& childslist = parentControl->GetChildren();
    for (auto it = childslist.begin(); it != childslist.end(); ++it)
    {
        UIControl* childControl = it->Get();
        if (!(childControl && childControl->GetVisibilityFlag()))
            continue;

        Rect childRect = childControl->GetRect();
        float32 controlPosX = currentContentOffset.x + childRect.x;
        float32 controlPosY = currentContentOffset.y + childRect.y;

        Vector2 controlOffset = GetControlOffset(childControl, Vector2(controlPosX, controlPosY));
        currentOffset.x = Min(currentOffset.x, controlOffset.x);
        currentOffset.y = Min(currentOffset.y, controlOffset.y);
    }
    return currentOffset;
}

Vector2 UIScrollView::GetMaxSize(UIControl* parentControl, Vector2 currentMaxSize, Vector2 parentOffset)
{
    // Initial content max size is actual control sizes
    Vector2 maxSize = currentMaxSize;

    const auto& childslist = parentControl->GetChildren();
    for (auto it = childslist.begin(); it != childslist.end(); ++it)
    {
        UIControl* childControl = it->Get();
        if (!(childControl && childControl->GetVisibilityFlag()))
            continue;

        const Rect& childRect = childControl->GetRect();

        // Calculate control full "length" and "height"
        float32 controlSizeX = Abs(parentOffset.x) + childRect.x + childRect.dx;
        float32 controlSizeY = Abs(parentOffset.y) + childRect.y + childRect.dy;
        // Check horizontal size
        if (controlSizeX >= maxSize.x)
        {
            maxSize.x = controlSizeX;
        }
        if (controlSizeY >= maxSize.y)
        {
            maxSize.y = controlSizeY;
        }

        // Change global offset - it has to include parent offset and current child offset
        Vector2 offset;
        offset.x = Abs(parentOffset.x) + childRect.x;
        offset.y = Abs(parentOffset.y) + childRect.y;
        // Move to next child
        maxSize = GetMaxSize(childControl, maxSize, offset);
    }

    return maxSize;
}

UIScrollView* UIScrollView::Clone()
{
    UIScrollView* t = new UIScrollView(GetRect());
    t->CopyDataFrom(this);
    return t;
}

void UIScrollView::CopyDataFrom(UIControl* srcControl)
{
    UIControl::CopyDataFrom(srcControl);
    UIScrollView* src = DynamicTypeCheck<UIScrollView*>(srcControl);
    scrollHorizontal->CopyDataFrom(src->scrollHorizontal);
    scrollVertical->CopyDataFrom(src->scrollVertical);
    autoUpdate = src->autoUpdate;
    centerContent = src->centerContent;
}

void UIScrollView::SetPadding(const Vector2& padding)
{
    if (!scrollHorizontal || !scrollVertical || !scrollContainer)
    {
        return;
    }

    Rect parentRect = GetRect();
    Rect contentRect = scrollContainer->GetRect();

    // Apply scroll offset only if it value don't exceed content size
    if ((Abs(padding.x) + parentRect.dx) <= contentRect.dx)
    {
        contentRect.x = padding.x;
    }

    if ((Abs(padding.y) + parentRect.dy) <= contentRect.dy)
    {
        contentRect.y = padding.y;
    }

    scrollContainer->SetRect(contentRect);
    scrollHorizontal->SetPosition(contentRect.x);
    scrollVertical->SetPosition(contentRect.y);
}

const Vector2 UIScrollView::GetPadding() const
{
    Rect contentRect = scrollContainer ? scrollContainer->GetRect() : Rect();
    return Vector2(contentRect.x, contentRect.y);
}

const Vector2 UIScrollView::GetContentSize() const
{
    Rect contentRect = scrollContainer ? scrollContainer->GetRect() : Rect();
    return Vector2(contentRect.dx, contentRect.dy);
}

void UIScrollView::LoadFromYamlNodeCompleted()
{
}

void UIScrollView::RecalculateContentSize()
{
    if (!scrollHorizontal || !scrollVertical || !scrollContainer)
    {
        return;
    }

    if (autoUpdate)
    {
        DVASSERT(!autoUpdate);
        return;
    }

    Rect contentRect = scrollContainer->GetRect();
    Rect parentRect = GetRect();

    // Move all scrollContainer content with negative positions iside its rect
    PushContentToBounds(scrollContainer);
    // Get max size of content - all childrens
    Vector2 maxSize = GetMaxSize(scrollContainer,
                                 Vector2(parentRect.dx + Abs(contentRect.x), parentRect.dy + Abs(contentRect.y)),
                                 Vector2(0, 0));

    // Update scroll view content size
    scrollContainer->SetRect(Rect(contentRect.x, contentRect.y, maxSize.x, maxSize.y));
    scrollHorizontal->SetElementSize(maxSize.x);
    scrollVertical->SetElementSize(maxSize.y);
}

void UIScrollView::SetReturnSpeed(float32 speedInSeconds)
{
    if (!scrollHorizontal || !scrollVertical)
    {
        return;
    }

    scrollHorizontal->SetBorderMoveModifer(speedInSeconds);
    scrollVertical->SetBorderMoveModifer(speedInSeconds);
}

float32 UIScrollView::GetReturnSpeed() const
{
    if (!scrollHorizontal || !scrollVertical)
    {
        return 0.f;
    }
    return scrollHorizontal->GetBorderMoveModifer();
}

void UIScrollView::SetScrollSpeed(float32 speedInSeconds)
{
    if (!scrollHorizontal || !scrollVertical)
    {
        return;
    }

    scrollHorizontal->SetSlowDownTime(speedInSeconds);
    scrollVertical->SetSlowDownTime(speedInSeconds);
}

float32 UIScrollView::GetScrollSpeed() const
{
    if (!scrollHorizontal || !scrollVertical)
    {
        return 0.f;
    }
    return scrollHorizontal->GetSlowDownTime();
}

float32 UIScrollView::VisibleAreaSize(UIScrollBar* forScrollBar)
{
    if (!forScrollBar || !scrollHorizontal || !scrollVertical)
    {
        return 0.0f;
    }

    // Visible area is our rect.
    Vector2 visibleAreaSize(scrollHorizontal->GetViewSize(), scrollVertical->GetViewSize());
    return GetParameterForScrollBar(forScrollBar, visibleAreaSize);
}

float32 UIScrollView::TotalAreaSize(UIScrollBar* forScrollBar)
{
    if (!forScrollBar || !scrollHorizontal || !scrollVertical)
    {
        return 0.0f;
    }

    // Total area is the rect of the container.
    Vector2 totalAreaSize(scrollHorizontal->GetElementSize(), scrollVertical->GetElementSize());
    return GetParameterForScrollBar(forScrollBar, totalAreaSize);
}

float32 UIScrollView::ViewPosition(UIScrollBar* forScrollBar)
{
    if (!forScrollBar || !scrollHorizontal || !scrollVertical)
    {
        return 0.0f;
    }

    // View position is the position of the scroll container in relation to us.
    Vector2 viewPosition = Vector2(scrollHorizontal->GetPosition(), scrollVertical->GetPosition());
    return GetParameterForScrollBar(forScrollBar, viewPosition);
}

void UIScrollView::OnViewPositionChanged(UIScrollBar* byScrollBar, float32 newPosition)
{
    if (!byScrollBar || !scrollContainer || !scrollHorizontal || !scrollVertical)
    {
        return;
    }

    Rect curContainerRect = scrollContainer->GetRect();
    if (byScrollBar->GetOrientation() == UIScrollBar::ORIENTATION_HORIZONTAL)
    {
        curContainerRect.x = -newPosition;
    }
    else if (byScrollBar->GetOrientation() == UIScrollBar::ORIENTATION_VERTICAL)
    {
        curContainerRect.y = -newPosition;
    }

    scrollHorizontal->SetPosition(curContainerRect.x);
    scrollVertical->SetPosition(curContainerRect.y);
    scrollContainer->SetRect(curContainerRect);
}

void UIScrollView::OnScrollViewContainerSizeChanged()
{
    if (autoUpdate && scrollContainer != nullptr)
    {
        scrollHorizontal->SetElementSize(scrollContainer->GetSize().dx);
        scrollVertical->SetElementSize(scrollContainer->GetSize().dy);
    }
}

float32 UIScrollView::GetParameterForScrollBar(UIScrollBar* forScrollBar, const Vector2& vectorParam)
{
    if (forScrollBar->GetOrientation() == UIScrollBar::ORIENTATION_HORIZONTAL)
    {
        return vectorParam.x;
    }
    else if (forScrollBar->GetOrientation() == UIScrollBar::ORIENTATION_VERTICAL)
    {
        return vectorParam.y;
    }

    DVASSERT(false, "Unknown orientation!");
    return 0.0f;
}

void UIScrollView::AddControlToContainer(UIControl* control)
{
    if (scrollContainer)
    {
        scrollContainer->AddControl(control);
    }
}

UIScrollViewContainer* UIScrollView::GetContainer()
{
    return scrollContainer;
}

ScrollHelper* UIScrollView::GetHorizontalScroll()
{
    return scrollHorizontal;
}

ScrollHelper* UIScrollView::GetVerticalScroll()
{
    return scrollVertical;
}

float32 UIScrollView::GetHorizontalScrollPosition() const
{
    if (scrollHorizontal)
    {
        return scrollHorizontal->GetPosition();
    }

    return 0.0f;
}

float32 UIScrollView::GetVerticalScrollPosition() const
{
    if (scrollVertical)
    {
        return scrollVertical->GetPosition();
    }

    return 0.0f;
}

Vector2 UIScrollView::GetScrollPosition() const
{
    return Vector2(GetHorizontalScrollPosition(), GetVerticalScrollPosition());
}

void UIScrollView::SetHorizontalScrollPosition(float32 horzPos)
{
    if (!scrollContainer || !scrollHorizontal)
    {
        return;
    }

    Vector2 pos = scrollContainer->GetPosition();
    pos.x = horzPos;
    if (scrollContainer->GetPosition() != pos)
    {
        scrollContainer->SetPosition(pos);
    }

    scrollHorizontal->SetPosition(horzPos);
}

void UIScrollView::SetVerticalScrollPosition(float32 vertPos)
{
    if (!scrollContainer || !scrollVertical)
    {
        return;
    }

    Vector2 pos = scrollContainer->GetPosition();
    pos.y = vertPos;
    if (scrollContainer->GetPosition() != pos)
    {
        scrollContainer->SetPosition(pos);
    }

    scrollVertical->SetPosition(vertPos);
}

void UIScrollView::SetScrollPosition(const Vector2& pos)
{
    SetHorizontalScrollPosition(pos.x);
    SetVerticalScrollPosition(pos.y);
}

void UIScrollView::ScrollToHorizontalPosition(float32 horzPos, float32 timeSec)
{
    scrollHorizontal->ScrollToPosition(horzPos, timeSec);
}

void UIScrollView::ScrollToVerticalPosition(float32 vertPos, float32 timeSec)
{
    scrollVertical->ScrollToPosition(vertPos, timeSec);
}

void UIScrollView::ScrollToPosition(const Vector2& pos, float32 timeSec)
{
    scrollHorizontal->ScrollToPosition(pos.x, timeSec);
    scrollVertical->ScrollToPosition(pos.y, timeSec);
}

bool UIScrollView::IsAutoUpdate() const
{
    return autoUpdate;
}

void UIScrollView::SetAutoUpdate(bool auto_)
{
    autoUpdate = auto_;
}

bool UIScrollView::IsCenterContent() const
{
    return centerContent;
}

void UIScrollView::SetCenterContent(bool center_)
{
    centerContent = center_;
}
};
