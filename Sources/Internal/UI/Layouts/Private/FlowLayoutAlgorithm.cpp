#include "FlowLayoutAlgorithm.h"

#include "UI/Layouts/UIFlowLayoutComponent.h"
#include "UI/Layouts/UISizePolicyComponent.h"
#include "UI/Layouts/UIFlowLayoutHintComponent.h"

#include "UI/Layouts/Private/AnchorLayoutAlgorithm.h"
#include "UI/Layouts/Private/SizeMeasuringAlgorithm.h"
#include "UI/Layouts/Private/LayoutHelpers.h"

#include "UI/UIControl.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
struct FlowLayoutAlgorithm::LineInfo
{
    int32 firstIndex;
    int32 lastIndex;
    int32 childrenCount;
    float32 usedSize;

    LineInfo(int32 first_, int32 last_, int32 count_, float32 size_)
        : firstIndex(first_)
        , lastIndex(last_)
        , childrenCount(count_)
        , usedSize(size_)
    {
        DVASSERT(lastIndex >= firstIndex);
        DVASSERT(childrenCount > 0);
    }
};

FlowLayoutAlgorithm::FlowLayoutAlgorithm(Layouter& layouter_)
    : layouter(layouter_)
{
}

FlowLayoutAlgorithm::~FlowLayoutAlgorithm() = default;

void FlowLayoutAlgorithm::Apply(ControlLayoutData& data, Vector2::eAxis axis)
{
    UIFlowLayoutComponent* layout = data.GetControl()->GetComponent<UIFlowLayoutComponent>();
    DVASSERT(layout != nullptr);

    inverse = layout->GetOrientation() == UIFlowLayoutComponent::ORIENTATION_RIGHT_TO_LEFT;
    if (layouter.IsRtl() && layout->IsUseRtl())
        inverse = !inverse;

    skipInvisible = layout->IsSkipInvisibleControls();

    horizontalLeadingPadding = layout->GetHorizontalPadding();
    horizontalTrailingPadding = layout->GetHorizontalPadding();
    if (layout->IsHorizontalSafeAreaPaddingInset())
    {
        if (inverse)
        {
            horizontalLeadingPadding += layouter.GetSafeAreaInsets().right;
            horizontalTrailingPadding += layouter.GetSafeAreaInsets().left;
        }
        else
        {
            horizontalLeadingPadding += layouter.GetSafeAreaInsets().left;
            horizontalTrailingPadding += layouter.GetSafeAreaInsets().right;
        }
    }
    horizontalSpacing = layout->GetHorizontalSpacing();
    dynamicHorizontalPadding = layout->IsDynamicHorizontalPadding();
    dynamicHorizontalInLinePadding = layout->IsDynamicHorizontalInLinePadding();
    dynamicHorizontalSpacing = layout->IsDynamicHorizontalSpacing();

    topPadding = layout->GetVerticalPadding();
    bottomPadding = layout->GetVerticalPadding();
    if (layout->IsVerticalSafeAreaPaddingInset())
    {
        topPadding += layouter.GetSafeAreaInsets().top;
        bottomPadding += layouter.GetSafeAreaInsets().bottom;
    }
    verticalSpacing = layout->GetVerticalSpacing();
    dynamicVerticalPadding = layout->IsDynamicVerticalPadding();
    dynamicVerticalSpacing = layout->IsDynamicVerticalSpacing();

    if (data.HasChildren())
    {
        switch (axis)
        {
        case Vector2::AXIS_X:
            ProcessXAxis(data, layout);
            break;

        case Vector2::AXIS_Y:
            ProcessYAxis(data);
            break;

        default:
            DVASSERT(false);
            break;
        }
    }

    AnchorLayoutAlgorithm anchorAlg(layouter);
    anchorAlg.Apply(data, axis, true, data.GetFirstChildIndex(), data.GetLastChildIndex());
}

