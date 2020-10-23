#ifndef __DAVAENGINE_SIZE_MEASURING_ALGORITHM_H__
#define __DAVAENGINE_SIZE_MEASURING_ALGORITHM_H__

#include "Base/BaseTypes.h"
#include "Math/Vector.h"
#include "Reflection/Reflection.h"
#include "UI/Layouts/Private/ControlLayoutData.h"
#include "UI/Layouts/Private/Layouter.h"
#include "UI/Layouts/Private/LayoutMargins.h"

namespace DAVA
{
class UISizePolicyComponent;
class UIFlowLayoutComponent;
class UILinearLayoutComponent;
class FormulaContext;

class SizeMeasuringAlgorithm : public ReflectionBase
{
    DAVA_VIRTUAL_REFLECTION(UISizeMeasuringAlgorithm, ReflectionBase);

public:
    SizeMeasuringAlgorithm(Layouter& layouter, ControlLayoutData& data, Vector2::eAxis axis, const UISizePolicyComponent* sizePolicy);
    ~SizeMeasuringAlgorithm() override;

    void SetParentSize(float32 parentSize);
    void SetParentRestSize(float32 parentRestSize);
    void SetParentLineSize(float32 size);
    void Apply();
    float32 Calculate();

private:
    float32 CalculateFixedSize() const;
    float32 CalculateChildrenSum() const;
    float32 CalculateDefaultChildrenSum() const;
    float32 CalculateHorizontalFlowLayoutChildrenSum() const;
    float32 CalculateVerticalFlowLayoutChildrenSum() const;
    float32 CalculateMaxChild() const;
    float32 CalculateFirstChild() const;
    float32 CalculateLastChild() const;
    float32 CalculateContent() const;
    const LayoutMargins& CalculateVisibilityMargins();
    const LayoutMargins& GetSafeAreaInsets();

    Vector2 GetContentPreferredSize(const Vector2& constraints) const; // -1.0f means no constraint for axis
    bool IsHeightDependsOnWidth() const;

    void ApplySize(float32 value);
    float32 GetSize(const ControlLayoutData& data) const;
    float32 GetLayoutPadding() const;
    float32 ClampValue(float32 value) const;

    // reflection methods
    float32 GetMinLimit() const;
    float32 GetMaxLimit() const;
    float32 GetValue() const;
    float32 Min(const std::shared_ptr<FormulaContext>& context, float32 a, float32 b) const;
    float32 Max(const std::shared_ptr<FormulaContext>& context, float32 a, float32 b) const;
    float32 Clamp(const std::shared_ptr<FormulaContext>& context, float32 val, float32 a, float32 b) const;

    Layouter& layouter;
    ControlLayoutData& data;
    Vector2::eAxis axis = Vector2::AXIS_X;
    float32 parentSize = 0.0f;
    float32 parentRestSize = 0.0f;
    float32 parentLineSize = 0.0f;

    const UISizePolicyComponent* sizePolicy = nullptr;
    const UILinearLayoutComponent* linearLayout = nullptr;
    const UIFlowLayoutComponent* flowLayout = nullptr;

    bool skipInvisible = false;

    LayoutMargins visibilityMargins;
};
}

#endif //__DAVAENGINE_SIZE_MEASURING_ALGORITHM_H__
