#ifndef __DAVAENGINE_UISCROLLVIEW_H__
#define __DAVAENGINE_UISCROLLVIEW_H__

#include "Base/BaseTypes.h"
#include "UI/UIControl.h"
#include "UI/UIScrollBarDelegate.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UIScrollViewContainer;
class ScrollHelper;

class UIScrollView : public UIControl, public UIScrollBarDelegate
{
    DAVA_VIRTUAL_REFLECTION(UIScrollView, UIControl);

public:
    UIScrollView(const Rect& rect = Rect());

protected:
    virtual ~UIScrollView();

public:
    void AddControl(UIControl* control) override;
    void RemoveControl(UIControl* control) override;

    // Add the control directly to the Scroll View Container.
    void AddControlToContainer(UIControl* control);

    // Access to the Scroll View Container.
    UIScrollViewContainer* GetContainer();
    ScrollHelper* GetHorizontalScroll();
    ScrollHelper* GetVerticalScroll();

    // Scroll Position getter/setters.
    float32 GetHorizontalScrollPosition() const;
    float32 GetVerticalScrollPosition() const;
    Vector2 GetScrollPosition() const;

    void SetHorizontalScrollPosition(float32 horzPos);
    void SetVerticalScrollPosition(float32 vertPos);
    void SetScrollPosition(const Vector2& pos);

    void ScrollToHorizontalPosition(float32 horzPos, float32 timeSec = 0.3f);
    void ScrollToVerticalPosition(float32 vertPos, float32 timeSec = 0.3f);
    void ScrollToPosition(const Vector2& pos, float32 timeSec = 0.3f);

    UIScrollView* Clone() override;
    void CopyDataFrom(UIControl* srcControl) override;

    void SetRect(const Rect& rect) override;
    void SetSize(const Vector2& newSize) override;

    void SetPadding(const Vector2& padding);
    const Vector2 GetPadding() const;

    const Vector2 GetContentSize() const;

    void RecalculateContentSize();

    //Sets how fast scroll container will return to its bounds
    void SetReturnSpeed(float32 speedInSeconds);

    //Returns how fast scroll container will return to its bounds
    float32 GetReturnSpeed() const;

    //Sets how fast scroll speed will be reduced
    void SetScrollSpeed(float32 speedInSeconds);

    //Returns how fast scroll speed will be reduced
    float32 GetScrollSpeed() const;

    // UIScrollBarDelegate implementation.
    float32 VisibleAreaSize(UIScrollBar* forScrollBar) override;
    float32 TotalAreaSize(UIScrollBar* forScrollBar) override;
    float32 ViewPosition(UIScrollBar* forScrollBar) override;
    void OnViewPositionChanged(UIScrollBar* byScrollBar, float32 newPosition) override;
    void OnScrollViewContainerSizeChanged();

    bool IsAutoUpdate() const;
    void SetAutoUpdate(bool auto_);

    bool IsCenterContent() const;
    void SetCenterContent(bool center_);

protected:
    void LoadFromYamlNodeCompleted() override;

    Vector2 GetMaxSize(UIControl* control, Vector2 currentMaxSize, Vector2 parentShift);
    void PushContentToBounds(UIControl* control);
    Vector2 GetControlOffset(UIControl* control, Vector2 currentContentOffset);

    // Get the X or Y parameter from the vector depending on the scrollbar orientation.
    float32 GetParameterForScrollBar(UIScrollBar* forScrollBar, const Vector2& vectorParam);

    UIScrollViewContainer* scrollContainer;
    ScrollHelper* scrollHorizontal;
    ScrollHelper* scrollVertical;

    bool autoUpdate;
    bool centerContent;

private:
    void FindRequiredControls();
};
};

#endif //__DAVAENGINE_UISCROLLVIEW__
