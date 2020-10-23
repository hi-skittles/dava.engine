#pragma once

#include "Functional/Function.h"
#include "Math/Vector.h"
#include "Math/Rect.h"
#include "UI/Layouts/Private/ControlLayoutData.h"
#include "UI/Layouts/Private/LayoutMargins.h"

namespace DAVA
{
class LayoutFormula;

class Layouter
{
public:
    void ApplyLayout(UIControl* control);
    void ApplyLayoutNonRecursive(UIControl* control);

    void CollectControls(UIControl* control, bool recursive);
    void CollectControlChildren(UIControl* control, int32 parentIndex, int32 index, bool recursive);

    void ProcessAxis(Vector2::eAxis axis, bool processSizes);
    void DoMeasurePhase(Vector2::eAxis axis);
    void DoLayoutPhase(Vector2::eAxis axis);

    void ApplySizesAndPositions();
    void ApplyPositions();

    void SetRtl(bool rtl);
    bool IsRtl() const;

    bool IsLeftNotch() const;
    bool IsRightNotch() const;
    const LayoutMargins& GetSafeAreaInsets() const;
    void SetSafeAreaInsets(float32 left, float32 top, float32 right, float32 bottom, bool isLeftNotch, bool isRightNotch);
    void SetSafeAreaInsets(const LayoutMargins& insets, bool isLeftNotch, bool isRightNotch);

    void SetVisibilityRect(const Rect& r);
    const Rect& GetVisibilityRect() const;

    const Vector<ControlLayoutData>& GetLayoutData() const;
    Vector<ControlLayoutData>& GetLayoutData();

    Function<void(UIControl*, Vector2::eAxis, const LayoutFormula*)> onFormulaRemoved;
    Function<void(UIControl*, Vector2::eAxis, const LayoutFormula*)> onFormulaProcessed;

private:
    Vector<ControlLayoutData> layoutData;
    bool isRtl = false;
    Rect visibilityRect;
    LayoutMargins safeAreaInsets;
    bool isLeftNotch = false;
    bool isRightNotch = false;
};

inline const Vector<ControlLayoutData>& Layouter::GetLayoutData() const
{
    return layoutData;
}

inline Vector<ControlLayoutData>& Layouter::GetLayoutData()
{
    return layoutData;
}

inline bool Layouter::IsRtl() const
{
    return isRtl;
}

inline const Rect& Layouter::GetVisibilityRect() const
{
    return visibilityRect;
}
}
