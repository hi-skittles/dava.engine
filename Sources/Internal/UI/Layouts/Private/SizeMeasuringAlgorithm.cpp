#include "SizeMeasuringAlgorithm.h"

#include "Reflection/ReflectionRegistrator.h"
#include "UI/Layouts/LayoutFormula.h"
#include "UI/Layouts/UIFlowLayoutComponent.h"
#include "UI/Layouts/UIFlowLayoutHintComponent.h"
#include "UI/Layouts/UILinearLayoutComponent.h"
#include "UI/Layouts/UISizePolicyComponent.h"
#include "UI/Text/UITextComponent.h"

#include "UI/UIControl.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(SizeMeasuringAlgorithm)
{
    ReflectionRegistrator<SizeMeasuringAlgorithm>::Begin()
    .Field("childrenSum", &SizeMeasuringAlgorithm::CalculateChildrenSum, nullptr)
    .Field("maxChild", &SizeMeasuringAlgorithm::CalculateMaxChild, nullptr)
    .Field("firstChild", &SizeMeasuringAlgorithm::CalculateFirstChild, nullptr)
    .Field("lastChild", &SizeMeasuringAlgorithm::CalculateLastChild, nullptr)
    .Field("content", &SizeMeasuringAlgorithm::CalculateContent, nullptr)
    .Field("parent", &SizeMeasuringAlgorithm::parentSize)
    .Field("parentRest", &SizeMeasuringAlgorithm::parentRestSize)
    .Field("parentLine", &SizeMeasuringAlgorithm::parentLineSize)
    .Field("minLimit", &SizeMeasuringAlgorithm::GetMinLimit, nullptr)
    .Field("maxLimit", &SizeMeasuringAlgorithm::GetMaxLimit, nullptr)
    .Field("value", &SizeMeasuringAlgorithm::GetValue, nullptr)
    .Field("visibilityMargins", &SizeMeasuringAlgorithm::CalculateVisibilityMargins, nullptr)
    .Field("safeAreaInsets", &SizeMeasuringAlgorithm::GetSafeAreaInsets, nullptr)
    .Method("min", &SizeMeasuringAlgorithm::Min)
    .Method("max", &SizeMeasuringAlgorithm::Max)
    .Method("clamp", &SizeMeasuringAlgorithm::Clamp)
    .End();
}

SizeMeasuringAlgorithm::SizeMeasuringAlgorithm(Layouter& layouter_, ControlLayoutData& data_, Vector2::eAxis axis_, const UISizePolicyComponent* sizePolicy_)
    : layouter(layouter_)
    , data(data_)
    , axis(axis_)
    , sizePolicy(sizePolicy_)
{
}

SizeMeasuringAlgorithm::~SizeMeasuringAlgorithm()
{
}

void SizeMeasuringAlgorithm::SetParentSize(float32 parentSize_)
{
    parentSize = parentSize_;
}

void SizeMeasuringAlgorithm::SetParentRestSize(float32 restSize_)
{
    parentRestSize = restSize_;
}

void SizeMeasuringAlgorithm::SetParentLineSize(float32 size)
{
    parentLineSize = size;
}

void SizeMeasuringAlgorithm::Apply()
{
    UISizePolicyComponent::eSizePolicy policy = sizePolicy->GetPolicyByAxis(axis);
    if (policy != UISizePolicyComponent::IGNORE_SIZE && policy != UISizePolicyComponent::PERCENT_OF_PARENT)
    {
        data.SetSize(axis, Calculate());
    }
}

