#include "UIControl.h"

#include "Animation/AnimationManager.h"
#include "Animation/LinearAnimation.h"
#include "Base/RefPtrUtils.h"
#include "Concurrency/LockGuard.h"
#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "Entity/ComponentManager.h"
#include "Logger/Logger.h"
#include "Reflection/ReflectionRegistrator.h"
#include "UI/Components/UIComponent.h"
#include "UI/Components/UIControlFamily.h"
#include "UI/Focus/FocusHelpers.h"
#include "UI/Layouts/UILayoutSystem.h"
#include "UI/Render/UIClipContentComponent.h"
#include "UI/Render/UIRenderSystem.h"
#include "UI/Styles/UIStyleSheetSystem.h"
#include "UI/UIAnalytics.h"
#include "UI/UIControlBackground.h"
#include "UI/UIControlHelpers.h"
#include "UI/UIControlPackageContext.h"
#include "UI/UIControlSystem.h"
#include "UI/Update/UIUpdateComponent.h"
#include "Utils/StringFormat.h"

#ifdef __DAVAENGINE_AUTOTESTING__
#include "Autotesting/AutotestingSystem.h"
#endif

namespace DAVA
{
const char* UIControl::STATE_NAMES[] = { "normal", "pressed_outside", "pressed_inside", "disabled", "selected", "hover", "focused" };

DAVA_VIRTUAL_REFLECTION_IMPL(UIControl)
{
    ReflectionRegistrator<UIControl>::Begin()
    .ConstructorByPointer()
    .DestructorByPointer([](UIControl* o) { o->Release(); })
    .Field<const FastName& (UIControl::*)() const, void (UIControl::*)(const FastName&)>("name", &UIControl::GetName, &UIControl::SetName)[M::HiddenField()] // Hide because QE control Name property manually
    .Field("position", &UIControl::GetPosition, &UIControl::SetPosition)[M::DisplayName("Position")]
    .Field("absolutePosition", &UIControl::GetAbsolutePosition, nullptr)[M::HiddenField()]
    .Field("size", &UIControl::GetSize, &UIControl::SetSize)[M::DisplayName("Size")]
    .Field("scale", &UIControl::GetScale, &UIControl::SetScale)[M::DisplayName("Scale")]
    .Field("pivot", &UIControl::GetPivot, &UIControl::SetPivot)[M::DisplayName("Pivot")]
    .Field("angle", &UIControl::GetAngleInDegrees, &UIControl::SetAngleInDegrees)[M::DisplayName("Angle")]
    .Field("visible", &UIControl::GetVisibilityFlag, &UIControl::SetVisibilityFlag)[M::DisplayName("Visible"), M::Bindable()]
    .Field("enabled", &UIControl::GetEnabled, &UIControl::SetEnabledNotHierarchic)[M::DisplayName("Enabled"), M::Bindable()]
    .Field("selected", &UIControl::GetSelected, &UIControl::SetSelectedNotHierarchic)[M::DisplayName("Selected"), M::Bindable()]
    .Field("noInput", &UIControl::GetNoInput, &UIControl::SetNoInput)[M::DisplayName("No Input")]
    .Field("exclusiveInput", &UIControl::GetExclusiveInput, &UIControl::SetExclusiveInputNotHierarchic)[M::DisplayName("Exclusive Input")]
    .Field("wheelSensitivity", &UIControl::GetWheelSensitivity, &UIControl::SetWheelSensitivity)[M::DisplayName("Wheel Sensitivity")]
    .Field("tag", &UIControl::GetTag, &UIControl::SetTag)[M::DisplayName("Tag")]
    .Field("classes", &UIControl::GetClassesAsString, &UIControl::SetClassesFromString)[M::DisplayName("Style Classes"), M::Bindable()]
    .Field("components", &UIControl::GetComponents, nullptr)[M::HiddenField()]
    .Method<UIControl* (UIControl::*)(const String&)>("FindByPath", &UIControl::FindByPath)
    .Method("GetComponentByName", &UIControl::GetComponentByName)
    .Method("GetOrCreateComponentByName", &UIControl::GetOrCreateComponentByName)
    .End();
}

static Mutex controlsListMutex;
static Vector<const UIControl*> controlsList; //weak pointers

static void StartControlTracking(const UIControl* control)
{
#if defined(__DAVAENGINE_DEBUG__)
    LockGuard<Mutex> lock(controlsListMutex);
    controlsList.push_back(control);
#endif
}

static void StopControlTracking(const UIControl* control)
{
#if defined(__DAVAENGINE_DEBUG__)
    LockGuard<Mutex> lock(controlsListMutex);
    controlsList.erase(find(controlsList.begin(), controlsList.end(), control));
#endif
}

UIControl::UIControl(const Rect& rect)
    : exclusiveInput(false)
    , isInputProcessed(false)
    , visible(true)
    , hiddenForDebug(false)
    , multiInput(false)
    , isIteratorCorrupted(false)
    , styleSheetDirty(true)
    , styleSheetInitialized(false)
    , layoutDirty(true)
    , layoutPositionDirty(true)
    , layoutOrderDirty(true)
    , inputEnabled(true)
{
    StartControlTracking(this);
    UpdateFamily();
    SetRect(rect);
}

UIControl::~UIControl()
{
    if (scene)
    {
        scene->CancelInputs(this);
    }
    else
    {
        GetEngineContext()->uiControlSystem->CancelInputs(this);
    }
    eventDispatcher = nullptr;
    RemoveAllControls();
    RemoveAllComponents();
    UIControlFamily::Release(family);
    StopControlTracking(this);
}

UIControlSystem* UIControl::GetScene() const
{
    return scene;
}

void UIControl::SetScene(UIControlSystem* scene_)
{
    if (scene == scene_)
    {
        return;
    }
    scene = scene_;
    for (const auto& child : children)
    {
        child->SetScene(scene);
    }
}

void UIControl::SetParent(UIControl* newParent)
{
    if (parent)
    {
        parent->UnregisterInputProcessors(inputProcessorsCount);
    }
    parent = newParent;
    if (parent)
    {
        PropagateParentWithContext(newParent->packageContext ? newParent : newParent->parentWithContext);

        parent->RegisterInputProcessors(inputProcessorsCount);
    }
    else
    {
        PropagateParentWithContext(nullptr);
    }
}
UIControl* UIControl::GetParent() const
{
    return parent;
}

void UIControl::SetExclusiveInput(bool isExclusiveInput, bool hierarchic /* = true*/)
{
    exclusiveInput = isExclusiveInput;

    if (hierarchic)
    {
        auto it = children.begin();
        for (; it != children.end(); ++it)
        {
            (*it)->SetExclusiveInput(isExclusiveInput, hierarchic);
        }
    }
}

void UIControl::SetMultiInput(bool isMultiInput, bool hierarchic /* = true*/)
{
    multiInput = isMultiInput;

    if (hierarchic)
    {
        auto it = children.begin();
        for (; it != children.end(); ++it)
        {
            (*it)->SetMultiInput(isMultiInput, hierarchic);
        }
    }
}

void UIControl::AddEvent(int32 eventType, const Message& msg)
{
    if (!eventDispatcher)
    {
        eventDispatcher = MakeRef<EventDispatcher>();
    }
    eventDispatcher->AddEvent(eventType, msg);
}
bool UIControl::RemoveEvent(int32 eventType, const Message& msg)
{
    if (eventDispatcher)
    {
        return eventDispatcher->RemoveEvent(eventType, msg);
    }
    return false;
}

bool UIControl::RemoveAllEvents()
{
    if (eventDispatcher)
    {
        return eventDispatcher->RemoveAllEvents();
    }
    return false;
}

void UIControl::PerformEvent(int32 eventType, const UIEvent* uiEvent /* = nullptr*/)
{
    if (scene)
    {
        scene->ProcessControlEvent(eventType, uiEvent, this);
    }
    else
    {
        GetEngineContext()->uiControlSystem->ProcessControlEvent(eventType, uiEvent, this);
    }

    if (eventDispatcher)
    {
        eventDispatcher->PerformEvent(eventType, this);
    }
}

void UIControl::PerformEventWithData(int32 eventType, void* callerData, const UIEvent* uiEvent /* = nullptr*/)
{
    if (scene)
    {
        scene->ProcessControlEvent(eventType, uiEvent, this);
    }
    else
    {
        GetEngineContext()->uiControlSystem->ProcessControlEvent(eventType, uiEvent, this);
    }

    if (eventDispatcher)
    {
        eventDispatcher->PerformEventWithData(eventType, this, callerData);
    }
}

const List<RefPtr<UIControl>>& UIControl::GetChildren() const
{
    return children;
}

void UIControl::SortChildren(const SortFunction& predicate)
{
    children.sort(predicate); // std::stable_sort and std::sort are not allowed for list

    isIteratorCorrupted = true;
    SetLayoutOrderDirty();
}

void UIControl::SetName(const String& name_)
{
    SetName(FastName(name_));
}

void UIControl::SetName(const FastName& name_)
{
    if (name == name_)
    {
        return;
    }

    DVASSERT(UIControlHelpers::IsControlNameValid(name_), Format("Control name '%s' contains incorrect symbols", name_.c_str()).c_str());

    name = name_;

    SetStyleSheetDirty();
}

void UIControl::SetTag(int32 tag_)
{
    tag = tag_;
}

// return first control with given name
UIControl* UIControl::FindByName(const String& name, bool recursive) const
{
    return UIControlHelpers::FindChildControlByName(name, this, recursive);
}

UIControl* UIControl::FindByName(const FastName& name, bool recursive) const
{
    return UIControlHelpers::FindChildControlByName(name, this, recursive);
}

const UIControl* UIControl::FindByPath(const String& path) const
{
    return UIControlHelpers::FindControlByPath(path, this);
}

UIControl* UIControl::FindByPath(const String& path)
{
    return UIControlHelpers::FindControlByPath(path, this);
}

void UIControl::SetState(int32 state)
{
    if (controlState != state)
    {
        controlState = state;
        SetStyleSheetDirty();
    }
}

void UIControl::AddState(int32 state)
{
    SetState(controlState | state);
}

void UIControl::RemoveState(int32 state)
{
    SetState(controlState & ~state);
}

int32 UIControl::GetFrame() const
{
    return GetComponent<UIControlBackground>()->GetFrame();
}

const UIGeometricData& UIControl::GetGeometricData() const
{
    tempGeometricData.position = relativePosition;
    tempGeometricData.size = size;
    tempGeometricData.pivotPoint = GetPivotPoint();
    tempGeometricData.scale = scale;
    tempGeometricData.angle = angle;
    tempGeometricData.unrotatedRect.x = relativePosition.x - relativePosition.x * scale.x;
    tempGeometricData.unrotatedRect.y = relativePosition.y - GetPivotPoint().y * scale.y;
    tempGeometricData.unrotatedRect.dx = size.x * scale.x;
    tempGeometricData.unrotatedRect.dy = size.y * scale.y;

    if (!parent)
    {
        if (scene)
        {
            tempGeometricData.AddGeometricData(scene->GetRenderSystem()->GetBaseGeometricData());
        }
        else
        {
            tempGeometricData.AddGeometricData(GetEngineContext()->uiControlSystem->GetRenderSystem()->GetBaseGeometricData());
        }
    }
    else
    {
        tempGeometricData.AddGeometricData(parent->GetGeometricData());
    }

    return tempGeometricData;
}

UIGeometricData UIControl::GetLocalGeometricData() const
{
    UIGeometricData drawData;
    drawData.position = relativePosition;
    drawData.size = size;
    drawData.pivotPoint = GetPivotPoint();
    drawData.scale = scale;
    drawData.angle = angle;

    return drawData;
}

Vector2 UIControl::GetAbsolutePosition() const
{
    return GetGeometricData().position;
}

void UIControl::SetPosition(const Vector2& position)
{
    if (relativePosition == position)
        return;

    relativePosition = position;
    SetLayoutPositionDirty();
}

void UIControl::SetAbsolutePosition(const Vector2& position)
{
    if (parent)
    {
        const UIGeometricData& parentGD = parent->GetGeometricData();
        SetPosition(position - parentGD.position + parentGD.pivotPoint);
    }
    else
    {
        SetPosition(position);
    }
}

void UIControl::SetSize(const Vector2& newSize)
{
    if (size == newSize)
        return;

    Vector2 oldPivot = GetPivot();
    size = newSize;
    SetPivot(oldPivot);

    SetLayoutDirty();
}

void UIControl::SetPivotPoint(const Vector2& newPivotPoint)
{
    Vector2 newPivot((size.x == 0.0f) ? 0.0f : (newPivotPoint.x / size.x),
                     (size.y == 0.0f) ? 0.0f : (newPivotPoint.y / size.y));

    if (pivot == newPivot)
        return;

    pivot = newPivot;

    SetLayoutPositionDirty();
}

void UIControl::SetPivot(const Vector2& newPivot)
{
    if (pivot == newPivot)
        return;

    pivot = newPivot;

    SetLayoutPositionDirty();
}

void UIControl::SetAngle(float32 angleInRad)
{
    angle = angleInRad;
}

void UIControl::SetAngleInDegrees(float32 angleInDeg)
{
    SetAngle(DegToRad(angleInDeg));
}

Rect UIControl::GetAbsoluteRect() const
{
    return Rect(GetAbsolutePosition() - GetPivotPoint(), size);
}

void UIControl::SetRect(const Rect& rect)
{
    SetSize(rect.GetSize());
    SetPosition(rect.GetPosition() + GetPivotPoint());
}

void UIControl::SetAbsoluteRect(const Rect& rect)
{
    if (!parent)
    {
        SetRect(rect);
        return;
    }

    Rect localRect = rect;
    const UIGeometricData& parentGD = parent->GetGeometricData();
    localRect.SetPosition(rect.GetPosition() - parentGD.position + parentGD.pivotPoint);
    SetRect(localRect);
}

void UIControl::SetScaledRect(const Rect& rect, bool rectInAbsoluteCoordinates /* = false*/)
{
    if (!rectInAbsoluteCoordinates || !parent)
    {
        scale.x = rect.dx / size.x;
        scale.y = rect.dy / size.y;
        SetPosition(Vector2(rect.x + GetPivotPoint().x * scale.x, rect.y + GetPivotPoint().y * scale.y));
    }
    else
    {
        const UIGeometricData& gd = parent->GetGeometricData();
        scale.x = rect.dx / (size.x * gd.scale.x);
        scale.y = rect.dy / (size.y * gd.scale.y);
        SetAbsolutePosition(Vector2(rect.x + GetPivotPoint().x * scale.x, rect.y + GetPivotPoint().y * scale.y));
    }
}

float32 UIControl::GetAngleInDegrees() const
{
    return RadToDeg(angle);
}

void UIControl::SetVisibilityFlag(bool isVisible)
{
    if (visible == isVisible)
    {
        return;
    }

    visible = isVisible;

    if (visible)
    {
        eViewState parentViewState = eViewState::INACTIVE;
        if (GetParent())
        {
            parentViewState = GetParent()->viewState;
        }
        else
        {
            if ((scene && scene->IsHostControl(this))
                || GetEngineContext()->uiControlSystem->IsHostControl(this))
            {
                parentViewState = eViewState::VISIBLE;
            }
        }

        InvokeVisible(parentViewState);
    }
    else
    {
        InvokeInvisible();
    }

    SetLayoutDirty();
}

void UIControl::SetInputEnabled(bool isEnabled, bool hierarchic /* = true*/)
{
    if (isEnabled != inputEnabled)
    {
        inputEnabled = isEnabled;
        if (inputEnabled)
        {
            RegisterInputProcessor();
        }
        else
        {
            UnregisterInputProcessor();
        }
    }
    if (hierarchic)
    {
        auto it = children.begin();
        for (; it != children.end(); ++it)
        {
            (*it)->SetInputEnabled(isEnabled, hierarchic);
        }
    }
}

bool UIControl::GetDisabled() const
{
    return ((controlState & STATE_DISABLED) != 0);
}

void UIControl::SetDisabled(bool isDisabled, bool hierarchic /* = true*/)
{
    if (isDisabled)
    {
        AddState(STATE_DISABLED);

        // Cancel all inputs because of DF-2943.
        if (scene)
        {
            scene->CancelInputs(this);
        }
        else
        {
            GetEngineContext()->uiControlSystem->CancelInputs(this);
        }
    }
    else
    {
        RemoveState(STATE_DISABLED);
    }

    if (hierarchic)
    {
        auto it = children.begin();
        for (; it != children.end(); ++it)
        {
            (*it)->SetDisabled(isDisabled, hierarchic);
        }
    }
}

bool UIControl::GetSelected() const
{
    return ((controlState & STATE_SELECTED) != 0);
}

void UIControl::SetSelected(bool isSelected, bool hierarchic /* = true*/)
{
    if (isSelected)
    {
        AddState(STATE_SELECTED);
    }
    else
    {
        RemoveState(STATE_SELECTED);
    }

    if (hierarchic)
    {
        auto it = children.begin();
        for (; it != children.end(); ++it)
        {
            (*it)->SetSelected(isSelected, hierarchic);
        }
    }
}

bool UIControl::GetHover() const
{
    return (controlState & STATE_HOVER) != 0;
}

void UIControl::AddControl(UIControl* control_)
{
    RefPtr<UIControl> control(RefPtr<UIControl>::ConstructWithRetain(control_));

    control->RemoveFromParent();

    control->isInputProcessed = false;
    control->SetScene(GetScene());
    control->SetParent(this);
    children.push_back(control);

    control->InvokeActive(viewState);

    isIteratorCorrupted = true;
    SetLayoutDirty();
}

void UIControl::AddControl(RefPtr<UIControl> control)
{
    AddControl(control.Get());
}

void UIControl::RemoveControl(UIControl* control_)
{
    if (nullptr == control_)
    {
        return;
    }

    RefPtr<UIControl> control(RefPtr<UIControl>::ConstructWithRetain(control_));

    auto it = children.begin();
    for (; it != children.end(); ++it)
    {
        if ((*it) == control)
        {
            control->InvokeInactive();

            control->SetScene(nullptr);
            control->SetParent(NULL);
            children.erase(it);
            isIteratorCorrupted = true;
            SetLayoutDirty();
            return;
        }
    }
}

void UIControl::RemoveControl(RefPtr<UIControl> control)
{
    RemoveControl(control.Get());
}

void UIControl::RemoveFromParent()
{
    UIControl* parentControl = GetParent();
    if (parentControl)
    {
        parentControl->RemoveControl(this);
    }
}

void UIControl::RemoveAllControls()
{
    while (!children.empty())
    {
        RemoveControl(children.front());
    }
}

void UIControl::BringChildFront(const UIControl* control_)
{
    auto it = children.begin();
    for (; it != children.end(); ++it)
    {
        if ((*it).Get() == control_)
        {
            RefPtr<UIControl> control(*it);
            children.erase(it);
            children.push_back(control);
            isIteratorCorrupted = true;
            SetLayoutOrderDirty();
            return;
        }
    }
}

void UIControl::BringChildBack(const UIControl* control_)
{
    auto it = children.begin();
    for (; it != children.end(); ++it)
    {
        if ((*it) == control_)
        {
            RefPtr<UIControl> control(*it);
            children.erase(it);
            children.push_front(control);
            isIteratorCorrupted = true;
            SetLayoutOrderDirty();
            return;
        }
    }
}
void UIControl::InsertChildBelow(UIControl* control_, const UIControl* belowThisChild_)
{
    RefPtr<UIControl> control(RefPtr<UIControl>::ConstructWithRetain(control_));
    auto it = children.begin();
    for (; it != children.end(); ++it)
    {
        if ((*it) == belowThisChild_)
        {
            control->RemoveFromParent();

            children.insert(it, control);
            control->SetScene(GetScene());
            control->SetParent(this);

            control->InvokeActive(viewState);

            isIteratorCorrupted = true;
            SetLayoutDirty();
            return;
        }
    }

    AddControl(control);
}

void UIControl::InsertChildBelow(RefPtr<UIControl> control, const UIControl* belowThisChild_)
{
    InsertChildBelow(control.Get(), belowThisChild_);
}

void UIControl::InsertChildAbove(UIControl* control_, const UIControl* aboveThisChild_)
{
    RefPtr<UIControl> control(RefPtr<UIControl>::ConstructWithRetain(control_));
    auto it = children.begin();
    for (; it != children.end(); ++it)
    {
        if ((*it) == aboveThisChild_)
        {
            control->RemoveFromParent();

            children.insert(++it, control);
            control->SetScene(GetScene());
            control->SetParent(this);

            control->InvokeActive(viewState);

            isIteratorCorrupted = true;
            SetLayoutDirty();
            return;
        }
    }

    AddControl(control);
}

void UIControl::InsertChildAbove(RefPtr<UIControl> control, const UIControl* aboveThisChild_)
{
    InsertChildAbove(control.Get(), aboveThisChild_);
}

void UIControl::SendChildBelow(const UIControl* control_, const UIControl* belowThisChild_)
{
    //TODO: Fix situation when controls not from this hierarchy

    // firstly find control in list and erase it
    RefPtr<UIControl> control;
    auto it = children.begin();
    for (; it != children.end(); ++it)
    {
        if ((*it) == control_)
        {
            control = *it;
            children.erase(it);
            isIteratorCorrupted = true;
            break;
        }
    }

    if (control == nullptr)
    {
        DVASSERT(control != nullptr, Format("[UIControl::SendChildBelow] Control \"%s\" not found in current hierarchy", control_->GetName().c_str()).c_str());
        return;
    }

    // after that find place where we should put the control and do that
    it = children.begin();
    for (; it != children.end(); ++it)
    {
        if ((*it) == belowThisChild_)
        {
            children.insert(it, control);
            isIteratorCorrupted = true;
            SetLayoutOrderDirty();
            return;
        }
    }
    DVASSERT(0, Format("[UIControl::SendChildBelow] Control \"%s\" not found in current hierarchy", belowThisChild_->GetName().c_str()).c_str());
}

void UIControl::SendChildAbove(const UIControl* control_, const UIControl* aboveThisChild_)
{
    //TODO: Fix situation when controls not from this hierarhy

    // firstly find control in list and erase it
    RefPtr<UIControl> control;
    auto it = children.begin();
    for (; it != children.end(); ++it)
    {
        if ((*it) == control_)
        {
            control = *it;
            children.erase(it);
            isIteratorCorrupted = true;
            break;
        }
    }
    if (control == nullptr)
    {
        DVASSERT(control != nullptr, Format("[UIControl::SendChildAbove] Control \"%s\" not found in current hierarchy", control_->GetName().c_str()).c_str());
        return;
    }
    // after that find place where we should put the control and do that
    it = children.begin();
    for (; it != children.end(); ++it)
    {
        if ((*it) == aboveThisChild_)
        {
            children.insert(++it, control);
            isIteratorCorrupted = true;
            SetLayoutOrderDirty();
            return;
        }
    }

    DVASSERT(0, Format("[UIControl::SendChildAbove] Control \"%s\" not found in current hierarchy", aboveThisChild_->GetName().c_str()).c_str());
}

UIControl* UIControl::Clone()
{
    UIControl* c = new UIControl(Rect(relativePosition.x, relativePosition.y, size.x, size.y));
    c->CopyDataFrom(this);
    return c;
}

RefPtr<UIControl> UIControl::SafeClone()
{
    return RefPtr<UIControl>(Clone());
}

void UIControl::CopyDataFrom(UIControl* srcControl)
{
    relativePosition = srcControl->relativePosition;
    size = srcControl->size;
    pivot = srcControl->pivot;
    scale = srcControl->scale;
    angle = srcControl->angle;

    tag = srcControl->GetTag();
    name = srcControl->name;

    controlState = srcControl->controlState;
    exclusiveInput = srcControl->exclusiveInput;
    visible = srcControl->visible;
    inputEnabled = srcControl->inputEnabled;

    hiddenForDebug = srcControl->hiddenForDebug;
    classes = srcControl->classes;
    localProperties = srcControl->localProperties;
    styledProperties = srcControl->styledProperties;
    styleSheetDirty = srcControl->styleSheetDirty;
    styleSheetInitialized = false;
    layoutDirty = srcControl->layoutDirty;
    layoutPositionDirty = srcControl->layoutPositionDirty;
    layoutOrderDirty = srcControl->layoutOrderDirty;
    packageContext = srcControl->packageContext;

    eventDispatcher = nullptr;
    if (srcControl->eventDispatcher != nullptr && srcControl->eventDispatcher->GetEventsCount() != 0)
    {
        Logger::FrameworkDebug("[UIControl::CopyDataFrom] Source control \"%s:%s\" have events."
                               "Event copying is forbidden.",
                               srcControl->GetClassName().c_str(), srcControl->GetName().c_str());
    }

    RemoveAllComponents();
    for (UIComponent* srcComponent : srcControl->components)
    {
        AddComponent(srcComponent->SafeClone().Get());
    }

    RemoveAllControls();
    if (inputEnabled)
    {
        inputProcessorsCount = 1;
    }
    else
    {
        inputProcessorsCount = 0;
    }

    for (const auto& srcChild : srcControl->GetChildren())
    {
        AddControl(srcChild->SafeClone());
    }
}

bool UIControl::IsActive() const
{
    return (viewState >= eViewState::ACTIVE);
}

bool UIControl::IsVisible() const
{
    return (viewState == eViewState::VISIBLE);
}

void UIControl::SetParentColor(const Color& parentColor)
{
    UIControlBackground* bg = GetComponent<UIControlBackground>();
    if (bg)
    {
        bg->SetParentColor(parentColor);
    }
}

bool UIControl::IsPointInside(const Vector2& point_, bool expandWithFocus /* = false*/) const
{
    Vector2 point = point_;
    if (GetPrimaryWindow()->GetCursorCapture() == eCursorCapture::PINNING)
    {
        Size2f sz = GetPrimaryWindow()->GetVirtualSize();
        point.x = sz.dx / 2.f;
        point.y = sz.dy / 2.f;
    }
    const UIGeometricData& gd = GetGeometricData();
    Rect rect = gd.GetUnrotatedRect();
    if (expandWithFocus)
    {
        rect.dx += CONTROL_TOUCH_AREA * 2;
        rect.dy += CONTROL_TOUCH_AREA * 2;
        rect.x -= CONTROL_TOUCH_AREA;
        rect.y -= CONTROL_TOUCH_AREA;
    }
    if (gd.angle != 0)
    {
        Vector2 testPoint;
        testPoint.x = (point.x - gd.position.x) * gd.cosA + (gd.position.y - point.y) * -gd.sinA + gd.position.x;
        testPoint.y = (point.x - gd.position.x) * -gd.sinA + (point.y - gd.position.y) * gd.cosA + gd.position.y;
        return rect.PointInside(testPoint);
    }

    return rect.PointInside(point);
}

bool UIControl::SystemProcessInput(UIEvent* currentInput)
{
    if (!inputEnabled || !GetVisibilityFlag() || controlState & STATE_DISABLED)
    {
        return false;
    }
    if ((scene && scene->GetExclusiveInputLocker() && scene->GetExclusiveInputLocker() != this)
        || (GetEngineContext()->uiControlSystem->GetExclusiveInputLocker() && GetEngineContext()->uiControlSystem->GetExclusiveInputLocker() != this))
    {
        return false;
    }
    if (customSystemProcessInput != nullptr && customSystemProcessInput(this, currentInput))
    {
        return true;
    }

    switch (currentInput->phase)
    {
    case UIEvent::Phase::CHAR:
    case UIEvent::Phase::CHAR_REPEAT:
    case UIEvent::Phase::KEY_DOWN:
    case UIEvent::Phase::KEY_DOWN_REPEAT:
    case UIEvent::Phase::KEY_UP:
    {
        Input(currentInput);
    }
    break;

    case UIEvent::Phase::MOVE:
    {
        if (!currentInput->touchLocker && IsPointInside(currentInput->point))
        {
            if (scene)
            {
                scene->SetHoveredControl(this);
            }
            else
            {
                GetEngineContext()->uiControlSystem->SetHoveredControl(this);
            }
            Input(currentInput);
            return true;
        }
    }
    break;
    case UIEvent::Phase::WHEEL:
    {
        if (IsPointInside(currentInput->point))
        {
            Input(currentInput);
            return true;
        }
    }
    break;
    case UIEvent::Phase::BEGAN:
    {
        if (!currentInput->touchLocker && IsPointInside(currentInput->point))
        {
            if (multiInput || !currentInputID)
            {
                AddState(STATE_PRESSED_INSIDE);
                RemoveState(STATE_NORMAL);
                ++touchesInside;
                ++totalTouches;
                currentInput->controlState = UIEvent::CONTROL_STATE_INSIDE;

                // Yuri Coder, 2013/12/18. Set the touch lockers before the EVENT_TOUCH_DOWN handler
                // to have possibility disable control inside the EVENT_TOUCH_DOWN. See also DF-2943.
                currentInput->touchLocker = this;
                if (exclusiveInput)
                {
                    if (scene)
                    {
                        scene->SetExclusiveInputLocker(this, currentInput->touchId);
                    }
                    else
                    {
                        GetEngineContext()->uiControlSystem->SetExclusiveInputLocker(this, currentInput->touchId);
                    }
                }

                if (scene)
                {
                    if (scene->GetFocusedControl() != this && FocusHelpers::CanFocusControl(this))
                    {
                        scene->SetFocusedControl(this);
                    }
                }
                else
                {
                    if (GetEngineContext()->uiControlSystem->GetFocusedControl() != this && FocusHelpers::CanFocusControl(this))
                    {
                        GetEngineContext()->uiControlSystem->SetFocusedControl(this);
                    }
                }

                if (!multiInput)
                {
                    currentInputID = currentInput->touchId;
                }

                PerformEventWithData(EVENT_TOUCH_DOWN, currentInput, currentInput);

                Input(currentInput);
                return true;
            }
            else
            {
                currentInput->touchLocker = this;
                return true;
            }
        }
    }
    break;
    case UIEvent::Phase::DRAG:
    {
        if (currentInput->touchLocker == this)
        {
            if (multiInput || currentInputID == currentInput->touchId)
            {
                if (controlState & STATE_PRESSED_INSIDE || controlState & STATE_PRESSED_OUTSIDE)
                {
                    if (IsPointInside(currentInput->point, true))
                    {
                        if (currentInput->controlState == UIEvent::CONTROL_STATE_OUTSIDE)
                        {
                            currentInput->controlState = UIEvent::CONTROL_STATE_INSIDE;
                            ++touchesInside;
                            if (touchesInside > 0)
                            {
                                AddState(STATE_PRESSED_INSIDE
#if !defined(__DAVAENGINE_IPHONE__) && !defined(__DAVAENGINE_ANDROID__)
                                         | STATE_HOVER
#endif
                                         );
                                RemoveState(STATE_PRESSED_OUTSIDE);
                            }
                        }
                    }
                    else
                    {
                        if (currentInput->controlState == UIEvent::CONTROL_STATE_INSIDE)
                        {
                            currentInput->controlState = UIEvent::CONTROL_STATE_OUTSIDE;
                            --touchesInside;
                            if (touchesInside == 0)
                            {
                                AddState(STATE_PRESSED_OUTSIDE);
                                RemoveState(STATE_PRESSED_INSIDE);
                            }
                        }
                    }
                }
                Input(currentInput);
            }
            return true;
        }
    }
    break;
    case UIEvent::Phase::ENDED:
    {
        if (currentInput->touchLocker == this)
        {
            if (multiInput || currentInputID == currentInput->touchId)
            {
                Input(currentInput);
                if (currentInput->touchId == currentInputID)
                {
                    currentInputID = 0;
                }
                if (totalTouches > 0)
                {
                    --totalTouches;
                    if (currentInput->controlState == UIEvent::CONTROL_STATE_INSIDE)
                    {
                        --touchesInside;
#if !defined(__DAVAENGINE_IPHONE__) && !defined(__DAVAENGINE_ANDROID__)
                        if (totalTouches == 0)
                        {
                            AddState(STATE_HOVER);
                        }
#endif
                    }

                    currentInput->controlState = UIEvent::CONTROL_STATE_RELEASED;

                    if (totalTouches == 0)
                    {
                        bool isPointInside = IsPointInside(currentInput->point, true);
                        eEventType event = isPointInside ? EVENT_TOUCH_UP_INSIDE : EVENT_TOUCH_UP_OUTSIDE;

                        Analytics::EmitUIEvent(this, event, currentInput);

#ifdef __DAVAENGINE_AUTOTESTING__
                        AutotestingSystem::Instance()->OnRecordClickControl(this);
#endif
                        PerformEventWithData(event, currentInput, currentInput);

                        AddState(STATE_NORMAL);
                        RemoveState(STATE_PRESSED_INSIDE | STATE_PRESSED_OUTSIDE);
                        if (scene)
                        {
                            if (scene->GetExclusiveInputLocker() == this)
                            {
                                scene->SetExclusiveInputLocker(nullptr, -1);
                            }
                        }
                        else
                        {
                            if (GetEngineContext()->uiControlSystem->GetExclusiveInputLocker() == this)
                            {
                                GetEngineContext()->uiControlSystem->SetExclusiveInputLocker(nullptr, -1);
                            }
                        }
                    }
                    else if (touchesInside <= 0)
                    {
                        AddState(STATE_PRESSED_OUTSIDE);
                        RemoveState(STATE_PRESSED_INSIDE
#if !defined(__DAVAENGINE_IPHONE__) && !defined(__DAVAENGINE_ANDROID__)
                                    | STATE_HOVER
#endif
                                    );
                    }
                }
            }

            currentInput->touchLocker = NULL;
            return true;
        }
    }
    break;
    case UIEvent::Phase::JOYSTICK:
    {
        Input(currentInput);
    }
    default:
        break;
    }

    return false;
}

bool UIControl::SystemInput(UIEvent* currentInput)
{
    isInputProcessed = true;

    if (!GetVisibilityFlag())
        return false;

    //if(currentInput->touchLocker != this)
    {
        UIClipContentComponent* clipContent = GetComponent<UIClipContentComponent>();
        bool clipContents = (clipContent != nullptr && clipContent->IsEnabled());
        if (clipContents &&
            (UIEvent::Phase::BEGAN == currentInput->phase || UIEvent::Phase::MOVE == currentInput->phase || UIEvent::Phase::WHEEL == currentInput->phase || UIEvent::Phase::CANCELLED == currentInput->phase))
        {
            if (!IsPointInside(currentInput->point))
            {
                return false;
            }
        }

        for (const auto& child : children)
        {
            child->isInputProcessed = false;
        };

        auto it = children.rbegin();
        auto itEnd = children.rend();
        while (it != itEnd)
        {
            isIteratorCorrupted = false;
            RefPtr<UIControl> current = *it;
            if (!current->isInputProcessed)
            {
                if (current->inputProcessorsCount > 0)
                {
                    if (current->SystemInput(currentInput))
                    {
                        return true;
                    }
                }
                if (isIteratorCorrupted)
                {
                    it = children.rbegin();
                    continue;
                }
            }
            ++it;
        }
    }
    return SystemProcessInput(currentInput);
}

void UIControl::SystemInputCancelled(UIEvent* currentInput)
{
    if (currentInput->controlState != UIEvent::CONTROL_STATE_RELEASED)
    {
        --totalTouches;
    }
    if (currentInput->controlState == UIEvent::CONTROL_STATE_INSIDE)
    {
        --touchesInside;
    }

    if (touchesInside == 0)
    {
        RemoveState(STATE_PRESSED_INSIDE | STATE_PRESSED_OUTSIDE);
        AddState(STATE_NORMAL);
        if (scene)
        {
            if (scene->GetExclusiveInputLocker() == this)
            {
                scene->SetExclusiveInputLocker(nullptr, -1);
            }
        }
        else
        {
            if (GetEngineContext()->uiControlSystem->GetExclusiveInputLocker() == this)
            {
                GetEngineContext()->uiControlSystem->SetExclusiveInputLocker(nullptr, -1);
            }
        }
    }

    currentInput->controlState = UIEvent::CONTROL_STATE_RELEASED;
    if (currentInput->touchId == currentInputID)
    {
        currentInputID = 0;
    }
    currentInput->touchLocker = NULL;

    InputCancelled(currentInput);
}

void UIControl::SystemDidSetHovered()
{
    AddState(STATE_HOVER);
    PerformEventWithData(EVENT_HOVERED_SET, NULL);
    DidSetHovered();
}

void UIControl::SystemDidRemoveHovered()
{
    PerformEventWithData(EVENT_HOVERED_REMOVED, NULL);
    RemoveState(STATE_HOVER);
    DidRemoveHovered();
}

void UIControl::DidSetHovered()
{
}

void UIControl::DidRemoveHovered()
{
}

void UIControl::Input(UIEvent* currentInput)
{
    currentInput->SetInputHandledType(UIEvent::INPUT_NOT_HANDLED);
}

void UIControl::InputCancelled(UIEvent* currentInput)
{
}

void UIControl::Update(float32 timeElapsed)
{
}

void UIControl::Draw(const UIGeometricData& geometricData)
{
    UIControlBackground* bg = GetComponent<UIControlBackground>();
    if (bg)
    {
        bg->Draw(geometricData);
    }
}

void UIControl::DrawAfterChilds(const UIGeometricData& geometricData)
{
}

void UIControl::SystemVisible()
{
    if (viewState == eViewState::VISIBLE)
    {
        DVASSERT(false, Format("Unexpected view state %d in control with name '%s'", static_cast<int32>(viewState), name.c_str()).c_str());
        return;
    }

    ChangeViewState(eViewState::VISIBLE);
    if (scene)
    {
        scene->RegisterVisibleControl(this);
    }
    else
    {
        GetEngineContext()->uiControlSystem->RegisterVisibleControl(this);
    }

    SetStyleSheetDirty();
    OnVisible();

    auto it = children.begin();
    isIteratorCorrupted = false;
    while (it != children.end())
    {
        RefPtr<UIControl> child;
        child = *it;

        child->InvokeVisible(viewState);

        if (isIteratorCorrupted)
        {
            it = children.begin();
            isIteratorCorrupted = false;
            continue;
        }

        ++it;
    }
}

void UIControl::SystemInvisible()
{
    if (viewState != eViewState::VISIBLE)
    {
        DVASSERT(false, Format("Unexpected view state %d in control with name '%s'", static_cast<int32>(viewState), name.c_str()).c_str());
        return;
    }

    auto it = children.rbegin();
    isIteratorCorrupted = false;
    while (it != children.rend())
    {
        RefPtr<UIControl> child;
        child = *it;
        if (child->IsVisible())
        {
            child->InvokeInvisible();

            if (isIteratorCorrupted)
            {
                it = children.rbegin();
                isIteratorCorrupted = false;
                continue;
            }
        }
        ++it;
    }

    ChangeViewState(eViewState::ACTIVE);
    if (scene)
    {
        scene->UnregisterVisibleControl(this);
    }
    else
    {
        GetEngineContext()->uiControlSystem->UnregisterVisibleControl(this);
    }
    OnInvisible();
}

void UIControl::OnVisible()
{
}

void UIControl::OnInvisible()
{
}

void UIControl::SystemActive()
{
    if (viewState >= eViewState::ACTIVE)
    {
        DVASSERT(false, Format("Unexpected view state %d in control with name '%s'", static_cast<int32>(viewState), name.c_str()).c_str());
        return;
    }

    ChangeViewState(eViewState::ACTIVE);
    if (scene)
    {
        scene->RegisterControl(this);
    }
    else
    {
        GetEngineContext()->uiControlSystem->RegisterControl(this);
    }

    OnActive();

    auto it = children.begin();
    isIteratorCorrupted = false;
    while (it != children.end())
    {
        RefPtr<UIControl> child;
        child = *it;

        child->InvokeActive(viewState);

        if (isIteratorCorrupted)
        {
            it = children.begin();
            isIteratorCorrupted = false;
            continue;
        }
        ++it;
    }
}

void UIControl::SystemInactive()
{
    if (viewState != eViewState::ACTIVE)
    {
        DVASSERT(false, Format("Unexpected view state %d in control with name '%s'", static_cast<int32>(viewState), name.c_str()).c_str());
        return;
    }

    auto it = children.rbegin();
    isIteratorCorrupted = false;
    while (it != children.rend())
    {
        RefPtr<UIControl> child;
        child = *it;

        child->InvokeInactive();

        if (isIteratorCorrupted)
        {
            it = children.rbegin();
            isIteratorCorrupted = false;
            continue;
        }

        ++it;
    }

    ChangeViewState(eViewState::INACTIVE);

    if (scene)
    {
        scene->UnregisterControl(this);
    }
    else
    {
        GetEngineContext()->uiControlSystem->UnregisterControl(this);
    }

    OnInactive();
}

void UIControl::OnActive()
{
}

void UIControl::OnInactive()
{
}

void UIControl::SystemScreenSizeChanged(const Rect& newFullScreenRect)
{
    OnScreenSizeChanged(newFullScreenRect);

    auto it = children.begin();
    isIteratorCorrupted = false;
    while (it != children.end())
    {
        RefPtr<UIControl> child;
        child = *it;

        child->SystemScreenSizeChanged(newFullScreenRect);

        if (isIteratorCorrupted)
        {
            it = children.begin();
            isIteratorCorrupted = false;
            continue;
        }

        ++it;
    }
}

void UIControl::OnScreenSizeChanged(const Rect& newFullScreenRect)
{
}

void UIControl::InvokeActive(eViewState parentViewState)
{
    if (!IsActive() && parentViewState >= eViewState::ACTIVE)
    {
        SystemActive();
        InvokeVisible(parentViewState);
    }
}

void UIControl::InvokeInactive()
{
    if (IsActive())
    {
        InvokeInvisible();
        SystemInactive();
    }
}

void UIControl::InvokeVisible(eViewState parentViewState)
{
    if (!IsVisible() && parentViewState == eViewState::VISIBLE && GetVisibilityFlag())
    {
        SystemVisible();
    }
}

void UIControl::InvokeInvisible()
{
    if (IsVisible())
    {
        SystemInvisible();
    }
}

bool IsControlActive(const UIControl* control)
{
    while (control->GetParent() != nullptr)
    {
        control = control->GetParent();
    }

    if (control->GetScene())
    {
        return control->GetScene()->IsHostControl(control);
    }
    else
    {
        return GetEngineContext()->uiControlSystem->IsHostControl(control);
    }
}

bool IsControlVisible(const UIControl* control)
{
    while (control->GetParent() != nullptr)
    {
        if (!control->GetVisibilityFlag())
            return false;

        control = control->GetParent();
    }

    if (control->GetScene())
    {
        return control->GetScene()->IsHostControl(control) ? control->GetVisibilityFlag() : false;
    }
    else
    {
        return GetEngineContext()->uiControlSystem->IsHostControl(control) ? control->GetVisibilityFlag() : false;
    }
}

void UIControl::ChangeViewState(eViewState newViewState)
{
    static const Vector<std::pair<eViewState, eViewState>> validTransitions =
    {
      { eViewState::INACTIVE, eViewState::ACTIVE },
      { eViewState::ACTIVE, eViewState::VISIBLE },
      { eViewState::VISIBLE, eViewState::ACTIVE },
      { eViewState::ACTIVE, eViewState::INACTIVE }
    };

    bool verified = true;
    String errorStr;

    if (!IsControlActive(this))
    {
        errorStr += "Control not in hierarhy.";
        verified = false;
    }

    std::pair<eViewState, eViewState> transition = { viewState, newViewState };
    if (std::find(validTransitions.begin(), validTransitions.end(), transition) == validTransitions.end())
    {
        errorStr += "Unexpected change sequence.";
        verified = false;
    }

    if (viewState == eViewState::ACTIVE && newViewState == eViewState::VISIBLE && !IsControlVisible(this))
    {
        errorStr += "Control not visible on screen.";
        verified = false;
    }

    if (!verified)
    {
        String errorMsg = Format("[UIControl::ChangeViewState] Control '%s', change from state %d to state %d. %s", GetName().c_str(), viewState, newViewState, errorStr.c_str());
        Logger::Error(errorMsg.c_str());
        DVASSERT(false, errorMsg.c_str());
    }

    viewState = newViewState;
}

Animation* UIControl::WaitAnimation(float32 time, int32 track)
{
    Animation* animation = new Animation(this, time, Interpolation::LINEAR);
    animation->Start(track);
    return animation;
}

Animation* UIControl::PositionAnimation(const Vector2& position_, float32 time, Interpolation::FuncType interpolationFunc, int32 track)
{
    LinearAnimation<Vector2>* animation = new LinearAnimation<Vector2>(this, &relativePosition, position_, time, interpolationFunc);
    animation->Start(track);
    return animation;
}

Animation* UIControl::SizeAnimation(const Vector2& size_, float32 time, Interpolation::FuncType interpolationFunc, int32 track)
{
    LinearAnimation<Vector2>* animation = new LinearAnimation<Vector2>(this, &size, size_, time, interpolationFunc);
    animation->Start(track);
    return animation;
}

Animation* UIControl::ScaleAnimation(const Vector2& newScale, float32 time, Interpolation::FuncType interpolationFunc, int32 track)
{
    LinearAnimation<Vector2>* animation = new LinearAnimation<Vector2>(this, &scale, newScale, time, interpolationFunc);
    animation->Start(track);
    return animation;
}

Animation* UIControl::AngleAnimation(float32 newAngle, float32 time, Interpolation::FuncType interpolationFunc /*= Interpolation::LINEAR*/, int32 track /*= 0*/)
{
    LinearAnimation<float32>* animation = new LinearAnimation<float32>(this, &angle, newAngle, time, interpolationFunc);
    animation->Start(track);
    return animation;
}

Animation* UIControl::MoveAnimation(const Rect& rect, float time, Interpolation::FuncType interpolationFunc, int32 track)
{
    TwoVector2LinearAnimation* animation = new TwoVector2LinearAnimation(this, &relativePosition, Vector2(rect.x + GetPivotPoint().x, rect.y + GetPivotPoint().y), &size, Vector2(rect.dx, rect.dy), time, interpolationFunc);
    animation->Start(track);
    return animation;
}

Animation* UIControl::ScaledRectAnimation(const Rect& rect, float32 time, Interpolation::FuncType interpolationFunc, int32 track)
{
    Vector2 finalScale(rect.dx / size.x, rect.dy / size.y);

    TwoVector2LinearAnimation* animation = new TwoVector2LinearAnimation(this, &relativePosition, Vector2(rect.x + GetPivotPoint().x * finalScale.x, rect.y + GetPivotPoint().y * finalScale.y), &scale, finalScale, time, interpolationFunc);
    animation->Start(track);
    return animation;
}

Animation* UIControl::ScaledSizeAnimation(const Vector2& newSize, float32 time, Interpolation::FuncType interpolationFunc, int32 track)
{
    Vector2 finalScale(newSize.x / size.x, newSize.y / size.y);
    LinearAnimation<Vector2>* animation = new LinearAnimation<Vector2>(this, &scale, finalScale, time, interpolationFunc);
    animation->Start(track);
    return animation;
}

void UIControl::TouchableAnimationCallback(BaseObject* caller, void* param, void* callerData)
{
    bool* params = static_cast<bool*>(param);
    SetInputEnabled(params[0], params[1]);
    delete[] params;
}

Animation* UIControl::TouchableAnimation(bool touchable, bool hierarhic /* = true*/, int32 track /* = 0*/)
{
    //TODO: change to bool animation - Dizz
    Animation* animation = new Animation(this, 0.01f, Interpolation::LINEAR);
    bool* params = new bool[2];
    params[0] = touchable;
    params[1] = hierarhic;
    animation->AddEvent(Animation::EVENT_ANIMATION_START, Message(this, &UIControl::TouchableAnimationCallback, static_cast<void*>(params)));
    animation->Start(track);
    return animation;
}

void UIControl::DisabledAnimationCallback(BaseObject* caller, void* param, void* callerData)
{
    bool* params = static_cast<bool*>(param);
    SetDisabled(params[0], params[1]);
    delete[] params;
}

Animation* UIControl::DisabledAnimation(bool disabled, bool hierarhic /* = true*/, int32 track /* = 0*/)
{
    //TODO: change to bool animation - Dizz
    Animation* animation = new Animation(this, 0.01f, Interpolation::LINEAR);
    bool* params = new bool[2];
    params[0] = disabled;
    params[1] = hierarhic;
    animation->AddEvent(Animation::EVENT_ANIMATION_START, Message(this, &UIControl::DisabledAnimationCallback, static_cast<void*>(params)));
    animation->Start(track);
    return animation;
}

void UIControl::VisibleAnimationCallback(BaseObject* caller, void* param, void* callerData)
{
    bool visible = (pointer_size(param) > 0);
    SetVisibilityFlag(visible);
}

Animation* UIControl::VisibleAnimation(bool visible, int32 track /* = 0*/)
{
    Animation* animation = new Animation(this, 0.01f, Interpolation::LINEAR);
    animation->AddEvent(Animation::EVENT_ANIMATION_START, Message(this, &UIControl::VisibleAnimationCallback, reinterpret_cast<void*>(static_cast<pointer_size>(visible))));
    animation->Start(track);
    return animation;
}

void UIControl::RemoveControlAnimationCallback(BaseObject* caller, void* param, void* callerData)
{
    if (parent)
    {
        parent->RemoveControl(this);
    }
}

Animation* UIControl::RemoveControlAnimation(int32 track)
{
    Animation* animation = new Animation(this, 0.01f, Interpolation::LINEAR);
    animation->AddEvent(Animation::EVENT_ANIMATION_START, Message(this, &UIControl::RemoveControlAnimationCallback));
    animation->Start(track);
    return animation;
}

Animation* UIControl::ColorAnimation(const Color& finalColor, float32 time, Interpolation::FuncType interpolationFunc, int32 track)
{
    UIControlBackground* bg = GetComponent<UIControlBackground>();
    DVASSERT(bg);
    LinearAnimation<Color>* animation = new LinearAnimation<Color>(this, &bg->color, finalColor, time, interpolationFunc);
    animation->Start(track);
    return animation;
}

bool UIControl::IsHiddenForDebug() const
{
    return hiddenForDebug;
}

void UIControl::SetHiddenForDebug(bool hidden)
{
    hiddenForDebug = hidden;
}

void UIControl::SystemOnFocusLost()
{
    SetState(GetState() & ~STATE_FOCUSED);
    PerformEvent(EVENT_FOCUS_LOST, nullptr);
    OnFocusLost();
}

void UIControl::SystemOnFocused()
{
    SetState(GetState() | STATE_FOCUSED);
    PerformEvent(EVENT_FOCUS_SET, nullptr);
    OnFocused();
}

void UIControl::OnFocusLost()
{
}

void UIControl::OnFocused()
{
}

void UIControl::OnTouchOutsideFocus()
{
}

void UIControl::SetSizeFromBg(bool pivotToCenter)
{
    UIControlBackground* bg = GetComponent<UIControlBackground>();
    DVASSERT(bg);
    if (bg)
    {
        SetSize(bg->GetSprite()->GetSize());
    }

    if (pivotToCenter)
    {
        SetPivot(Vector2(0.5f, 0.5f));
    }
}

void UIControl::RegisterInputProcessor()
{
    inputProcessorsCount++;
    if (parent)
    {
        parent->RegisterInputProcessor();
    }
}

void UIControl::RegisterInputProcessors(int32 processorsCount)
{
    inputProcessorsCount += processorsCount;
    if (parent)
    {
        parent->RegisterInputProcessors(processorsCount);
    }
}

void UIControl::UnregisterInputProcessor()
{
    inputProcessorsCount--;
    DVASSERT(inputProcessorsCount >= 0);
    if (parent)
    {
        parent->UnregisterInputProcessor();
    }
}
void UIControl::UnregisterInputProcessors(int32 processorsCount)
{
    inputProcessorsCount -= processorsCount;
    DVASSERT(inputProcessorsCount >= 0);
    if (parent)
    {
        parent->UnregisterInputProcessors(processorsCount);
    }
}

void UIControl::DumpInputs(int32 depthLevel)
{
    String outStr;
    for (int32 i = 0; i < depthLevel; i++)
    {
        outStr += "| ";
    }
    outStr += "\\-";
    outStr += name.c_str();
    if (inputProcessorsCount > 0)
    {
        outStr += " ";
        outStr += Format("%d", inputProcessorsCount);
    }

    if (inputEnabled)
    {
        outStr += " ***";
    }
    Logger::Info("%s", outStr.c_str());
    auto it = children.begin();
    for (; it != children.end(); ++it)
    {
        (*it)->DumpInputs(depthLevel + 1);
    }
}

void UIControl::DumpControls(bool onlyOrphans)
{
    LockGuard<Mutex> lock(controlsListMutex);
    Logger::FrameworkDebug("============================================================");
    Logger::FrameworkDebug("--------------- Currently allocated controls ----------------");

    uint32 allCount = static_cast<uint32>(controlsList.size());
    uint32 orphansCount = 0;
    for (auto control : controlsList)
    {
        if (control->GetParent() == nullptr)
        {
            ++orphansCount;
        }

        if (onlyOrphans && control->GetParent() != nullptr)
        {
            continue;
        }

        Logger::FrameworkDebug("class:\"%s\" name:\"%s\" count:%d", control->GetClassName().c_str(), control->GetName().c_str(), control->GetRetainCount());
    }

    Logger::FrameworkDebug("Total controls count: %d, orphans count: %d", allCount, orphansCount);
    Logger::FrameworkDebug("============================================================");
}

void UIControl::UpdateLayout()
{
    if (scene)
    {
        scene->GetLayoutSystem()->ManualApplyLayout(this);
    }
    else
    {
        GetEngineContext()->uiControlSystem->GetLayoutSystem()->ManualApplyLayout(this);
    }
}

void UIControl::OnSizeChanged()
{
}

/* Components */

void UIControl::AddComponent(UIComponent* component)
{
    DVASSERT(component->GetControl() == nullptr);
    component->SetControl(this);
    components.push_back(SafeRetain(component));
    std::stable_sort(components.begin(), components.end(), [](const UIComponent* left, const UIComponent* right) {
        return left->GetRuntimeType() < right->GetRuntimeType();
    });
    UpdateFamily();

    if (viewState >= eViewState::ACTIVE)
    {
        if (scene)
        {
            scene->RegisterComponent(this, component);
        }
        else
        {
            GetEngineContext()->uiControlSystem->RegisterComponent(this, component);
        }
    }
    SetStyleSheetDirty();
    SetLayoutDirty();
}

void UIControl::InsertComponentAt(UIComponent* component, uint32 index)
{
    uint32 count = family->GetComponentsCount(component->GetRuntimeType());
    if (count == 0 || index >= count)
    {
        AddComponent(component);
    }
    else
    {
        DVASSERT(component->GetControl() == nullptr);
        component->SetControl(this);

        uint32 insertIndex = family->GetComponentIndex(component->GetRuntimeType(), index);
        components.insert(components.begin() + insertIndex, SafeRetain(component));

        UpdateFamily();

        if (viewState >= eViewState::ACTIVE)
        {
            if (scene)
            {
                scene->RegisterComponent(this, component);
            }
            else
            {
                GetEngineContext()->uiControlSystem->RegisterComponent(this, component);
            }
        }

        SetStyleSheetDirty();
        SetLayoutDirty();
    }
}

UIComponent* UIControl::GetComponent(int32 runtimeType, uint32 index) const
{
    uint32 maxCount = family->GetComponentsCount(runtimeType);
    if (index < maxCount)
    {
        return components[family->GetComponentIndex(runtimeType, index)];
    }
    return nullptr;
}

UIComponent* UIControl::GetComponent(const Type* type, uint32 index /*= 0*/) const
{
    ComponentManager* cm = GetEngineContext()->componentManager;
    return GetComponent(cm->GetRuntimeComponentId(type), index);
}

UIComponent* UIControl::GetComponentByName(const String& typeName, uint32 index) const
{
    return UIControlHelpers::GetComponentByName(this, typeName, index);
}

int32 UIControl::GetComponentIndex(const UIComponent* component) const
{
    uint32 count = family->GetComponentsCount(component->GetRuntimeType());
    uint32 index = family->GetComponentIndex(component->GetRuntimeType(), 0);
    for (uint32 i = 0; i < count; i++)
    {
        if (components[index + i] == component)
            return i;
    }
    return -1;
}

UIComponent* UIControl::GetOrCreateComponent(const Type* type, uint32 index)
{
    ComponentManager* cm = GetEngineContext()->componentManager;
    UIComponent* ret = GetComponent(cm->GetRuntimeComponentId(type), index);
    if (!ret)
    {
        DVASSERT(index == 0);
        ret = UIComponent::CreateByType(type);
        if (ret)
        {
            AddComponent(ret);
            ret->Release(); // refCount was increased in AddComponent
        }
    }

    return ret;
}

void UIControl::UpdateFamily()
{
    UIControlFamily::Release(family);
    family = UIControlFamily::GetOrCreate(components);
}

int32 UIControl::TypeToRuntimeType(const Type* type)
{
    return GetEngineContext()->componentManager->GetRuntimeComponentId(type);
}

void UIControl::RemoveAllComponents()
{
    while (!components.empty())
    {
        auto it = components.end() - 1;
        UIComponent* component = *it;

        if (viewState >= eViewState::ACTIVE)
        {
            if (scene)
            {
                scene->UnregisterComponent(this, component);
            }
            else
            {
                GetEngineContext()->uiControlSystem->UnregisterComponent(this, component);
            }
        }

        components.erase(it);

        component->SetControl(nullptr);
        SafeRelease(component);
    }
    UpdateFamily();
    SetStyleSheetDirty();
    SetLayoutDirty();
}

void UIControl::RemoveComponent(const Vector<UIComponent*>::iterator& it)
{
    if (it != components.end())
    {
        UIComponent* component = *it;

        if (viewState >= eViewState::ACTIVE)
        {
            if (scene)
            {
                scene->UnregisterComponent(this, component);
            }
            else
            {
                GetEngineContext()->uiControlSystem->UnregisterComponent(this, component);
            }
        }

        components.erase(it);
        UpdateFamily();
        component->SetControl(nullptr);
        SafeRelease(component);

        SetStyleSheetDirty();
        SetLayoutDirty();
    }
}

void UIControl::RemoveComponent(UIComponent* component)
{
    DVASSERT(component);
    auto it = std::find(components.begin(), components.end(), component);
    RemoveComponent(it);
}

void UIControl::RemoveComponent(int32 runtimeType, uint32 index /*= 0*/)
{
    UIComponent* c = GetComponent(runtimeType, index);
    if (c)
    {
        RemoveComponent(c);
    }
}

void UIControl::RemoveComponent(const Type* componentType, uint32 index)
{
    UIComponent* c = GetComponent(componentType, index);
    if (c)
    {
        RemoveComponent(c);
    }
}

uint32 UIControl::GetComponentCount() const
{
    return static_cast<uint32>(components.size());
}

uint32 UIControl::GetComponentCount(int32 runtimeType) const
{
    return family->GetComponentsCount(runtimeType);
}

UIComponent* UIControl::GetOrCreateComponentByName(const String& typeName, uint32 index)
{
    return UIControlHelpers::GetOrCreateComponentByName(this, typeName, index);
}

uint32 UIControl::GetComponentCount(const Type* type) const
{
    ComponentManager* cm = GetEngineContext()->componentManager;
    return family->GetComponentsCount(cm->GetRuntimeComponentId(type));
}

const Vector<UIComponent*>& UIControl::GetComponents()
{
    return components;
}

/* Components */

/* Styles */

void UIControl::AddClass(const FastName& clazz)
{
    if (classes.AddClass(clazz))
    {
        SetStyleSheetDirty();
    }
}

void UIControl::RemoveClass(const FastName& clazz)
{
    if (classes.RemoveClass(clazz))
    {
        SetStyleSheetDirty();
    }
}

bool UIControl::HasClass(const FastName& clazz) const
{
    return classes.HasClass(clazz);
}

void UIControl::SetTaggedClass(const FastName& tag, const FastName& clazz)
{
    if (classes.SetTaggedClass(tag, clazz))
    {
        SetStyleSheetDirty();
    }
}

FastName UIControl::GetTaggedClass(const FastName& tag) const
{
    return classes.GetTaggedClass(tag);
}

void UIControl::ResetTaggedClass(const FastName& tag)
{
    if (classes.ResetTaggedClass(tag))
    {
        SetStyleSheetDirty();
    }
}

String UIControl::GetClassesAsString() const
{
    return classes.GetClassesAsString();
}

void UIControl::SetClassesFromString(const String& classesStr)
{
    classes.SetClassesFromString(classesStr);
    SetStyleSheetDirty();
}

const UIStyleSheetPropertySet& UIControl::GetLocalPropertySet() const
{
    return localProperties;
}

void UIControl::SetLocalPropertySet(const UIStyleSheetPropertySet& set)
{
    localProperties = set;
}

void UIControl::SetPropertyLocalFlag(uint32 propertyIndex, bool value)
{
    localProperties.set(propertyIndex, value);
    SetStyleSheetDirty();
}

const UIStyleSheetPropertySet& UIControl::GetStyledPropertySet() const
{
    return styledProperties;
}

void UIControl::SetStyledPropertySet(const UIStyleSheetPropertySet& set)
{
    styledProperties = set;
}

bool UIControl::IsStyleSheetInitialized() const
{
    return styleSheetInitialized;
}

void UIControl::SetStyleSheetInitialized()
{
    styleSheetInitialized = true;
}

bool UIControl::IsStyleSheetDirty() const
{
    return styleSheetDirty;
}

void UIControl::SetStyleSheetDirty()
{
    styleSheetDirty = true;
    if (scene)
    {
        scene->GetStyleSheetSystem()->SetDirty();
    }
    else
    {
        GetEngineContext()->uiControlSystem->GetStyleSheetSystem()->SetDirty();
    }
}

void UIControl::ResetStyleSheetDirty()
{
    styleSheetDirty = false;
}

void UIControl::SetLayoutDirty()
{
    layoutDirty = true;
    if (scene)
    {
        scene->GetLayoutSystem()->SetDirty();
    }
    else
    {
        GetEngineContext()->uiControlSystem->GetLayoutSystem()->SetDirty();
    }
}

void UIControl::ResetLayoutDirty()
{
    layoutDirty = false;
    layoutPositionDirty = false;
    layoutOrderDirty = false;
}

void UIControl::SetLayoutPositionDirty()
{
    layoutPositionDirty = true;
    if (scene)
    {
        scene->GetLayoutSystem()->SetDirty();
    }
    else
    {
        GetEngineContext()->uiControlSystem->GetLayoutSystem()->SetDirty();
    }
}

void UIControl::ResetLayoutPositionDirty()
{
    layoutPositionDirty = false;
}

void UIControl::SetLayoutOrderDirty()
{
    layoutOrderDirty = true;
}

void UIControl::ResetLayoutOrderDirty()
{
    layoutOrderDirty = false;
}

void UIControl::SetPackageContext(const RefPtr<UIControlPackageContext>& newPackageContext)
{
    if (packageContext != newPackageContext)
    {
        SetStyleSheetDirty();
    }

    packageContext = newPackageContext;
    for (const auto& child : children)
        child->PropagateParentWithContext(packageContext ? this : parentWithContext);
}

UIControl* UIControl::GetParentWithContext() const
{
    return parentWithContext;
}

void UIControl::PropagateParentWithContext(UIControl* newParentWithContext)
{
    SetStyleSheetDirty();

    parentWithContext = newParentWithContext;
    if (packageContext == nullptr)
    {
        for (const auto& child : children)
        {
            child->PropagateParentWithContext(newParentWithContext);
        }
    }
}

RefPtr<UIControlPackageContext> UIControl::GetPackageContext() const
{
    return packageContext.Valid() ?
    packageContext :
    (parentWithContext ? parentWithContext->GetLocalPackageContext() : RefPtr<UIControlPackageContext>());
}

const RefPtr<UIControlPackageContext>& UIControl::GetLocalPackageContext() const
{
    return packageContext;
}

/* Styles */
}
