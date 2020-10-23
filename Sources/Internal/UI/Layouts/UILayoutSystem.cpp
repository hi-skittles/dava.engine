#include "UILayoutSystem.h"

#include "Concurrency/Thread.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "Engine/Window.h"
#include "Entity/Component.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "UI/Layouts/LayoutFormula.h"
#include "UI/Layouts/Private/Layouter.h"
#include "UI/Layouts/UIAnchorComponent.h"
#include "UI/Layouts/UIFlowLayoutComponent.h"
#include "UI/Layouts/UILayoutIsolationComponent.h"
#include "UI/Layouts/UILayoutSourceRectComponent.h"
#include "UI/Layouts/UILinearLayoutComponent.h"
#include "UI/Layouts/UISizePolicyComponent.h"
#include "UI/Text/UITextComponent.h"
#include "UI/Text/UITextSystem.h"
#include "UI/UIControl.h"
#include "UI/UIControlSystem.h"
#include "UI/UIScreen.h"
#include "UI/UIScreenTransition.h"

namespace DAVA
{
UILayoutSystem::UILayoutSystem()
    : sharedLayouter(std::make_unique<Layouter>())
{
    sharedLayouter->onFormulaProcessed = [this](UIControl* control, Vector2::eAxis axis, const LayoutFormula* formula) {
        formulaProcessed.Emit(control, axis, formula);
    };

    sharedLayouter->onFormulaRemoved = [this](UIControl* control, Vector2::eAxis axis, const LayoutFormula* formula) {
        formulaRemoved.Emit(control, axis, formula);
    };
}

UILayoutSystem::~UILayoutSystem()
{
}

void UILayoutSystem::RegisterSystem()
{
    // Subscribe on visible frame changing only for primary window if it exists
    if (GetPrimaryWindow())
    {
        visibleFrameChangedToken = GetPrimaryWindow()->visibleFrameChanged.Connect([&](Window* w, Rect rect) {
            Rect vr = GetScene()->vcs->ConvertInputToVirtual(rect);
            UpdateVisibilityRect(vr);
        });
        virtualSizeChangedToken = GetScene()->vcs->virtualSizeChanged.Connect([&](const Size2i& size) {
            UpdateVisibilityRect(Rect(0.f, 0.f, static_cast<float32>(size.dx), static_cast<float32>(size.dy)));
            SetPhysicalSafeAreaInsets(physicalLeftSafeAreaInset,
                                      physicalTopSafeAreaInset,
                                      physicalRightSafeAreaInset,
                                      physicalBottomSafeAreaInset,
                                      isLeftNotch,
                                      isRightNotch);
        });
        inputSizeChangedToken = GetScene()->vcs->inputAreaSizeChanged.Connect([&](const Size2i& size) {
            Vector2 s = GetScene()->vcs->ConvertInputToVirtual(Vector2(static_cast<float32>(size.dx), static_cast<float32>(size.dy)));
            UpdateVisibilityRect(Rect(0.f, 0.f, s.x, s.y));
        });
    }
}

void UILayoutSystem::UnregisterSystem()
{
    if (GetPrimaryWindow())
    {
        GetPrimaryWindow()->visibleFrameChanged.Disconnect(visibleFrameChangedToken);
        GetScene()->vcs->virtualSizeChanged.Disconnect(virtualSizeChangedToken);
        GetScene()->vcs->inputAreaSizeChanged.Disconnect(inputSizeChangedToken);
    }
}

void UILayoutSystem::Process(float32 elapsedTime)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::UI_LAYOUT_SYSTEM);

    DVASSERT(Thread::IsMainThread());

    if (!IsAutoupdatesEnabled())
        return;

    CheckDirty();

    if (!needUpdate)
        return;

    if (currentScreen.Valid())
    {
        ProcessControlHierarhy(currentScreen.Get());
    }

    if (popupContainer.Valid())
    {
        ProcessControlHierarhy(popupContainer.Get());
    }
}

void UILayoutSystem::UnregisterControl(UIControl* control)
{
    UISizePolicyComponent* sizePolicyComponent = control->GetComponent<UISizePolicyComponent>();
    if (sizePolicyComponent != nullptr)
    {
        for (int32 axis = Vector2::AXIS_X; axis < Vector2::AXIS_COUNT; axis++)
        {
            LayoutFormula* formula = sizePolicyComponent->GetFormula(axis);
            if (formula != nullptr)
            {
                formula->MarkChanges();
                formulaRemoved.Emit(control, static_cast<Vector2::eAxis>(axis), formula);
            }
        }
    }
}

