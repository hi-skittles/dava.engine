#include "UI/Layouts/Private/AnchorLayoutAlgorithm.h"
#include "UI/Layouts/Private/SizeMeasuringAlgorithm.h"
#include "UI/Layouts/UIAnchorComponent.h"
#include "UI/Layouts/UIAnchorSafeAreaComponent.h"
#include "UI/Layouts/UISizePolicyComponent.h"
#include "UI/UIControl.h"

namespace DAVA
{
AnchorLayoutAlgorithm::AnchorLayoutAlgorithm(Layouter& layouter_)
    : layouter(layouter_)
{
}

AnchorLayoutAlgorithm::~AnchorLayoutAlgorithm() = default;

void AnchorLayoutAlgorithm::Apply(ControlLayoutData& data, Vector2::eAxis axis, bool onlyForIgnoredControls)
{
    Apply(data, axis, onlyForIgnoredControls, data.GetFirstChildIndex(), data.GetLastChildIndex());
}

void AnchorLayoutAlgorithm::Apply(ControlLayoutData& data, Vector2::eAxis axis, bool onlyForIgnoredControls, int32 firstIndex, int32 lastIndex)
{
    Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();
    for (int32 i = firstIndex; i <= lastIndex; i++)
    {
        ControlLayoutData& childData = layoutData[i];
        if (!onlyForIgnoredControls || childData.HaveToSkipControl(false))
        {
            const UISizePolicyComponent* sizeHint = childData.GetControl()->GetComponent<UISizePolicyComponent>();
            if (sizeHint != nullptr && sizeHint->GetPolicyByAxis(axis) == UISizePolicyComponent::PERCENT_OF_PARENT)
            {
                float32 size = data.GetSize(axis) * sizeHint->GetValueByAxis(axis) / 100.0f;
                size = Clamp(size, sizeHint->GetMinValueByAxis(axis), sizeHint->GetMaxValueByAxis(axis));
                childData.SetSize(axis, size);
            }
            else if (sizeHint != nullptr && sizeHint->GetPolicyByAxis(axis) == UISizePolicyComponent::FORMULA)
            {
                SizeMeasuringAlgorithm alg(layouter, childData, axis, sizeHint);
                alg.SetParentSize(data.GetSize(axis));
                alg.SetParentRestSize(data.GetSize(axis));

                float32 size = alg.Calculate();
                if (size < 0.0f)
                {
                    size = 0.0f;
                }

                childData.SetSize(axis, size);
            }

            ApplyAnchor(childData, axis, 0.0f, data.GetSize(axis), layouter.IsRtl(), layouter);
        }
    }
}

void AnchorLayoutAlgorithm::ApplyAnchor(ControlLayoutData& data, Vector2::eAxis axis, float32 min, float32 max, bool isRtl, const Layouter& layouter)
{
    UIAnchorComponent* hint = data.GetControl()->GetComponent<UIAnchorComponent>();
    if (hint != nullptr && hint->IsEnabled())
    {
        UIAnchorSafeAreaComponent* safeArea = data.GetControl()->GetComponent<UIAnchorSafeAreaComponent>();
        float32 leftInset = 0.0f;
        float32 rightInset = 0.0f;
        float32 topInset = 0.0f;
        float32 bottomInset = 0.0f;

        float32 v1 = 0.0f;
        bool v1Enabled = false;

        float32 v2 = 0.0f;
        bool v2Enabled = false;

        float32 v3 = 0.0f;
        bool v3Enabled = false;

        switch (axis)
        {
        case Vector2::AXIS_X:
            if (safeArea)
            {
                if (!FLOAT_EQUAL(layouter.GetSafeAreaInsets().left, 0.0f))
                {
                    if (safeArea->GetLeftInset() == UIAnchorSafeAreaComponent::eInsetType::INSET ||
                        (safeArea->GetLeftInset() == UIAnchorSafeAreaComponent::eInsetType::INSET_ONLY_IF_NOTCH && layouter.IsLeftNotch()))
                    {
                        leftInset = layouter.GetSafeAreaInsets().left + safeArea->GetLeftInsetCorrection();
                    }
                    else if (safeArea->GetLeftInset() == UIAnchorSafeAreaComponent::eInsetType::REVERSE)
                    {
                        leftInset -= layouter.GetSafeAreaInsets().left + safeArea->GetLeftInsetCorrection();
                    }
                }

                if (!FLOAT_EQUAL(layouter.GetSafeAreaInsets().right, 0.0f))
                {
                    if (safeArea->GetRightInset() == UIAnchorSafeAreaComponent::eInsetType::INSET ||
                        (safeArea->GetRightInset() == UIAnchorSafeAreaComponent::eInsetType::INSET_ONLY_IF_NOTCH && layouter.IsRightNotch()))
                    {
                        rightInset = layouter.GetSafeAreaInsets().right + safeArea->GetRightInsetCorrection();
                    }
                    else if (safeArea->GetRightInset() == UIAnchorSafeAreaComponent::eInsetType::REVERSE)
                    {
                        rightInset -= layouter.GetSafeAreaInsets().right + safeArea->GetRightInsetCorrection();
                    }
                }
            }

            v1Enabled = hint->IsLeftAnchorEnabled();
            v1 = hint->GetLeftAnchor() + leftInset;

            v2Enabled = hint->IsHCenterAnchorEnabled();
            v2 = hint->GetHCenterAnchor();

            v3Enabled = hint->IsRightAnchorEnabled();
            v3 = hint->GetRightAnchor() + rightInset;

            if (isRtl && hint->IsUseRtl())
            {
                v1Enabled = hint->IsRightAnchorEnabled();
                v1 = hint->GetRightAnchor() + leftInset;

                v3Enabled = hint->IsLeftAnchorEnabled();
                v3 = hint->GetLeftAnchor() + rightInset;

                v2 = -v2;
            }
            break;

        case Vector2::AXIS_Y:
            if (safeArea)
            {
                if (!FLOAT_EQUAL(layouter.GetSafeAreaInsets().top, 0.0f))
                {
                    if (safeArea->GetTopInset() == UIAnchorSafeAreaComponent::eInsetType::INSET)
                    {
                        topInset = layouter.GetSafeAreaInsets().top + safeArea->GetTopInsetCorrection();
                    }
                    else if (safeArea->GetTopInset() == UIAnchorSafeAreaComponent::eInsetType::REVERSE)
                    {
                        topInset -= layouter.GetSafeAreaInsets().top + safeArea->GetTopInsetCorrection();
                    }
                }

                if (!FLOAT_EQUAL(layouter.GetSafeAreaInsets().bottom, 0.0f))
                {
                    if (safeArea->GetBottomInset() == UIAnchorSafeAreaComponent::eInsetType::INSET)
                    {
                        bottomInset = layouter.GetSafeAreaInsets().bottom + safeArea->GetBottomInsetCorrection();
                    }
                    else if (safeArea->GetBottomInset() == UIAnchorSafeAreaComponent::eInsetType::REVERSE)
                    {
                        bottomInset -= layouter.GetSafeAreaInsets().bottom + safeArea->GetBottomInsetCorrection();
                    }
                }
            }

            v1Enabled = hint->IsTopAnchorEnabled();
            v1 = hint->GetTopAnchor() + topInset;

            v2Enabled = hint->IsVCenterAnchorEnabled();
            v2 = hint->GetVCenterAnchor();

            v3Enabled = hint->IsBottomAnchorEnabled();
            v3 = hint->GetBottomAnchor() + bottomInset;
            break;

        default:
            DVASSERT(false);
            break;
        }

        if (v1Enabled || v2Enabled || v3Enabled)
        {
            float32 parentSize = max - min;

            if (v1Enabled && v3Enabled) // left and right
            {
                data.SetPosition(axis, v1 + min);
                data.SetSize(axis, parentSize - (v1 + v3));
            }
            else if (v1Enabled && v2Enabled) // left and center
            {
                data.SetPosition(axis, v1 + min);
                data.SetSize(axis, parentSize / 2.0f - (v1 - v2));
            }
            else if (v2Enabled && v3Enabled) // center and right
            {
                data.SetPosition(axis, parentSize / 2.0f + v2 + min);
                data.SetSize(axis, parentSize / 2.0f - (v2 + v3));
            }
            else if (v1Enabled) // left
            {
                data.SetPosition(axis, v1 + min);
            }
            else if (v2Enabled) // center
            {
                data.SetPosition(axis, (parentSize - data.GetSize(axis)) / 2.0f + v2 + min);
            }
            else if (v3Enabled) // right
            {
                data.SetPosition(axis, parentSize - (data.GetSize(axis) + v3) + min);
            }
        }
    }
}
}
