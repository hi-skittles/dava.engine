#ifndef __DAVAENGINE_UI_SCROLL_BAR_H__
#define __DAVAENGINE_UI_SCROLL_BAR_H__

#include "Base/BaseTypes.h"
#include "UI/UIControl.h"
#include "Reflection/Reflection.h"

#define MINIMUM_SLIDER_SIZE 30

namespace DAVA
{
class UIScrollBarDelegate;

class UIScrollBar : public UIControl
{ //TODO: add top and bottom buttons
    DAVA_VIRTUAL_REFLECTION(UIScrollBar, UIControl);

public:
    enum eScrollOrientation
    {
        ORIENTATION_VERTICAL = 0
        ,
        ORIENTATION_HORIZONTAL
    };

protected:
    virtual ~UIScrollBar();

public:
    UIScrollBar(const Rect& rect = Rect(), eScrollOrientation requiredOrientation = ORIENTATION_VERTICAL);

    void SetDelegate(UIScrollBarDelegate* newDelegate);
    UIControl* GetSlider();

    void Draw(const UIGeometricData& geometricData) override;
    void AddControl(UIControl* control) override;
    void RemoveControl(UIControl* control) override;
    UIScrollBar* Clone() override;
    void CopyDataFrom(UIControl* srcControl) override;

    void LoadFromYamlNodeCompleted() override;

    void Input(UIEvent* currentInput) override;

    int32 GetOrientation() const;
    void SetOrientation(int32 value);

protected:
    // Calculate the start offset based on the initial click point.
    void CalculateStartOffset(const Vector2& inputPoint);
    void InitControls(const Rect& rect = Rect());

private:
    eScrollOrientation orientation;
    UIScrollBarDelegate* delegate;

    UIControl* slider;

    bool resizeSliderProportionally;

    Vector2 startPoint;
    Vector2 startOffset;

    float32 GetValidSliderSize(float32 size);
};
};



#endif