void UILayoutSystem::UnregisterComponent(UIControl* control, UIComponent* component)
{
    UISizePolicyComponent* sizePolicyComponent = CastIfEqual<UISizePolicyComponent*>(component);
    if (sizePolicyComponent != nullptr)
    {
        for (int32 axis = Vector2::AXIS_X; axis < Vector2::AXIS_COUNT; axis++)
        {
            LayoutFormula* formula = sizePolicyComponent->GetFormula(axis);
            if (formula != nullptr)
            {
                formulaRemoved.Emit(control, static_cast<Vector2::eAxis>(axis), formula);
            }
        }
    }
}

void UILayoutSystem::ForceProcessControl(float32 elapsedTime, UIControl* control)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::UI_LAYOUT_SYSTEM);

    if (!needUpdate && !dirty)
        return;

    ProcessControlHierarhy(control);
}

void UILayoutSystem::SetCurrentScreen(const RefPtr<UIScreen>& screen)
{
    currentScreen = screen;
}

void UILayoutSystem::SetPopupContainer(const RefPtr<UIControl>& _popupContainer)
{
    popupContainer = _popupContainer;
}

bool UILayoutSystem::IsRtl() const
{
    return sharedLayouter->IsRtl();
}

void UILayoutSystem::SetRtl(bool rtl)
{
    sharedLayouter->SetRtl(rtl);
}

void UILayoutSystem::SetPhysicalSafeAreaInsets(float32 left, float32 top, float32 right, float32 bottom, bool isLeftNotch_, bool isRightNotch_)
{
    physicalLeftSafeAreaInset = left;
    physicalTopSafeAreaInset = top;
    physicalRightSafeAreaInset = right;
    physicalBottomSafeAreaInset = bottom;
    isLeftNotch = isLeftNotch_;
    isRightNotch = isRightNotch_;

    VirtualCoordinatesSystem* vcs = GetScene()->vcs;
    sharedLayouter->SetSafeAreaInsets(vcs->ConvertPhysicalToVirtualX(left),
                                      vcs->ConvertPhysicalToVirtualY(top),
                                      vcs->ConvertPhysicalToVirtualX(right),
                                      vcs->ConvertPhysicalToVirtualY(bottom),
                                      isLeftNotch,
                                      isRightNotch);

    if (currentScreen.Valid())
    {
        currentScreen->SetLayoutDirty();
    }
    if (popupContainer.Valid())
    {
        popupContainer->SetLayoutDirty();
    }
}

float32 UILayoutSystem::GetSafeAreaLeftInset() const
{
    VirtualCoordinatesSystem* vcs = GetScene()->vcs;
    return vcs->ConvertPhysicalToVirtualX(physicalLeftSafeAreaInset);
}

float32 UILayoutSystem::GetSafeAreaTopInset() const
{
    VirtualCoordinatesSystem* vcs = GetScene()->vcs;
    return vcs->ConvertPhysicalToVirtualY(physicalTopSafeAreaInset);
}

float32 UILayoutSystem::GetSafeAreaRightInset() const
{
    VirtualCoordinatesSystem* vcs = GetScene()->vcs;
    return vcs->ConvertPhysicalToVirtualX(physicalRightSafeAreaInset);
}

float32 UILayoutSystem::GetSafeAreaBottomInset() const
{
    VirtualCoordinatesSystem* vcs = GetScene()->vcs;
    return vcs->ConvertPhysicalToVirtualY(physicalBottomSafeAreaInset);
}

void UILayoutSystem::ProcessControl(UIControl* control)
{
    bool layoutDirty = control->IsLayoutDirty();
    bool orderDirty = control->IsLayoutOrderDirty();
    bool positionDirty = control->IsLayoutPositionDirty();
    control->ResetLayoutDirty();

    if (layoutDirty || (orderDirty && HaveToLayoutAfterReorder(control)) || (positionDirty && control->GetParent() && control->GetParent()->GetComponent(Type::Instance<UILayoutSourceRectComponent>())))
    {
        UIControl* container = FindNotDependentOnChildrenControl(control);
        sharedLayouter->ApplyLayout(container);

        controlLayouted.Emit(container);
    }
    else if (positionDirty && HaveToLayoutAfterReposition(control))
    {
        UIControl* container = control->GetParent();
        sharedLayouter->ApplyLayoutNonRecursive(container);
        controlLayouted.Emit(container);
    }
}

