#include "UI/Layouts/Private/LinearLayoutAlgorithm.h"
#include "UI/Layouts/Private/AnchorLayoutAlgorithm.h"
#include "UI/Layouts/Private/LayoutHelpers.h"
#include "UI/Layouts/Private/SizeMeasuringAlgorithm.h"
#include "UI/Layouts/UISizePolicyComponent.h"
#include "UI/UIControl.h"

namespace DAVA
{
LinearLayoutAlgorithm::LinearLayoutAlgorithm(Layouter& layouter_)
    : layouter(layouter_)
{
}

LinearLayoutAlgorithm::~LinearLayoutAlgorithm() = default;

void LinearLayoutAlgorithm::SetInverse(bool inverse_)
{
    inverse = inverse_;
}

void LinearLayoutAlgorithm::SetSkipInvisible(bool skipInvisible_)
{
    skipInvisible = skipInvisible_;
}

void LinearLayoutAlgorithm::SetPadding(float32 padding_)
{
    initialPadding = padding_;
}

void LinearLayoutAlgorithm::SetSpacing(float32 spacing_)
{
    initialSpacing = spacing_;
}

void LinearLayoutAlgorithm::SetDynamicPadding(bool dynamicPadding_)
{
    dynamicPadding = dynamicPadding_;
}

void LinearLayoutAlgorithm::SetSafeAreaPaddingInset(bool paddingInset_)
{
    safeAreaPaddingInset = paddingInset_;
}

void LinearLayoutAlgorithm::SetDynamicSpacing(bool dynamicSpacing_)
{
    dynamicSpacing = dynamicSpacing_;
}

void LinearLayoutAlgorithm::Apply(ControlLayoutData& data, Vector2::eAxis axis)
{
    if (data.HasChildren())
    {
        Apply(data, axis, data.GetFirstChildIndex(), data.GetLastChildIndex());
    }
}

void LinearLayoutAlgorithm::Apply(ControlLayoutData& data, Vector2::eAxis axis, int32 firstIndex, int32 lastIndex)
{
    DVASSERT(firstIndex <= lastIndex);

    InitializeParams(data, axis, firstIndex, lastIndex);

    if (childrenCount > 0)
    {
        CalculateDependentOnParentSizes(data, axis, firstIndex, lastIndex);
        CalculateDynamicPaddingAndSpaces(data, axis);
        PlaceChildren(data, axis, firstIndex, lastIndex);
    }

    AnchorLayoutAlgorithm anchorAlg(layouter);
    anchorAlg.Apply(data, axis, true, firstIndex, lastIndex);
}

void LinearLayoutAlgorithm::InitializeParams(ControlLayoutData& data, Vector2::eAxis axis, int32 firstIndex, int32 lastIndex)
{
    leadingPadding = initialPadding;
    trailingPadding = initialPadding;

    if (safeAreaPaddingInset)
    {
        if (axis == Vector2::AXIS_X)
        {
            if (inverse)
            {
                leadingPadding += layouter.GetSafeAreaInsets().right;
                trailingPadding += layouter.GetSafeAreaInsets().left;
            }
            else
            {
                leadingPadding += layouter.GetSafeAreaInsets().left;
                trailingPadding += layouter.GetSafeAreaInsets().right;
            }
        }
        else
        {
            if (inverse)
            {
                leadingPadding += layouter.GetSafeAreaInsets().bottom;
                trailingPadding += layouter.GetSafeAreaInsets().top;
            }
            else
            {
                leadingPadding += layouter.GetSafeAreaInsets().top;
                trailingPadding += layouter.GetSafeAreaInsets().bottom;
            }
        }
    }

    spacing = initialSpacing;

    fixedSize = 0.0f;
    totalPercent = 0.0f;
    childrenCount = 0;
    const Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();

    for (int32 i = firstIndex; i <= lastIndex; i++)
    {
        const ControlLayoutData& childData = layoutData[i];
        if (childData.HaveToSkipControl(skipInvisible))
        {
            continue;
        }

        childrenCount++;

        const UISizePolicyComponent* sizeHint = childData.GetControl()->GetComponent<UISizePolicyComponent>();
        if (sizeHint != nullptr && sizeHint->GetPolicyByAxis(axis) == UISizePolicyComponent::PERCENT_OF_PARENT)
        {
            totalPercent += sizeHint->GetValueByAxis(axis);
        }
        else if (sizeHint == nullptr || sizeHint->GetPolicyByAxis(axis) != UISizePolicyComponent::FORMULA)
        {
            fixedSize += childData.GetSize(axis);
        }
    }

    currentSize = data.GetSize(axis);
    spacesCount = childrenCount - 1;
    contentSize = currentSize - leadingPadding - trailingPadding;
    restSize = contentSize - fixedSize - spacesCount * spacing;
}

void LinearLayoutAlgorithm::CalculateDependentOnParentSizes(ControlLayoutData& data, Vector2::eAxis axis, int32 firstIndex, int32 lastIndex)
{
    int32 index = firstIndex;
    Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();
    while (index <= lastIndex)
    {
        ControlLayoutData& childData = layoutData[index];

        bool haveToSkip = childData.HasFlag(ControlLayoutData::FLAG_SIZE_CALCULATED) || childData.HaveToSkipControl(skipInvisible);

        bool needRestart = false;
        if (!haveToSkip)
        {
            bool sizeWasLimited = CalculateChildDependentOnParentSize(childData, axis);
            if (sizeWasLimited)
            {
                needRestart = true;
                childData.SetFlag(ControlLayoutData::FLAG_SIZE_CALCULATED);
            }
        }

        if (needRestart)
        {
            index = firstIndex;
        }
        else
        {
            index++;
        }
    }
}

bool LinearLayoutAlgorithm::CalculateChildDependentOnParentSize(ControlLayoutData& childData, Vector2::eAxis axis)
{
    const UISizePolicyComponent* sizeHint = childData.GetControl()->GetComponent<UISizePolicyComponent>();
    if (sizeHint != nullptr && sizeHint->GetPolicyByAxis(axis) == UISizePolicyComponent::PERCENT_OF_PARENT)
    {
        float32 percents = sizeHint->GetValueByAxis(axis);
        float32 size = 0.0f;
        if (totalPercent > LayoutHelpers::EPSILON)
        {
            size = restSize * percents / Max(totalPercent, 100.0f);
        }

        float32 minSize = sizeHint->GetMinValueByAxis(axis);
        float32 maxSize = sizeHint->GetMaxValueByAxis(axis);
        if (size < minSize || maxSize < size)
        {
            size = Clamp(size, minSize, maxSize);
            childData.SetSize(axis, size);

            restSize -= size;
            totalPercent -= percents;

            return true;
        }
        else
        {
            childData.SetSize(axis, size);
        }
    }
    else if (sizeHint != nullptr && sizeHint->GetPolicyByAxis(axis) == UISizePolicyComponent::FORMULA)
    {
        SizeMeasuringAlgorithm alg(layouter, childData, axis, sizeHint);
        alg.SetParentSize(currentSize);
        alg.SetParentRestSize(restSize);

        float32 size = alg.Calculate();
        restSize -= size;
        childData.SetSize(axis, size);
        return true;
    }

    return false;
}

void LinearLayoutAlgorithm::CalculateDynamicPaddingAndSpaces(ControlLayoutData& data, Vector2::eAxis axis)
{
    if (totalPercent < 100.0f - LayoutHelpers::EPSILON && restSize > LayoutHelpers::EPSILON)
    {
        if (dynamicPadding || (dynamicSpacing && spacesCount > 0))
        {
            int32 cnt = 0;
            if (dynamicPadding)
            {
                cnt = 2;
            }

            if (dynamicSpacing)
            {
                cnt += spacesCount;
            }

            float32 delta = restSize * (1.0f - totalPercent / 100.0f) / cnt;
            if (dynamicPadding)
            {
                leadingPadding += delta;
            }

            if (dynamicSpacing)
            {
                spacing += delta;
            }
        }
    }
}

void LinearLayoutAlgorithm::PlaceChildren(ControlLayoutData& data, Vector2::eAxis axis, int32 firstIndex, int32 lastIndex)
{
    float32 position = leadingPadding;
    if (inverse)
    {
        position = data.GetSize(axis) - leadingPadding;
    }
    Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();
    for (int32 i = firstIndex; i <= lastIndex; i++)
    {
        ControlLayoutData& childData = layoutData[i];
        if (childData.HaveToSkipControl(skipInvisible))
        {
            continue;
        }

        float32 size = childData.GetSize(axis);
        childData.SetPosition(axis, inverse ? position - size : position);

        if (inverse)
        {
            position -= size + spacing;
        }
        else
        {
            position += size + spacing;
        }
    }
}
}