void FlowLayoutAlgorithm::ProcessXAxis(ControlLayoutData& data, const UIFlowLayoutComponent* component)
{
    Vector<LineInfo> lines;
    CollectLinesInformation(data, lines);

    if (component->IsDynamicHorizontalPadding())
    {
        FixHorizontalPadding(data, lines);
    }

    for (LineInfo& line : lines)
    {
        LayoutLine(data, line.firstIndex, line.lastIndex, line.childrenCount, line.usedSize);
    }
}

void FlowLayoutAlgorithm::CollectLinesInformation(ControlLayoutData& data, Vector<LineInfo>& lines)
{
    int32 firstIndex = data.GetFirstChildIndex();

    bool newLineBeforeNext = false;
    bool stickItemBeforeNext = false;
    bool stickHardBeforeNext = false;
    BiDiHelper::Direction prevDirection = BiDiHelper::NEUTRAL;
    int32 stickChildrenInLine = 0;
    float32 usedSize = 0.0f;
    Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();

    for (int32 index = data.GetFirstChildIndex(); index <= data.GetLastChildIndex(); index++)
    {
        ControlLayoutData& childData = layoutData[index];
        if (childData.HaveToSkipControl(skipInvisible))
        {
            continue;
        }

        float32 childSize = childData.GetWidth();
        UISizePolicyComponent* sizePolicy = childData.GetControl()->GetComponent<UISizePolicyComponent>();
        if (sizePolicy != nullptr && sizePolicy->GetHorizontalPolicy() == UISizePolicyComponent::PERCENT_OF_PARENT)
        {
            childSize = sizePolicy->GetHorizontalValue() * (data.GetWidth() - horizontalLeadingPadding - horizontalTrailingPadding) / 100.0f;
            childSize = Clamp(childSize, sizePolicy->GetHorizontalMinValue(), sizePolicy->GetHorizontalMaxValue());
            childData.SetSize(Vector2::AXIS_X, childSize);
        }
        else if (sizePolicy != nullptr && sizePolicy->GetHorizontalPolicy() == UISizePolicyComponent::FORMULA)
        {
            SizeMeasuringAlgorithm alg(layouter, childData, Vector2::AXIS_X, sizePolicy);
            alg.SetParentSize(data.GetWidth());
            childSize = alg.Calculate();
            childData.SetSize(Vector2::AXIS_X, childSize);
        }

        bool newLineBeforeThis = newLineBeforeNext;
        newLineBeforeNext = false;
        bool stickItemBeforeThis = stickItemBeforeNext;
        stickItemBeforeNext = false;
        bool stickHardBeforeThis = stickHardBeforeNext;
        stickHardBeforeNext = false;
        BiDiHelper::Direction direction = inverse ? BiDiHelper::RTL : BiDiHelper::LTR;

        UIFlowLayoutHintComponent* hint = childData.GetControl()->GetComponent<UIFlowLayoutHintComponent>();
        if (hint != nullptr)
        {
            newLineBeforeThis |= hint->IsNewLineBeforeThis();
            newLineBeforeNext = hint->IsNewLineAfterThis();
            stickItemBeforeThis |= hint->IsStickItemBeforeThis();
            stickItemBeforeNext = hint->IsStickItemAfterThis();
            stickHardBeforeThis |= hint->IsStickHardBeforeThis();
            stickHardBeforeNext = hint->IsStickHardAfterThis();
            direction = hint->GetContentDirection();
        }

        if (direction != BiDiHelper::NEUTRAL && direction != prevDirection)
        {
            // Skip sticking if previous control's direction is different
            stickItemBeforeThis = false;
            stickHardBeforeThis = false;
        }
        prevDirection = direction;

        if (stickItemBeforeThis)
        {
            childData.SetFlag(ControlLayoutData::FLAG_STICK_THIS);
            if (stickHardBeforeThis)
            {
                childData.SetFlag(ControlLayoutData::FLAG_STICK_HARD);
            }
        }

        if (direction == BiDiHelper::Direction::LTR)
        {
            childData.SetFlag(ControlLayoutData::FLAG_LTR);
        }
        else if (direction == BiDiHelper::Direction::RTL)
        {
            childData.SetFlag(ControlLayoutData::FLAG_RTL);
        }

        if (newLineBeforeThis && index > firstIndex)
        {
            if (stickChildrenInLine > 0)
            {
                lines.emplace_back(LineInfo(firstIndex, index - 1, stickChildrenInLine, usedSize));
            }
            firstIndex = index;
            stickChildrenInLine = 0;
            usedSize = 0.0f;
        }

        float32 restSize = data.GetWidth() - usedSize;
        restSize -= horizontalLeadingPadding + horizontalTrailingPadding + horizontalSpacing * (stickChildrenInLine - 1) + childSize;
        if (!stickItemBeforeThis)
        {
            restSize -= horizontalSpacing;
        }

        if (restSize < -LayoutHelpers::EPSILON)
        {
            if (stickItemBeforeThis && stickHardBeforeThis && index > firstIndex)
            {
                int32 i = index - 1;
                while (i > firstIndex)
                {
                    ControlLayoutData& ld = layoutData[i];
                    usedSize -= ld.GetWidth();
                    if (!ld.HasFlag(ControlLayoutData::FLAG_STICK_HARD))
                    {
                        break;
                    }
                    i--;
                }
                index = i;
                ControlLayoutData& ld = layoutData[index];
                if (!ld.HasFlag(ControlLayoutData::FLAG_STICK_THIS))
                {
                    stickChildrenInLine--;
                }
                childSize = ld.GetWidth();
            }

            if (index > firstIndex)
            {
                if (stickChildrenInLine > 0)
                {
                    lines.emplace_back(LineInfo(firstIndex, index - 1, stickChildrenInLine, usedSize));
                }
                firstIndex = index;
                stickChildrenInLine = 1;
                usedSize = childSize;
            }
            else
            {
                lines.emplace_back(LineInfo(firstIndex, index, 1, childSize));
                firstIndex = index + 1;
                stickChildrenInLine = 0;
                usedSize = 0.0f;
            }
        }
        else
        {
            if (!stickItemBeforeThis || index == firstIndex)
            {
                stickChildrenInLine++;
            }
            usedSize += childSize;
        }
    }

    if (firstIndex <= data.GetLastChildIndex() && stickChildrenInLine > 0)
    {
        lines.emplace_back(LineInfo(firstIndex, data.GetLastChildIndex(), stickChildrenInLine, usedSize));
    }
}