float32 SizeMeasuringAlgorithm::Calculate()
{
    linearLayout = nullptr;
    flowLayout = data.GetControl()->GetComponent<UIFlowLayoutComponent>();

    skipInvisible = false;

    if (flowLayout && flowLayout->IsEnabled())
    {
        skipInvisible = flowLayout->IsSkipInvisibleControls();
    }
    else
    {
        linearLayout = data.GetControl()->GetComponent<UILinearLayoutComponent>();
        if (linearLayout != nullptr && linearLayout->IsEnabled())
        {
            skipInvisible = linearLayout->IsSkipInvisibleControls();
        }
    }

    float32 value = 0.0f;
    UISizePolicyComponent::eSizePolicy policy = sizePolicy->GetPolicyByAxis(axis);
    switch (policy)
    {
    case UISizePolicyComponent::IGNORE_SIZE:
    case UISizePolicyComponent::PERCENT_OF_PARENT:
        // do nothing
        break;

    case UISizePolicyComponent::FIXED_SIZE:
        value = CalculateFixedSize();
        break;

    case UISizePolicyComponent::PERCENT_OF_CHILDREN_SUM:
        value = CalculateChildrenSum();
        break;

    case UISizePolicyComponent::PERCENT_OF_MAX_CHILD:
        value = CalculateMaxChild();
        break;

    case UISizePolicyComponent::PERCENT_OF_FIRST_CHILD:
        value = CalculateFirstChild();
        break;

    case UISizePolicyComponent::PERCENT_OF_LAST_CHILD:
        value = CalculateLastChild();
        break;

    case UISizePolicyComponent::PERCENT_OF_CONTENT:
        value = CalculateContent();
        break;

    case UISizePolicyComponent::FORMULA:
    {
        LayoutFormula* formula = sizePolicy->GetFormula(axis);
        if (formula != nullptr)
        {
            value = formula->Calculate(Reflection::Create(ReflectedObject(this)));
        }
    }
    break;

    default:
        DVASSERT(false);
        break;
    }

    if (policy == UISizePolicyComponent::PERCENT_OF_CHILDREN_SUM ||
        policy == UISizePolicyComponent::PERCENT_OF_MAX_CHILD ||
        policy == UISizePolicyComponent::PERCENT_OF_FIRST_CHILD ||
        policy == UISizePolicyComponent::PERCENT_OF_LAST_CHILD ||
        policy == UISizePolicyComponent::PERCENT_OF_CONTENT)
    {
        float32 valueWithPadding = value + GetLayoutPadding();
        float32 percentedValue = valueWithPadding * sizePolicy->GetValueByAxis(axis) / 100.0f;

        value = ClampValue(percentedValue);
    }

    return DAVA::Max(value, 0.0f);
}

float32 SizeMeasuringAlgorithm::CalculateFixedSize() const
{
    return ClampValue(sizePolicy->GetValueByAxis(axis));
}

float32 SizeMeasuringAlgorithm::CalculateChildrenSum() const
{
    if (flowLayout && flowLayout->IsEnabled())
    {
        switch (axis)
        {
        case Vector2::AXIS_X:
            return CalculateHorizontalFlowLayoutChildrenSum();

        case Vector2::AXIS_Y:
            return CalculateVerticalFlowLayoutChildrenSum();

        default:
            DVASSERT(false);
            return 0.0f;
        }
    }
    else
    {
        return CalculateDefaultChildrenSum();
    }
}

float32 SizeMeasuringAlgorithm::CalculateDefaultChildrenSum() const
{
    float32 value = 0;
    int32 processedChildrenCount = 0;
    const Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();

    bool newLineBeforeNext = false;
    for (int32 i = data.GetFirstChildIndex(); i <= data.GetLastChildIndex(); i++)
    {
        const ControlLayoutData& childData = layoutData[i];
        if (!childData.HaveToSkipControl(skipInvisible))
        {
            bool newLineBeforeThis = newLineBeforeNext;
            newLineBeforeNext = false;
            UIFlowLayoutHintComponent* hint = childData.GetControl()->GetComponent<UIFlowLayoutHintComponent>();
            if (hint != nullptr)
            {
                newLineBeforeThis |= hint->IsNewLineBeforeThis();
                newLineBeforeNext = hint->IsNewLineAfterThis();
            }

            processedChildrenCount++;
            value += GetSize(childData);
        }
    }

    if (linearLayout && linearLayout->IsEnabled() && axis == linearLayout->GetAxis() && processedChildrenCount > 0)
    {
        value += linearLayout->GetSpacing() * (processedChildrenCount - 1);
    }

    return value;
}

float32 SizeMeasuringAlgorithm::CalculateHorizontalFlowLayoutChildrenSum() const
{
    DVASSERT(flowLayout && flowLayout->IsEnabled());

    float32 lineWidth = 0.0f;
    float32 maxWidth = 0.0f;
    bool newLineBeforeNext = false;
    bool firstInLine = true;
    const Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();

    for (int32 i = data.GetFirstChildIndex(); i <= data.GetLastChildIndex(); i++)
    {
        const ControlLayoutData& childData = layoutData[i];
        if (!childData.HaveToSkipControl(skipInvisible))
        {
            float32 childSize = childData.GetWidth();
            UISizePolicyComponent* sizePolicy = childData.GetControl()->GetComponent<UISizePolicyComponent>();
            if (sizePolicy != nullptr && sizePolicy->GetHorizontalPolicy() == UISizePolicyComponent::PERCENT_OF_PARENT)
            {
                childSize = sizePolicy->GetHorizontalMinValue();
            }

            bool newLineBeforeThis = newLineBeforeNext;
            newLineBeforeNext = false;
            UIFlowLayoutHintComponent* hint = childData.GetControl()->GetComponent<UIFlowLayoutHintComponent>();
            if (hint != nullptr)
            {
                newLineBeforeThis |= hint->IsNewLineBeforeThis();
                newLineBeforeNext = hint->IsNewLineAfterThis();
            }

            if (newLineBeforeThis)
            {
                maxWidth = DAVA::Max(maxWidth, lineWidth);
                lineWidth = 0.0f;
                firstInLine = true;
            }

            lineWidth += childSize;
            if (!firstInLine)
            {
                lineWidth += flowLayout->GetHorizontalSpacing();
            }
            firstInLine = false;
        }
    }

    maxWidth = DAVA::Max(maxWidth, lineWidth);
    return maxWidth;
}

