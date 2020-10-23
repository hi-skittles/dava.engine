#pragma once

#include "Base/BaseTypes.h"
#include "Math/Vector.h"
#include "UI/Layouts/Private/ControlLayoutData.h"
#include "UI/Layouts/Private/Layouter.h"

namespace DAVA
{
class UIControl;
class UIFlowLayoutComponent;
class UISizePolicyComponent;

class FlowLayoutAlgorithm
{
public:
    FlowLayoutAlgorithm(Layouter& layouter);
    ~FlowLayoutAlgorithm();

    void Apply(ControlLayoutData& data, Vector2::eAxis axis);

private:
    struct LineInfo;

    void ProcessXAxis(ControlLayoutData& data, const UIFlowLayoutComponent* component);
    void CollectLinesInformation(ControlLayoutData& data, Vector<LineInfo>& lines);
    void FixHorizontalPadding(ControlLayoutData& data, Vector<LineInfo>& lines);
    void LayoutLine(ControlLayoutData& data, int32 firstIndex, int32 lastIndex, int32 childrenCount, float32 childrenSize);
    void CalculateHorizontalDynamicPaddingAndSpaces(ControlLayoutData& data, int32 firstIndex, int32 lastIndex);

    void ProcessYAxis(ControlLayoutData& data);
    void CalculateVerticalDynamicPaddingAndSpaces(ControlLayoutData& data);
    void LayoutLineVertically(ControlLayoutData& data, int32 firstIndex, int32 lastIndex, float32 top, float32 bottom);

    void CorrectPaddingAndSpacing(float32& leadingPadding, float32& trailingPadding, float32& spacing, bool dynamicPadding, bool dynamicSpacing, float32 restSize, int32 childrenCount);
    void SortLineItemsByContentDirection(int32 firstIndex, int32 lastIndex, List<uint32>& order, int32& realLastIndex);

private:
    Layouter& layouter;

    bool inverse = false;
    bool skipInvisible = true;

    float32 horizontalLeadingPadding = 0.0f;
    float32 horizontalTrailingPadding = 0.0f;
    float32 horizontalSpacing = 0.0f;
    bool dynamicHorizontalPadding = false;
    bool dynamicHorizontalInLinePadding = false;
    bool dynamicHorizontalSpacing = false;

    float32 topPadding = 0.0f;
    float32 bottomPadding = 0.0f;
    float32 verticalSpacing = 0.0f;
    bool dynamicVerticalPadding = false;
    bool dynamicVerticalSpacing = false;
};
}