void FlowLayoutAlgorithm::FixHorizontalPadding(ControlLayoutData& data, Vector<LineInfo>& lines)
{
    float32 maxUsedSize = 0.0f;
    for (const LineInfo& line : lines)
    {
        float32 usedLineLize = line.usedSize;
        if (line.childrenCount > 1)
        {
            usedLineLize += (line.childrenCount - 1) * horizontalSpacing;
        }
        maxUsedSize = Max(usedLineLize, maxUsedSize);
    }

    float32 restSize = data.GetWidth() - maxUsedSize;
    restSize -= horizontalLeadingPadding + horizontalTrailingPadding;
    if (restSize >= LayoutHelpers::EPSILON)
    {
        horizontalLeadingPadding += restSize / 2.0f;
        horizontalTrailingPadding += restSize / 2.0f;
    }
}

void FlowLayoutAlgorithm::LayoutLine(ControlLayoutData& data, int32 firstIndex, int32 lastIndex, int32 childrenCount, float32 childrenSize)
{
    float32 leadingPadding = horizontalLeadingPadding;
    float32 trailingPadding = horizontalTrailingPadding;
    float32 spacing = horizontalSpacing;
    CorrectPaddingAndSpacing(leadingPadding, trailingPadding, spacing, dynamicHorizontalInLinePadding, dynamicHorizontalSpacing, data.GetWidth() - childrenSize, childrenCount);

    float32 position = leadingPadding;
    if (inverse)
    {
        position = data.GetWidth() - leadingPadding;
    }
    int32 realLastIndex = -1;

    // Sort controls in line by direction hint
    List<uint32> order;
    SortLineItemsByContentDirection(firstIndex, lastIndex, order, realLastIndex);

    // Layout controls in correct order
    bool first = true;
    bool prevSameDirection = true;
    bool prevSpace = false;
    bool stickNext = false;
    Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();

    for (uint32 i : order)
    {
        ControlLayoutData& childData = layoutData[i];

        // Current item sticks with previous
        bool stickThis = childData.HasFlag(ControlLayoutData::FLAG_STICK_THIS);
        // Curent item's direction same as layout direction
        bool ltr = childData.HasFlag(ControlLayoutData::FLAG_LTR);
        bool rtl = childData.HasFlag(ControlLayoutData::FLAG_RTL);
        bool neutral = !rtl && !ltr;
        bool sameDirection = neutral || (!inverse && ltr) || (inverse && rtl);
        // Current item's direction same as previous item's direction
        bool pairedDirection = sameDirection == prevSameDirection;

        if (first)
        {
            if (stickThis && !sameDirection)
            {
                // Skip space
                stickNext = true;
            }
        }
        else
        {
            if (stickThis && sameDirection)
            {
                // Skip space
                stickNext = false;
            }
            else
            {
                if (stickNext && pairedDirection)
                {
                    // Skip space
                }
                else
                {
                    if (inverse)
                    {
                        position -= spacing;
                    }
                    else
                    {
                        position += spacing;
                    }
                }
                stickNext = stickThis && !sameDirection;
            }
        }

        float32 size = childData.GetWidth();
        childData.SetPosition(Vector2::AXIS_X, inverse ? position - size : position);

        if (inverse)
        {
            position -= size;
        }
        else
        {
            position += size;
        }

        first = false;
        prevSameDirection = sameDirection;
    }

    DVASSERT(realLastIndex != -1);
    layoutData[realLastIndex].SetFlag(ControlLayoutData::FLAG_LAST_IN_LINE);
}

