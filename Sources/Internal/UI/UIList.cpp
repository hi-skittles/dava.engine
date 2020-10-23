#include "UI/UIList.h"
#include "Base/Message.h"
#include "Base/ObjectFactory.h"
#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Time/SystemTimer.h"
#include "UI/Layouts/UISizePolicyComponent.h"
#include "UI/Render/UIClipContentComponent.h"
#include "UI/ScrollHelper.h"
#include "UI/UIControlHelpers.h"
#include "UI/UIControlSystem.h"
#include "UI/UIListCell.h"
#include "UI/UIListDelegate.h"
#include "UI/Update/UIUpdateComponent.h"

namespace DAVA
{
static const int32 INVALID_INDEX = -1;

DAVA_VIRTUAL_REFLECTION_IMPL(UIList)
{
    ReflectionRegistrator<UIList>::Begin()[M::DisplayName("List")]
    .ConstructorByPointer()
    .DestructorByPointer([](UIList* o) { o->Release(); })
    .Field("orientation", &UIList::GetOrientation, &UIList::SetOrientation)[M::EnumT<eListOrientation>(), M::DisplayName("Orientation")]
    .Field("slowDownTime", &UIList::GetSlowDownTime, &UIList::SetSlowDownTime)[M::DisplayName("Slow Down Time")]
    .Field("borderMoveModifer", &UIList::GetBorderMoveModifer, &UIList::SetBorderMoveModifer)[M::DisplayName("Border Move Modifier")]
    .Field("touchHoldDelta", &UIList::GetTouchHoldDelta, &UIList::SetTouchHoldDelta)[M::DisplayName("Touch Hold Delta")]
    .End();
}

UIList::UIList(const Rect& rect /* = Rect()*/, eListOrientation requiredOrientation /* = ORIENTATION_VERTICAL*/)
    : UIControl(rect)
    , delegate(NULL)
    , orientation(requiredOrientation)
    , scrollContainer(NULL)
    , scroll(NULL)
{
    InitAfterYaml();
    GetOrCreateComponent<UIUpdateComponent>();
    GetOrCreateComponent<UIClipContentComponent>();
}

void UIList::InitAfterYaml()
{
    SetInputEnabled(true, false);
    Rect r = GetRect();
    r.x = 0;
    r.y = 0;

    // InitAfterYaml might be called multiple times - check this and remove previous scroll, if yes.
    if (scrollContainer)
    {
        RemoveControl(scrollContainer);
        SafeRelease(scrollContainer);
    }

    scrollContainer = new UIControl(r);

    UISizePolicyComponent* spc = scrollContainer->GetOrCreateComponent<UISizePolicyComponent>();
    spc->SetHorizontalPolicy(UISizePolicyComponent::eSizePolicy::PERCENT_OF_PARENT);
    spc->SetHorizontalValue(100.f);
    spc->SetVerticalPolicy(UISizePolicyComponent::eSizePolicy::PERCENT_OF_PARENT);
    spc->SetVerticalValue(100.f);

    AddControl(scrollContainer);

    oldPos = 0;
    newPos = 0;

    mainTouch = -1;

    touchHoldSize = 15;

    lockTouch = false;

    needRefresh = false;

    if (scroll == NULL)
    {
        scroll = new ScrollHelper();
    }
}

UIList::~UIList()
{
    SafeRelease(scrollContainer);
    SafeRelease(scroll);

    ClearReusableCells();
}

void UIList::ScrollTo(float delta)
{
    scroll->Impulse(delta * -4.8f);
}

void UIList::SetSize(const Vector2& newSize)
{
    if (orientation == ORIENTATION_HORIZONTAL)
    {
        scroll->SetViewSize(newSize.dx);
    }
    else
    {
        scroll->SetViewSize(newSize.dy);
    }

    UIControl::SetSize(newSize);
}

void UIList::SetDelegate(UIListDelegate* newDelegate)
{
    delegate = newDelegate;
    Refresh();
}

UIListDelegate* UIList::GetDelegate()
{
    return delegate;
}

void UIList::ScrollToElement(int32 index)
{
    DVASSERT(delegate);
    DVASSERT(0 <= index && index < delegate->ElementsCount(this));
    float32 newScrollPos = 0.0f;
    if (orientation == ORIENTATION_HORIZONTAL)
    {
        for (int32 i = 0; i < index; ++i)
        {
            newScrollPos += delegate->CellWidth(this, i);
        }
    }
    else
    {
        for (int32 i = 0; i < index; ++i)
        {
            newScrollPos += delegate->CellHeight(this, i);
        }
    }
    SetScrollPosition(newScrollPos);
}

void UIList::SetOrientation(int32 _orientation)
{
    orientation = static_cast<UIList::eListOrientation>(_orientation);
}

float32 UIList::GetScrollPosition()
{
    return scroll->GetPosition();
}

void UIList::SetScrollPosition(float32 newScrollPos)
{
    if (needRefresh)
    {
        FullRefresh();
    }

    scroll->SetPosition(-newScrollPos);
}

void UIList::ResetScrollPosition()
{
    if (orientation == ORIENTATION_HORIZONTAL)
    {
        scrollContainer->SetPosition(Vector2(0.f, scrollContainer->GetPosition().y));
        scroll->SetPosition(0);
    }
    else
    {
        scrollContainer->SetPosition(Vector2(scrollContainer->GetPosition().x, 0.f));
        scroll->SetPosition(0);
    }
}

void UIList::FullRefresh()
{
    needRefresh = false;

    RemoveAllCells();
    if (!delegate)
    {
        return;
    }

    addPos = 0.0f;
    float32 scrollAdd;
    float32 maxSize;
    if (orientation == ORIENTATION_HORIZONTAL)
    {
        scrollAdd = scrollContainer->GetRect().x;
        maxSize = GetRect().dx;
    }
    else
    {
        scrollAdd = scrollContainer->GetRect().y;
        maxSize = GetRect().dy;
    }

    scroll->SetViewSize(maxSize);

    float32 sz = 0.0f;
    int32 elCnt = delegate->ElementsCount(this);
    int32 index = 0;
    for (; index < elCnt; index++)
    {
        float32 curPos = addPos + scrollAdd;
        float32 size = 0.0f;
        if (orientation == ORIENTATION_HORIZONTAL)
        {
            size = delegate->CellWidth(this, index);
        }
        else
        {
            size = delegate->CellHeight(this, index);
        }

        sz += size;
        if (curPos + size > -maxSize)
        {
            AddCellAtPos(delegate->CellAtIndex(this, index), addPos, size, index);
        }

        addPos += size;
        if (addPos + scrollAdd > maxSize * 2.0f)
        {
            break;
        }
    }

    index++;
    for (; index < elCnt; index++)
    {
        if (orientation == ORIENTATION_HORIZONTAL)
        {
            sz += delegate->CellWidth(this, index);
        }
        else
        {
            sz += delegate->CellHeight(this, index);
        }
    }

    scroll->SetElementSize(sz);
}

void UIList::Refresh()
{
    needRefresh = true;
}

void UIList::Update(float32 timeElapsed)
{
    if (needRefresh)
    {
        FullRefresh();
    }

    if (!delegate)
    {
        return;
    }

    float32 d = newPos - oldPos;
    oldPos = newPos;

    float32 deltaWheel = newScroll - oldScroll;
    oldScroll = newScroll;

    const float32 accuracyDelta = 0.1f;

    Rect r = scrollContainer->GetRect();

    if (accuracyDelta <= Abs(deltaWheel))
    {
        // this code works for mouse or touchpad scrolls
        if (orientation == ORIENTATION_HORIZONTAL)
        {
            scroll->ScrollWithoutAnimation(deltaWheel, r.dx, &r.x);
        }
        else
        {
            scroll->ScrollWithoutAnimation(deltaWheel, r.dy, &r.y);
        }
    }
    else
    {
        // this code works for scroll through touch screen.
        if (orientation == ORIENTATION_HORIZONTAL)
        {
            r.x = scroll->GetPosition(d, SystemTimer::GetFrameDelta(), lockTouch);
        }
        else
        {
            r.y = scroll->GetPosition(d, SystemTimer::GetFrameDelta(), lockTouch);
        }
    }

    if (r.x != scrollContainer->GetRect().x || r.y != scrollContainer->GetRect().y)
    {
        scrollContainer->SetPosition(r.GetPosition());
    }

    Rect viewRect = GetGeometricData().GetUnrotatedRect();
    const auto& scrollList = scrollContainer->GetChildren();
    List<UIListCell*> removeList;

    //removing invisible elements
    for (auto it = scrollList.begin(); it != scrollList.end(); it++)
    {
        UIListCell* cell = DynamicTypeCheck<UIListCell*>(it->Get());

        Rect crect = cell->GetGeometricData().GetUnrotatedRect();
        if (orientation == ORIENTATION_HORIZONTAL)
        {
            if (crect.x + crect.dx < viewRect.x - viewRect.dx || crect.x > viewRect.x + viewRect.dx * 2)
            {
                removeList.push_back(cell);
            }
        }
        else
        {
            if (crect.y + crect.dy < viewRect.y - viewRect.dy || crect.y > viewRect.y + viewRect.dy * 2)
            {
                removeList.push_back(cell);
            }
        }
    }
    for (UIListCell* cell : removeList)
    {
        RemoveCell(cell);
    }

    if (!scrollList.empty())
    {
        //adding elements at the list end
        int32 ind = -1;
        UIListCell* fc = NULL;
        for (auto it = scrollList.begin(); it != scrollList.end(); it++)
        {
            UIListCell* lc = static_cast<UIListCell*>(it->Get());
            int32 i = lc->GetIndex();
            if (i > ind)
            {
                ind = i;
                fc = lc;
            }
        }
        if (fc)
        {
            float32 borderPos;
            float32 rPos;
            float32 size = 0.0f;
            float32 off;
            if (orientation == ORIENTATION_HORIZONTAL)
            {
                borderPos = viewRect.dx + viewRect.dx / 2.0f;
                off = scrollContainer->GetRect().x;
                rPos = fc->GetRect().x + fc->GetRect().dx + off;
            }
            else
            {
                borderPos = viewRect.dy + viewRect.dy / 2.0f;
                off = scrollContainer->GetRect().y;
                rPos = fc->GetRect().y + fc->GetRect().dy + off;
            }

            int32 elementsCount = delegate->ElementsCount(this);
            while (rPos < borderPos && fc->GetIndex() < elementsCount - 1)
            {
                int32 i = fc->GetIndex() + 1;
                fc = delegate->CellAtIndex(this, i);
                if (orientation == ORIENTATION_HORIZONTAL)
                {
                    size = delegate->CellWidth(this, i);
                }
                else
                {
                    size = delegate->CellHeight(this, i);
                }
                AddCellAtPos(fc, rPos - off, size, i);
                rPos += size;
                //			scroll->SetElementSize((float32)(rPos - off));
            }
        }

        //adding elements at the list begin
        ind = maximumElementsCount;
        fc = NULL;
        for (auto it = scrollList.begin(); it != scrollList.end(); it++)
        {
            UIListCell* lc = static_cast<UIListCell*>(it->Get());
            int32 i = lc->GetIndex();
            if (i < ind)
            {
                ind = i;
                fc = lc;
            }
        }
        if (fc)
        {
            float32 borderPos;
            float32 rPos;
            float32 size = 0.0f;
            float32 off;
            if (orientation == ORIENTATION_HORIZONTAL)
            {
                borderPos = -viewRect.dx / 2.0f;
                off = scrollContainer->GetRect().x;
                rPos = fc->GetRect().x + off;
            }
            else
            {
                borderPos = -viewRect.dy / 2.0f;
                off = scrollContainer->GetRect().y;
                rPos = fc->GetRect().y + off;
            }
            while (rPos > borderPos && fc->GetIndex() > 0)
            {
                int32 i = fc->GetIndex() - 1;
                fc = delegate->CellAtIndex(this, i);
                if (orientation == ORIENTATION_HORIZONTAL)
                {
                    size = delegate->CellWidth(this, i);
                }
                else
                {
                    size = delegate->CellHeight(this, i);
                }
                rPos -= size;
                AddCellAtPos(fc, rPos - off, size, i);
            }
        }
    }
    else
    {
        FullRefresh();
    }
}

void UIList::Input(UIEvent* currentInput)
{
    if (lockTouch && currentInput->touchId != mainTouch)
    {
        // Ignore any other touches when the input is locked.
        currentInput->SetInputHandledType(UIEvent::INPUT_HANDLED_HARD);
        return;
    }

    if (UIEvent::Phase::WHEEL == currentInput->phase)
    {
        if (eInputDevices::MOUSE == currentInput->device)
        {
            // In horizontal list also work horizontal wheel
            if (!FLOAT_EQUAL(currentInput->wheelDelta.x, 0.f) && ORIENTATION_HORIZONTAL == orientation)
            {
                newScroll += currentInput->wheelDelta.x * GetWheelSensitivity();
            }
            else
            {
                newScroll += currentInput->wheelDelta.y * GetWheelSensitivity();
            }
        }
        else // eInputDevices::TOUCH_PAD
        {
            if (ORIENTATION_HORIZONTAL == orientation)
            {
                newScroll += currentInput->wheelDelta.x * GetWheelSensitivity();
            }
            else
            {
                newScroll += currentInput->wheelDelta.y * GetWheelSensitivity();
            }
        }
    }
    else
    {
        if (orientation == ORIENTATION_HORIZONTAL)
        {
            newPos = currentInput->point.x;
        }
        else
        {
            newPos = currentInput->point.y;
        }
    }

    switch (currentInput->phase)
    {
    case UIEvent::Phase::BEGAN:
    {
        lockTouch = true;
        oldPos = newPos;
        mainTouch = currentInput->touchId;
    }
    break;
    case UIEvent::Phase::DRAG:
    {
    }
    break;
    case UIEvent::Phase::ENDED:
    case UIEvent::Phase::CANCELLED:
    {
        lockTouch = false;
        mainTouch = -1;
    }
    break;
    default:
        break;
    }

    currentInput->SetInputHandledType(UIEvent::INPUT_HANDLED_HARD); // Drag is handled - see please DF-2508.
}

bool UIList::SystemInput(UIEvent* currentInput)
{
    if (!GetInputEnabled() || !visible || GetState() & STATE_DISABLED)
    {
        return false;
    }

    if (currentInput->touchLocker != this)
    {
        if (UIEvent::Phase::WHEEL == currentInput->phase)
        {
            if (IsPointInside(currentInput->point))
            {
                Input(currentInput);
                return true;
            }
        }
        else if (currentInput->phase == UIEvent::Phase::BEGAN)
        {
            if (IsPointInside(currentInput->point))
            {
                PerformEvent(EVENT_TOUCH_DOWN, currentInput);
                Input(currentInput);
            }
        }
        else if (currentInput->touchId == mainTouch && currentInput->phase == UIEvent::Phase::DRAG)
        {
            if (orientation == ORIENTATION_HORIZONTAL)
            {
                if (Abs(currentInput->point.x - newPos) > touchHoldSize)
                {
                    GetEngineContext()->uiControlSystem->SwitchInputToControl(mainTouch, this);
                    newPos = currentInput->point.x;
                    return true;
                }
            }
            else
            {
                if (Abs(currentInput->point.y - newPos) > touchHoldSize)
                {
                    GetEngineContext()->uiControlSystem->SwitchInputToControl(mainTouch, this);
                    newPos = currentInput->point.y;
                    return true;
                }
            }
        }
        else if (currentInput->touchId == mainTouch && (currentInput->phase == UIEvent::Phase::ENDED || currentInput->phase == UIEvent::Phase::CANCELLED))
        {
            mainTouch = -1;
            lockTouch = false;
            return UIControl::SystemInput(currentInput);
        }
    }

    return UIControl::SystemInput(currentInput);
}

void UIList::InputCancelled(UIEvent* currentInput)
{
    lockTouch = false;
    mainTouch = -1;

    UIControl::InputCancelled(currentInput);
}

void UIList::OnSelectEvent(BaseObject* pCaller, void* pUserData, void* callerData)
{
    if (delegate)
    {
        delegate->OnCellSelected(this, static_cast<UIListCell*>(pCaller));
    }
}

void UIList::RemoveCell(UIListCell* cell)
{
    DVASSERT(cell->cellStore == this);
    DVASSERT(cell->GetParent() == scrollContainer);
    scrollContainer->RemoveControl(cell);
    cell->SetIndex(INVALID_INDEX);
}

void UIList::RemoveAllCells()
{
    scrollContainer->RemoveAllControls();
    for (const auto& mapPair : cellStore)
    {
        for (UIListCell* cell : *mapPair.second)
        {
            cell->SetIndex(INVALID_INDEX);
        }
    }
}

void UIList::AddCellAtPos(UIListCell* cell, float32 pos, float32 size, int32 index)
{
    DVASSERT(cell);
    DVASSERT(cell->cellStore == NULL || cell->cellStore == this);
    DVASSERT(index >= 0);
    if (!cell->cellStore)
    {
        cell->cellStore = this;
        cell->AddEvent(EVENT_TOUCH_UP_INSIDE, Message(this, &UIList::OnSelectEvent));
        Vector<UIListCell*>* store = GetStoreVector(cell->identifier);
        if (!store)
        {
            store = new Vector<UIListCell*>;
            cellStore[cell->identifier] = store;
        }
        store->push_back(cell);
    }
    cell->SetIndex(index);
    Rect r = cell->GetRect();
    if (orientation == ORIENTATION_HORIZONTAL)
    {
        r.dx = size;
        r.x = pos;
    }
    else
    {
        r.dy = size;
        r.y = pos;
    }
    cell->SetRect(r);

    scrollContainer->AddControl(cell);
}

Vector<UIListCell*>* UIList::GetStoreVector(const String& cellIdentifier)
{
    Map<String, Vector<UIListCell*>*>::const_iterator mit;
    mit = cellStore.find(cellIdentifier);
    if (mit == cellStore.end())
    {
        return NULL;
    }

    return mit->second;
}

UIListCell* UIList::GetReusableCell(const String& cellIdentifier)
{
    Vector<UIListCell*>* store = GetStoreVector(cellIdentifier);
    if (!store)
    {
        return NULL;
    }

    for (Vector<UIListCell*>::iterator it = store->begin(); it != store->end(); it++)
    {
        if ((*it)->GetIndex() == INVALID_INDEX)
        {
            return (*it);
        }
    }

    return NULL;
}

const List<RefPtr<UIControl>>& UIList::GetVisibleCells() const
{
    return scrollContainer->GetChildren();
}

void UIList::SetTouchHoldDelta(int32 holdDelta)
{
    touchHoldSize = holdDelta;
}

int32 UIList::GetTouchHoldDelta() const
{
    return touchHoldSize;
}

void UIList::SetSlowDownTime(float32 newValue)
{
    scroll->SetSlowDownTime(newValue);
}

float32 UIList::GetSlowDownTime() const
{
    return scroll->GetSlowDownTime();
}

void UIList::SetBorderMoveModifer(float32 newValue)
{
    scroll->SetBorderMoveModifer(newValue);
}

float32 UIList::GetBorderMoveModifer() const
{
    return scroll->GetBorderMoveModifer();
}

void UIList::OnActive()
{
    UIControl::OnActive();
    Refresh();
}

UIList* UIList::Clone()
{
    UIList* c = new UIList(GetRect(), this->orientation);
    c->CopyDataFrom(this);
    return c;
}

void UIList::CopyDataFrom(UIControl* srcControl)
{
    UIControl::CopyDataFrom(srcControl);
    UIList* t = static_cast<UIList*>(srcControl);
    InitAfterYaml();
    orientation = t->orientation;
}

float32 UIList::VisibleAreaSize(UIScrollBar* forScrollBar)
{
    return scroll->GetViewSize();
}

float32 UIList::TotalAreaSize(UIScrollBar* forScrollBar)
{
    return scroll->GetElementSize();
}

float32 UIList::ViewPosition(UIScrollBar* forScrollBar)
{
    return scroll->GetPosition();
}

void UIList::OnViewPositionChanged(UIScrollBar* byScrollBar, float32 newPosition)
{
    scroll->SetPosition(-newPosition);
}

void UIList::ScrollToPosition(float32 position, float32 timeSec /*= 0.3f*/)
{
    scroll->ScrollToPosition(-position);
}

void UIList::ImmediateClearCells()
{
    ClearReusableCells();
    RemoveAllCells();
    Refresh();
}

void UIList::ClearReusableCells()
{
    for (const std::pair<String, Vector<UIListCell*>*>& mit : cellStore)
    {
        for (UIListCell* cell : *(mit.second))
        {
            cell->cellStore = nullptr;
            cell->Release();
        }
        delete mit.second;
    }
    cellStore.clear();
}
};