float32 SizeMeasuringAlgorithm::CalculateVerticalFlowLayoutChildrenSum() const
{
    DVASSERT(flowLayout && flowLayout->IsEnabled());

    float32 value = 0;
    int32 linesCount = 0;
    float32 lineHeight = 0;
    Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();

    for (int32 index = data.GetFirstChildIndex(); index <= data.GetLastChildIndex(); index++)
    {
        ControlLayoutData& childData = layoutData[index];
        if (childData.HaveToSkipControl(skipInvisible))
            continue;

        lineHeight = DAVA::Max(lineHeight, childData.GetHeight());

        if (childData.HasFlag(ControlLayoutData::FLAG_LAST_IN_LINE))
        {
            value += lineHeight;
            linesCount++;
            lineHeight = 0;
        }
    }
    if (linesCount > 0)
    {
        value += flowLayout->GetVerticalSpacing() * (linesCount - 1);
    }

    return value;
}

float32 SizeMeasuringAlgorithm::CalculateMaxChild() const
{
    float32 value = 0.0f;
    const Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();
    for (int32 i = data.GetFirstChildIndex(); i <= data.GetLastChildIndex(); i++)
    {
        const ControlLayoutData& childData = layoutData[i];
        if (!childData.HaveToSkipControl(skipInvisible))
        {
            value = DAVA::Max(value, GetSize(childData));
        }
    }

    return value;
}

float32 SizeMeasuringAlgorithm::CalculateFirstChild() const
{
    float32 value = 0.0f;
    const Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();
    for (int32 i = data.GetFirstChildIndex(); i <= data.GetLastChildIndex(); i++)
    {
        const ControlLayoutData& childData = layoutData[i];
        if (!childData.HaveToSkipControl(skipInvisible))
        {
            value = GetSize(childData);
            break;
        }
    }

    return value;
}

float32 SizeMeasuringAlgorithm::CalculateLastChild() const
{
    float32 value = 0.0f;
    const Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();
    for (int32 i = data.GetLastChildIndex(); i >= data.GetFirstChildIndex(); i--)
    {
        const ControlLayoutData& childData = layoutData[i];
        if (!childData.HaveToSkipControl(skipInvisible))
        {
            value = GetSize(childData);
            break;
        }
    }

    return value;
}

float32 SizeMeasuringAlgorithm::CalculateContent() const
{
    Vector2 constraints(-1.0f, -1.0f);
    if (IsHeightDependsOnWidth() && axis == Vector2::AXIS_Y)
    {
        constraints.x = data.GetSize(Vector2::AXIS_X);
    }

    return GetContentPreferredSize(constraints).data[axis];
}

Vector2 SizeMeasuringAlgorithm::GetContentPreferredSize(const Vector2& constraints) const
{
    UIControl* control = data.GetControl();
    UITextComponent* txt = control->GetComponent<UITextComponent>();
    if (txt)
    {
        DVASSERT(!txt->IsModified());
        return txt->GetLink()->GetTextBlock()->GetPreferredSizeForWidth(constraints.x);
    }
    UIControlBackground* bg = control->GetComponent<UIControlBackground>();
    if (bg != nullptr && bg->GetSprite() != nullptr)
    {
        if (constraints.dx > 0)
        {
            Vector2 size;
            size.dx = constraints.dx;
            size.dy = bg->GetSprite()->GetHeight() * size.dx / bg->GetSprite()->GetWidth();
            return size;
        }
        else
        {
            return bg->GetSprite()->GetSize();
        }
    }
    return Vector2(0.0f, 0.0f);
}

bool SizeMeasuringAlgorithm::IsHeightDependsOnWidth() const
{
    UIControl* control = data.GetControl();
    UITextComponent* txt = control->GetComponent<UITextComponent>();
    if (txt)
    {
        return txt->GetMultiline() != UITextComponent::eTextMultiline::MULTILINE_DISABLED;
    }
    UIControlBackground* bg = control->GetComponent<UIControlBackground>();
    if (bg == nullptr || bg->GetSprite() == nullptr)
    {
        return false;
    }

    UIControlBackground::eDrawType dt = bg->GetDrawType();
    return dt == UIControlBackground::DRAW_SCALE_PROPORTIONAL || dt == UIControlBackground::DRAW_SCALE_PROPORTIONAL_ONE;
}