void FlowLayoutAlgorithm::ProcessYAxis(ControlLayoutData& data)
{
    CalculateVerticalDynamicPaddingAndSpaces(data);

    float32 lineHeight = 0.0f;
    float32 y = topPadding;
    int32 firstIndex = data.GetFirstChildIndex();
    const Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();

    for (int32 index = data.GetFirstChildIndex(); index <= data.GetLastChildIndex(); index++)
    {
        const ControlLayoutData& childData = layoutData[index];
        if (childData.HaveToSkipControl(skipInvisible))
        {
            continue;
        }

        lineHeight = Max(lineHeight, childData.GetHeight());

        if (childData.HasFlag(ControlLayoutData::FLAG_LAST_IN_LINE))
        {
            LayoutLineVertically(data, firstIndex, index, y, y + lineHeight);
            y += lineHeight + verticalSpacing;
            lineHeight = 0;
            firstIndex = index + 1;
        }
    }
}

void FlowLayoutAlgorithm::CalculateVerticalDynamicPaddingAndSpaces(ControlLayoutData& data)
{
    if (dynamicVerticalPadding || dynamicVerticalSpacing)
    {
        int32 linesCount = 0;
        float32 contentSize = 0.0f;
        float32 lineHeight = 0.0f;
        const Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();

        for (int32 index = data.GetFirstChildIndex(); index <= data.GetLastChildIndex(); index++)
        {
            const ControlLayoutData& childData = layoutData[index];
            if (childData.HaveToSkipControl(skipInvisible))
            {
                continue;
            }

            lineHeight = Max(lineHeight, childData.GetHeight());

            if (childData.HasFlag(ControlLayoutData::FLAG_LAST_IN_LINE))
            {
                linesCount++;
                contentSize += lineHeight;
                lineHeight = 0.0f;
            }
        }

        float32 restSize = data.GetHeight() - contentSize;
        CorrectPaddingAndSpacing(topPadding, bottomPadding, verticalSpacing, dynamicVerticalPadding, dynamicVerticalSpacing, restSize, linesCount);
    }
}

