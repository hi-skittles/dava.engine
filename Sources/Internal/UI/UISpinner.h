#ifndef __DAVAENGINE_UI_SPINNER_H__
#define __DAVAENGINE_UI_SPINNER_H__

#include "Base/BaseTypes.h"
#include "UI/UIControl.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class UISpinner;

/*
 * Provider of sequential data for UISpinner. Data set it contains can be bounded, cyclic or infinite.
 * 
 * Methods related to selection or dataset change (Next()/Previous() and those from specific implementations) can be called from any place, 
 * that's why UISpinner should use notification mechanism to stay in sync with adapter.
 */
class SpinnerAdapter : public BaseObject
{
public:
    enum eItemOrder
    {
        EIO_PREVIOUS = -1,
        EIO_CURRENT = 0,
        EIO_NEXT = 1
    };

    class SelectionObserver
    {
    public:
        virtual ~SelectionObserver() = default;
        /*
         * If isSelectedChanged == true selected element was actually changed.
         *
         * Call with isSelectedChanged == false is possible for changeable dataset when selected 
         * element becomes first/last/not first/not last as a result of data set change.
         */
        virtual void OnSelectedChanged(bool isSelectedFirst, bool isSelectedLast, bool isSelectedChanged) = 0;
    };

protected:
    virtual ~SpinnerAdapter() = default;

public:
    /*
     * This method actually displays selected element in a manner specific for particular control (it can be text or image or whatever).
     *
     * Default implementation assumes that spinner has only scrollable content, so it just calls FillScrollableContent() with order == CURRENT.
     * If your spinner has some selected-item-dependant content outside scrollable area (e.g. text description of selected item) override this method to update this content.
     */
    virtual void DisplaySelectedData(UISpinner* spinner);

    /*
     * This method displays scrollable content of current/previous/next item.
     * Implementation depends on a kind of controls used to display item data: use lookup by name to find controls inside the 'scrollableContent' and fill them with data.
     */
    virtual void FillScrollableContent(UIControl* scrollableContent, eItemOrder order) = 0;

    /*
     * Select next element. Returns 'true' and calls OnSelectedChanged for all observers if next element selected successfully. Returns 'false' otherwise.
     */
    bool Next();

    /*
     * Select previous element. Returns 'true' and calls OnSelectedChanged for all observers if previous element selected successfully. Returns 'false' otherwise.
     */
    bool Previous();

    //For next two implementation depends on a type of data set, not on a kind of controls used to display data element.
    virtual bool IsSelectedLast() const = 0;
    virtual bool IsSelectedFirst() const = 0;

    void AddObserver(SelectionObserver* anObserver);
    void RemoveObserver(SelectionObserver* anObserver);

protected:
    Set<SelectionObserver*> observers;

    //For next two implementation depends on a type of data set, not on a kind of controls used to display data element.
    //See description for Next() and Previous()
    virtual bool SelectNext() = 0;
    virtual bool SelectPrevious() = 0;

    void NotifyObservers(bool isSelectedFirst, bool isSelectedLast, bool isSelectedChanged);
};

/*
 * UISpinner has two buttons to select next and previous element from some dataset.
 * To display selected element UISpinner itself (as UIControl) or its children should be used:
 * use SpinnerAdapter with your custom DisplaySelectedData implementation of display logic.
 */
class UISpinner : public UIControl, SpinnerAdapter::SelectionObserver
{
    DAVA_VIRTUAL_REFLECTION(UISpinner, UIControl);

public:
    UISpinner(const Rect& rect = Rect());

protected:
    virtual ~UISpinner();

public:
    UISpinner* Clone() override;
    void CopyDataFrom(UIControl* srcControl) override;

    void AddControl(UIControl* control) override;
    void RemoveControl(UIControl* control) override;

    void Input(UIEvent* currentInput) override;
    void Update(float32 timeElapsed) override;

    void LoadFromYamlNodeCompleted() override;

    SpinnerAdapter* GetAdater() const
    {
        return adapter;
    }
    void SetAdapter(SpinnerAdapter* adapter);

    UIControl* GetButtonNext() const
    {
        return buttonNext.Get();
    }
    UIControl* GetButtonPrevious() const
    {
        return buttonPrevious.Get();
    }
    UIControl* GetContent() const
    {
        return content.Get();
    }

protected:
    struct Move
    {
        float32 dx;
        float32 time;
    };

    SpinnerAdapter* adapter;

    RefPtr<UIControl> buttonNext;
    RefPtr<UIControl> buttonPrevious;

    //we need these 'content' controls to scroll items with slide
    RefPtr<UIControl> content;
    //internal controls, need only for animation
    RefPtr<UIControl> nextContent;
    RefPtr<UIControl> contentViewport; //area that clips items when we scroll them

    float32 dragAnchorX;

    //these are for quick short slide gesure recognition
    float32 previousTouchX;
    float32 currentTouchX;
    float32 totalGestureTime;
    float32 totalGestureDx;
    List<Move> moves;

    void OnNextPressed(BaseObject* caller, void* param, void* callerData);
    void OnPreviousPressed(BaseObject* caller, void* param, void* callerData);
    void OnScrollAnimationEnd(BaseObject* caller, void* param, void* callerData);

    void OnSelectedChanged(bool isSelectedFirst, bool isSelectedLast, bool isSelectedChanged) override;

    void OnSelectWithSlide(bool isPrevious);
    void SetupInternalControls();
};
}
#endif //__DAVAENGINE_UI_SPINNER_H__