float32 SizeMeasuringAlgorithm::GetSize(const ControlLayoutData& data) const
{
    UISizePolicyComponent* sizePolicy = data.GetControl()->GetComponent<UISizePolicyComponent>();
    if (sizePolicy && sizePolicy->GetPolicyByAxis(axis) == UISizePolicyComponent::PERCENT_OF_PARENT)
    {
        return sizePolicy->GetMinValueByAxis(axis);
    }
    return data.GetSize(axis);
}

float32 SizeMeasuringAlgorithm::GetLayoutPadding() const
{
    if (flowLayout && flowLayout->IsEnabled())
    {
        float32 padding = flowLayout->GetPaddingByAxis(axis) * 2.0f;
        if (axis == Vector2::AXIS_X && flowLayout->IsHorizontalSafeAreaPaddingInset())
        {
            padding += layouter.GetSafeAreaInsets().left + layouter.GetSafeAreaInsets().right;
        }
        else if (axis == Vector2::AXIS_Y && flowLayout->IsVerticalSafeAreaPaddingInset())
        {
            padding += layouter.GetSafeAreaInsets().top + layouter.GetSafeAreaInsets().bottom;
        }

        return padding;
    }
    else if (linearLayout != nullptr && linearLayout->IsEnabled() && linearLayout->GetAxis() == axis)
    {
        float32 padding = linearLayout->GetPadding() * 2.0f;
        if (linearLayout->IsSafeAreaPaddingInset())
        {
            if (axis == Vector2::AXIS_X)
            {
                padding += layouter.GetSafeAreaInsets().left + layouter.GetSafeAreaInsets().right;
            }
            else
            {
                padding += layouter.GetSafeAreaInsets().top + layouter.GetSafeAreaInsets().bottom;
            }
        }

        return padding;
    }
    return 0.0f;
}

float32 SizeMeasuringAlgorithm::ClampValue(float32 value) const
{
    UISizePolicyComponent::eSizePolicy policy = sizePolicy->GetPolicyByAxis(axis);
    if (policy != UISizePolicyComponent::PERCENT_OF_PARENT && policy != UISizePolicyComponent::IGNORE_SIZE)
    {
        return DAVA::Clamp(value, sizePolicy->GetMinValueByAxis(axis), sizePolicy->GetMaxValueByAxis(axis));
    }
    return value;
}

float32 SizeMeasuringAlgorithm::GetMinLimit() const
{
    return sizePolicy->GetMinValueByAxis(axis);
}

float32 SizeMeasuringAlgorithm::GetMaxLimit() const
{
    return sizePolicy->GetMaxValueByAxis(axis);
}

float32 SizeMeasuringAlgorithm::GetValue() const
{
    return sizePolicy->GetValueByAxis(axis);
}

float32 SizeMeasuringAlgorithm::Min(const std::shared_ptr<FormulaContext>& context, float32 a, float32 b) const
{
    return DAVA::Min(a, b);
}

float32 SizeMeasuringAlgorithm::Max(const std::shared_ptr<FormulaContext>& context, float32 a, float32 b) const
{
    return DAVA::Max(a, b);
}

float32 SizeMeasuringAlgorithm::Clamp(const std::shared_ptr<FormulaContext>& context, float32 val, float32 a, float32 b) const
{
    return DAVA::Clamp(val, a, b);
}

const LayoutMargins& SizeMeasuringAlgorithm::CalculateVisibilityMargins()
{
    const Rect& visibilityRect = layouter.GetVisibilityRect();
    const Vector<ControlLayoutData>& layoutData = layouter.GetLayoutData();

    int32 parent = data.GetParentIndex();
    float32 parentPos = 0.f;
    while (parent >= 0)
    {
        const ControlLayoutData& d = layoutData[parent];
        parentPos += d.GetPosition(axis);
        parent = d.GetParentIndex();
    }

    if (axis == Vector2::AXIS_X)
    {
        visibilityMargins.top = visibilityMargins.bottom = 0.f;
        visibilityMargins.left = DAVA::Max(0.f, visibilityRect.x - parentPos);
        visibilityMargins.right = DAVA::Max(0.f, (parentPos + parentSize) - (visibilityRect.x + visibilityRect.dx));
    }
    else
    {
        visibilityMargins.left = visibilityMargins.right = 0.f;
        visibilityMargins.top = DAVA::Max(0.f, visibilityRect.y - parentPos);
        visibilityMargins.bottom = DAVA::Max(0.f, (parentPos + parentSize) - (visibilityRect.y + visibilityRect.dy));
    }
    return visibilityMargins;
}

const LayoutMargins& SizeMeasuringAlgorithm::GetSafeAreaInsets()
{
    return layouter.GetSafeAreaInsets();
}
}