void FlowLayoutAlgorithm::LayoutLineVertically(ControlLayoutData& data, int32 firstIndex, int32 lastIndex, float32 top, float32 bottom)
{
    Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();
    for (int32 index = firstIndex; index <= lastIndex; index++)
    {
        ControlLayoutData& childData = layoutData[index];
        if (childData.HaveToSkipControl(skipInvisible))
        {
            continue;
        }
        UISizePolicyComponent* sizePolicy = childData.GetControl()->GetComponent<UISizePolicyComponent>();
        float32 childSize = childData.GetHeight();
        if (sizePolicy)
        {
            if (sizePolicy->GetVerticalPolicy() == UISizePolicyComponent::PERCENT_OF_PARENT)
            {
                childSize = (bottom - top) * sizePolicy->GetVerticalValue() / 100.0f;
                childSize = Clamp(childSize, sizePolicy->GetVerticalMinValue(), sizePolicy->GetVerticalMaxValue());
                childData.SetSize(Vector2::AXIS_Y, childSize);
            }
            else if (sizePolicy != nullptr && sizePolicy->GetVerticalPolicy() == UISizePolicyComponent::FORMULA)
            {
                SizeMeasuringAlgorithm alg(layouter, childData, Vector2::AXIS_Y, sizePolicy);
                alg.SetParentSize(data.GetHeight());
                alg.SetParentLineSize(bottom - top);
                childData.SetSize(Vector2::AXIS_Y, alg.Calculate());
            }
        }

        childData.SetPosition(Vector2::AXIS_Y, top);
        AnchorLayoutAlgorithm::ApplyAnchor(childData, Vector2::AXIS_Y, top, bottom, false, layouter);
    }
}

void FlowLayoutAlgorithm::CorrectPaddingAndSpacing(float32& leadingPadding, float32& trailingPadding, float32& spacing, bool dynamicPadding, bool dynamicSpacing, float32 restSize, int32 childrenCount)
{
    if (childrenCount > 0)
    {
        int32 spacesCount = childrenCount - 1;
        restSize -= leadingPadding + trailingPadding;
        restSize -= spacing * spacesCount;
        if (restSize > LayoutHelpers::EPSILON)
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

                float32 delta = restSize / cnt;
                if (dynamicPadding)
                {
                    leadingPadding += delta;
                    trailingPadding += delta;
                }

                if (dynamicSpacing)
                {
                    spacing += delta;
                }
            }
        }
    }
}

void FlowLayoutAlgorithm::SortLineItemsByContentDirection(int32 firstIndex, int32 lastIndex, List<uint32>& order, int32& realLastIndex)
{
    order.clear();
    auto lastIt = order.end();
    BiDiHelper::Direction lastDir = BiDiHelper::Direction::NEUTRAL;
    const Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();
    for (int32 i = firstIndex; i <= lastIndex; i++)
    {
        const ControlLayoutData& childData = layoutData[i];
        if (childData.HaveToSkipControl(skipInvisible))
        {
            continue;
        }

        BiDiHelper::Direction dir = BiDiHelper::Direction::NEUTRAL;
        if (childData.HasFlag(ControlLayoutData::FLAG_LTR))
        {
            dir = BiDiHelper::Direction::LTR;
        }
        else if (childData.HasFlag(ControlLayoutData::FLAG_RTL))
        {
            dir = BiDiHelper::Direction::RTL;
        }

        if (!order.empty() && lastDir == dir)
        {
            switch (dir)
            {
            case BiDiHelper::Direction::NEUTRAL:
                lastIt = order.insert(order.end(), i);
                break;
            case BiDiHelper::Direction::LTR:
                lastIt = order.insert(inverse ? lastIt : order.end(), i);
                break;
            case BiDiHelper::Direction::RTL:
                lastIt = order.insert(inverse ? order.end() : lastIt, i);
                break;
            default:
                DVASSERT(false);
            }
        }
        else
        {
            lastIt = order.insert(order.end(), i);
        }

        lastDir = dir;
        realLastIndex = i;
    }
}
}