void UILayoutSystem::ManualApplyLayout(UIControl* control)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::UI_LAYOUT_SYSTEM);
    UIControlSystem* scene = control->GetScene();
    if (scene)
    {
        scene->GetTextSystem()->ForceProcessControl(0.0f, control);
    }
    else
    {
        GetEngineContext()->uiControlSystem->GetTextSystem()->ForceProcessControl(0.0f, control);
    }

    Layouter localLayouter;
    localLayouter.SetRtl(sharedLayouter->IsRtl());
    localLayouter.SetVisibilityRect(sharedLayouter->GetVisibilityRect());
    localLayouter.SetSafeAreaInsets(sharedLayouter->GetSafeAreaInsets(), isLeftNotch, isRightNotch);
    localLayouter.ApplyLayout(control);
}

bool UILayoutSystem::IsAutoupdatesEnabled() const
{
    return autoupdatesEnabled;
}

void UILayoutSystem::SetAutoupdatesEnabled(bool enabled)
{
    autoupdatesEnabled = enabled;
}

UIControl* UILayoutSystem::FindNotDependentOnChildrenControl(UIControl* control) const
{
    UIControl* result = control;
    while (result->GetParent() != nullptr && result->GetComponentCount<UILayoutIsolationComponent>() == 0)
    {
        UISizePolicyComponent* sizePolicy = result->GetParent()->GetComponent<UISizePolicyComponent>();
        if ((sizePolicy != nullptr && (sizePolicy->IsDependsOnChildren(Vector2::AXIS_X) || sizePolicy->IsDependsOnChildren(Vector2::AXIS_Y))) ||
            result->GetParent()->GetComponent(Type::Instance<UILayoutSourceRectComponent>()) != nullptr)
        {
            result = result->GetParent();
        }
        else
        {
            break;
        }
    }

    if (result->GetParent() != nullptr && result->GetComponentCount<UILayoutIsolationComponent>() == 0)
    {
        result = result->GetParent();
    }

    return result;
}

bool UILayoutSystem::HaveToLayoutAfterReorder(const UIControl* control) const
{
    if (control->GetComponentCount<UILinearLayoutComponent>() || control->GetComponentCount<UIFlowLayoutComponent>())
    {
        return true;
    }

    UISizePolicyComponent* policy = control->GetComponent<UISizePolicyComponent>();
    if (policy)
    {
        return policy->IsDependsOnChildren(Vector2::AXIS_X) || policy->IsDependsOnChildren(Vector2::AXIS_Y);
    }

    return false;
}

bool UILayoutSystem::HaveToLayoutAfterReposition(const UIControl* control) const
{
    const UIControl* parent = control->GetParent();
    if (parent == nullptr)
    {
        return false;
    }

    if (control->GetComponentCount<UIAnchorComponent>())
    {
        return true;
    }

    if (parent->GetComponentCount<UILinearLayoutComponent>() || parent->GetComponentCount<UIFlowLayoutComponent>())
    {
        return true;
    }

    return false;
}

void UILayoutSystem::ProcessControlHierarhy(UIControl* control)
{
    ProcessControl(control);

    // TODO: For now game has many places where changes in layouts can
    // change hierarchy of controls. In future client want fix this places,
    // after that this code should be replaced by simple for-each.
    const auto& children = control->GetChildren();
    auto it = children.begin();
    auto endIt = children.end();
    while (it != endIt)
    {
        control->isIteratorCorrupted = false;
        ProcessControlHierarhy(it->Get());
        if (control->isIteratorCorrupted)
        {
            it = children.begin();
            continue;
        }
        ++it;
    }
}

void UILayoutSystem::UpdateVisibilityRect(const Rect& visibilityRect)
{
    sharedLayouter->SetVisibilityRect(visibilityRect);
    if (currentScreen.Valid())
    {
        currentScreen->SetLayoutDirty();
    }
    if (popupContainer.Valid())
    {
        popupContainer->SetLayoutDirty();
    }
}
}
