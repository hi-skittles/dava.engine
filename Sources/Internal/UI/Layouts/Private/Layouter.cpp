#include "Layouter.h"
#include "UI/Layouts/LayoutFormula.h"
#include "UI/Layouts/Private/AnchorLayoutAlgorithm.h"
#include "UI/Layouts/Private/FlowLayoutAlgorithm.h"
#include "UI/Layouts/Private/LinearLayoutAlgorithm.h"
#include "UI/Layouts/Private/SizeMeasuringAlgorithm.h"
#include "UI/Layouts/UIAnchorComponent.h"
#include "UI/Layouts/UIFlowLayoutComponent.h"
#include "UI/Layouts/UILayoutIsolationComponent.h"
#include "UI/Layouts/UILinearLayoutComponent.h"
#include "UI/Layouts/UISizePolicyComponent.h"
#include "UI/UIControl.h"

namespace DAVA
{
void Layouter::ApplyLayout(UIControl* control)
{
    CollectControls(control, true);

    ProcessAxis(Vector2::AXIS_X, true);
    ProcessAxis(Vector2::AXIS_Y, true);

    ApplySizesAndPositions();

    layoutData.clear();
}

void Layouter::ApplyLayoutNonRecursive(UIControl* control)
{
    CollectControls(control, false);

    ProcessAxis(Vector2::AXIS_X, false);
    ProcessAxis(Vector2::AXIS_Y, false);

    ApplyPositions();

    layoutData.clear();
}

void Layouter::CollectControls(UIControl* control, bool recursive)
{
    layoutData.clear();
    layoutData.emplace_back(ControlLayoutData(control));
    CollectControlChildren(control, -1, 0, recursive);
}

void Layouter::CollectControlChildren(UIControl* control, int32 parentIndex, int32 index, bool recursive)
{
    int32 childIndex = static_cast<int32>(layoutData.size());
    const auto& children = control->GetChildren();

    int32 childrenCount = 0;

    for (const auto& child : children)
    {
        if (child->GetComponentCount<UILayoutIsolationComponent>() == 0)
        {
            layoutData.emplace_back(ControlLayoutData(child.Get()));
            childrenCount++;
        }
    }

    layoutData[index].SetParentIndex(parentIndex);
    layoutData[index].SetFirstChildIndex(childIndex);
    layoutData[index].SetLastChildIndex(childIndex + childrenCount - 1);

    if (recursive)
    {
        for (const auto& child : children)
        {
            if (child->GetComponentCount<UILayoutIsolationComponent>() == 0)
            {
                CollectControlChildren(child.Get(), index, childIndex, recursive);
                childIndex++;
            }
        }
    }
}

void Layouter::ProcessAxis(Vector2::eAxis axis, bool processSizes)
{
    if (processSizes)
    {
        DoMeasurePhase(axis);
    }
    DoLayoutPhase(axis);
}

void Layouter::DoMeasurePhase(Vector2::eAxis axis)
{
    int32 lastIndex = static_cast<int32>(layoutData.size() - 1);
    for (int32 index = lastIndex; index >= 0; index--)
    {
        UISizePolicyComponent* sizePolicy = layoutData[index].GetControl()->GetComponent<UISizePolicyComponent>();
        if (sizePolicy != nullptr)
        {
            SizeMeasuringAlgorithm(*this, layoutData[index], axis, sizePolicy).Apply();
        }
    }
}

void Layouter::DoLayoutPhase(Vector2::eAxis axis)
{
    for (auto it = layoutData.begin(); it != layoutData.end(); ++it)
    {
        UIFlowLayoutComponent* flowLayoutComponent = it->GetControl()->GetComponent<UIFlowLayoutComponent>();
        if (flowLayoutComponent && flowLayoutComponent->IsEnabled())
        {
            FlowLayoutAlgorithm(*this).Apply(*it, axis);
        }
        else
        {
            UILinearLayoutComponent* linearLayoutComponent = it->GetControl()->GetComponent<UILinearLayoutComponent>();
            if (linearLayoutComponent != nullptr && linearLayoutComponent->IsEnabled() && linearLayoutComponent->GetAxis() == axis)
            {
                LinearLayoutAlgorithm alg(*this);

                bool inverse = linearLayoutComponent->IsInverse();
                if (isRtl && linearLayoutComponent->IsUseRtl() && linearLayoutComponent->GetAxis() == Vector2::AXIS_X)
                {
                    inverse = !inverse;
                }
                alg.SetInverse(inverse);
                alg.SetSkipInvisible(linearLayoutComponent->IsSkipInvisibleControls());

                alg.SetPadding(linearLayoutComponent->GetPadding());
                alg.SetSpacing(linearLayoutComponent->GetSpacing());

                alg.SetDynamicPadding(linearLayoutComponent->IsDynamicPadding());
                alg.SetSafeAreaPaddingInset(linearLayoutComponent->IsSafeAreaPaddingInset());
                alg.SetDynamicSpacing(linearLayoutComponent->IsDynamicSpacing());

                alg.Apply(*it, axis);
            }
            else
            {
                AnchorLayoutAlgorithm(*this).Apply(*it, axis, false);
            }
        }

        UISizePolicyComponent* sizePolicy = (*it).GetControl()->GetComponent<UISizePolicyComponent>();
        if (sizePolicy != nullptr)
        {
            LayoutFormula* formula = sizePolicy->GetFormula(axis);
            if (formula != nullptr && formula->HasChanges())
            {
                formula->ResetChanges();
                if (formula->IsEmpty())
                {
                    if (onFormulaRemoved)
                    {
                        onFormulaRemoved(sizePolicy->GetControl(), axis, formula);
                    }
                    sizePolicy->RemoveFormula(axis);
                }
                else
                {
                    if (onFormulaProcessed)
                    {
                        onFormulaProcessed(sizePolicy->GetControl(), axis, formula);
                    }
                }
            }
        }
    }
}

void Layouter::ApplySizesAndPositions()
{
    for (ControlLayoutData& data : layoutData)
    {
        data.ApplyLayoutToControl();
    }
}

void Layouter::ApplyPositions()
{
    for (ControlLayoutData& data : layoutData)
    {
        data.ApplyOnlyPositionLayoutToControl();
    }
}

void Layouter::SetRtl(bool rtl)
{
    isRtl = rtl;
}

bool Layouter::IsLeftNotch() const
{
    return isLeftNotch;
}

bool Layouter::IsRightNotch() const
{
    return isRightNotch;
}

const LayoutMargins& Layouter::GetSafeAreaInsets() const
{
    return safeAreaInsets;
}

void Layouter::SetSafeAreaInsets(float32 left, float32 top, float32 right, float32 bottom, bool isLeftNotch_, bool isRightNotch_)
{
    safeAreaInsets.left = left;
    safeAreaInsets.top = top;
    safeAreaInsets.right = right;
    safeAreaInsets.bottom = bottom;
    isLeftNotch = isLeftNotch_;
    isRightNotch = isRightNotch_;
}

void Layouter::SetSafeAreaInsets(const LayoutMargins& insets, bool isLeftNotch_, bool isRightNotch_)
{
    safeAreaInsets = insets;
    isLeftNotch = isLeftNotch_;
    isRightNotch = isRightNotch_;
}

void Layouter::SetVisibilityRect(const Rect& r)
{
    visibilityRect = r;
}
}
